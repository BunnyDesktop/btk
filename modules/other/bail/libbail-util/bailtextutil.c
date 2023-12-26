/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2001 Sun Microsystems Inc.
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

#include <stdlib.h>
#include "bailtextutil.h"

/**
 * SECTION:bailtextutil
 * @Short_description: BailTextUtil is a utility class which can be used to
 *   implement some of the #BatkText functions for accessible objects
 *   which implement #BatkText.
 * @Title: BailTextUtil
 *
 * BailTextUtil is a utility class which can be used to implement the
 * #BatkText functions which get text for accessible objects which implement
 * #BatkText.
 *
 * In BAIL it is used by the accsesible objects for #BunnyCanvasText, #BtkEntry,
 * #BtkLabel, #BtkCellRendererText and #BtkTextView.
 */

static void bail_text_util_class_init      (BailTextUtilClass *klass);

static void bail_text_util_init            (BailTextUtil      *textutil);
static void bail_text_util_finalize        (BObject           *object);


static void get_bango_text_offsets         (BangoLayout         *layout,
                                            BtkTextBuffer       *buffer,
                                            BailOffsetType      function,
                                            BatkTextBoundary     boundary_type,
                                            gint                offset,
                                            gint                *start_offset,
                                            gint                *end_offset,
                                            BtkTextIter         *start_iter,
                                            BtkTextIter         *end_iter);
static BObjectClass *parent_class = NULL;

GType
bail_text_util_get_type(void)
{
  static GType type = 0;

  if (!type)
    {
      const GTypeInfo tinfo =
      {
        sizeof (BailTextUtilClass),
        (GBaseInitFunc) NULL, /* base init */
        (GBaseFinalizeFunc) NULL, /* base finalize */
        (GClassInitFunc) bail_text_util_class_init,
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL, /* class data */
        sizeof(BailTextUtil),
        0, /* nb preallocs */
        (GInstanceInitFunc) bail_text_util_init,
        NULL, /* value table */
      };

      type = g_type_register_static (B_TYPE_OBJECT, "BailTextUtil", &tinfo, 0);
    }
  return type;
}

/**
 * bail_text_util_new:
 *
 * This function creates a new BailTextUtil object.
 *
 * Returns: the BailTextUtil object
 **/
BailTextUtil*
bail_text_util_new (void)
{
  return BAIL_TEXT_UTIL (g_object_new (BAIL_TYPE_TEXT_UTIL, NULL));
}

static void
bail_text_util_init (BailTextUtil *textutil)
{
  textutil->buffer = NULL;
}

static void
bail_text_util_class_init (BailTextUtilClass *klass)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  bobject_class->finalize = bail_text_util_finalize;
}

static void
bail_text_util_finalize (BObject *object)
{
  BailTextUtil *textutil = BAIL_TEXT_UTIL (object);

  if (textutil->buffer)
    g_object_unref (textutil->buffer);

  B_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * bail_text_util_text_setup:
 * @textutil: The #BailTextUtil to be initialized.
 * @text: A gchar* which points to the text to be stored in the BailTextUtil
 *
 * This function initializes the BailTextUtil with the specified character string,
 **/
void
bail_text_util_text_setup (BailTextUtil *textutil,
                           const gchar  *text)
{
  g_return_if_fail (BAIL_IS_TEXT_UTIL (textutil));

  if (textutil->buffer)
    {
      if (!text)
        {
          g_object_unref (textutil->buffer);
          textutil->buffer = NULL;
          return;
        }
    }
  else
    {
      textutil->buffer = btk_text_buffer_new (NULL);
    }

  btk_text_buffer_set_text (textutil->buffer, text, -1);
}

/**
 * bail_text_util_buffer_setup:
 * @textutil: A #BailTextUtil to be initialized
 * @buffer: The #BtkTextBuffer which identifies the text to be stored in the BailUtil.
 *
 * This function initializes the BailTextUtil with the specified BtkTextBuffer
 **/
void
bail_text_util_buffer_setup  (BailTextUtil  *textutil,
                              BtkTextBuffer   *buffer)
{
  g_return_if_fail (BAIL_IS_TEXT_UTIL (textutil));

  textutil->buffer = g_object_ref (buffer);
}

/**
 * bail_text_util_get_text:
 * @textutil: A #BailTextUtil
 * @layout: A gpointer which is a BangoLayout, a BtkTreeView of NULL
 * @function: An enumeration specifying whether to return the text before, at, or
 *   after the offset.
 * @boundary_type: The boundary type.
 * @offset: The offset of the text in the BailTextUtil 
 * @start_offset: Address of location in which the start offset is returned
 * @end_offset: Address of location in which the end offset is returned
 *
 * This function gets the requested substring from the text in the BtkTextUtil.
 * The layout is used only for getting the text on a line. The value is NULL 
 * for a BtkTextView which is not wrapped, is a BtkTextView for a BtkTextView 
 * which is wrapped and is a BangoLayout otherwise.
 *
 * Returns: the substring requested
 **/
gchar*
bail_text_util_get_text (BailTextUtil    *textutil,
                         gpointer        layout,
                         BailOffsetType  function,
                         BatkTextBoundary boundary_type,
                         gint            offset,
                         gint            *start_offset,
                         gint            *end_offset)
{
  BtkTextIter start, end;
  gint line_number;
  BtkTextBuffer *buffer;

  g_return_val_if_fail (BAIL_IS_TEXT_UTIL (textutil), NULL);

  buffer = textutil->buffer;
  if (buffer == NULL)
    {
      *start_offset = 0;
      *end_offset = 0;
      return NULL;
    }

  if (!btk_text_buffer_get_char_count (buffer))
    {
      *start_offset = 0;
      *end_offset = 0;
      return g_strdup ("");
    }
  btk_text_buffer_get_iter_at_offset (buffer, &start, offset);

    
  end = start;

  switch (function)
    {
    case BAIL_BEFORE_OFFSET:
      switch (boundary_type)
        {
        case BATK_TEXT_BOUNDARY_CHAR:
          btk_text_iter_backward_char(&start);
          break;
        case BATK_TEXT_BOUNDARY_WORD_START:
          if (!btk_text_iter_starts_word (&start))
            btk_text_iter_backward_word_start (&start);
          end = start;
          btk_text_iter_backward_word_start(&start);
          break;
        case BATK_TEXT_BOUNDARY_WORD_END:
          if (btk_text_iter_inside_word (&start) &&
              !btk_text_iter_starts_word (&start))
            btk_text_iter_backward_word_start (&start);
          while (!btk_text_iter_ends_word (&start))
            {
              if (!btk_text_iter_backward_char (&start))
                break;
            }
          end = start;
          btk_text_iter_backward_word_start(&start);
          while (!btk_text_iter_ends_word (&start))
            {
              if (!btk_text_iter_backward_char (&start))
                break;
            }
          break;
        case BATK_TEXT_BOUNDARY_SENTENCE_START:
          if (!btk_text_iter_starts_sentence (&start))
            btk_text_iter_backward_sentence_start (&start);
          end = start;
          btk_text_iter_backward_sentence_start (&start);
          break;
        case BATK_TEXT_BOUNDARY_SENTENCE_END:
          if (btk_text_iter_inside_sentence (&start) &&
              !btk_text_iter_starts_sentence (&start))
            btk_text_iter_backward_sentence_start (&start);
          while (!btk_text_iter_ends_sentence (&start))
            {
              if (!btk_text_iter_backward_char (&start))
                break;
            }
          end = start;
          btk_text_iter_backward_sentence_start (&start);
          while (!btk_text_iter_ends_sentence (&start))
            {
              if (!btk_text_iter_backward_char (&start))
                break;
            }
          break;
        case BATK_TEXT_BOUNDARY_LINE_START:
          if (layout == NULL)
            {
              line_number = btk_text_iter_get_line (&start);
              if (line_number == 0)
                {
                  btk_text_buffer_get_iter_at_offset (buffer,
                    &start, 0);
                }
              else
                {
                  btk_text_iter_backward_line (&start);
                  btk_text_iter_forward_line (&start);
                }
              end = start;
              btk_text_iter_backward_line (&start);
            }
          else if BTK_IS_TEXT_VIEW (layout)
            {
              BtkTextView *view = BTK_TEXT_VIEW (layout);

              btk_text_view_backward_display_line_start (view, &start);
              end = start;
              btk_text_view_backward_display_line (view, &start);
            }
          else if (BANGO_IS_LAYOUT (layout))
            get_bango_text_offsets (BANGO_LAYOUT (layout),
                                    buffer,
                                    function,
                                    boundary_type,
                                    offset,
                                    start_offset,
                                    end_offset,
                                    &start,
                                    &end);
          break;
        case BATK_TEXT_BOUNDARY_LINE_END:
          if (layout == NULL)
            {
              line_number = btk_text_iter_get_line (&start);
              if (line_number == 0)
                {
                  btk_text_buffer_get_iter_at_offset (buffer,
                    &start, 0);
                  end = start;
                }
              else
                {
                  btk_text_iter_backward_line (&start);
                  end = start;
                  while (!btk_text_iter_ends_line (&start))
                    {
                      if (!btk_text_iter_backward_char (&start))
                        break;
                    }
                  btk_text_iter_forward_to_line_end (&end);
                }
            }
          else if BTK_IS_TEXT_VIEW (layout)
            {
              BtkTextView *view = BTK_TEXT_VIEW (layout);

              btk_text_view_backward_display_line_start (view, &start);
              if (!btk_text_iter_is_start (&start))
                {
                  btk_text_view_backward_display_line (view, &start);
                  end = start;
                  if (!btk_text_iter_is_start (&start))
                    {
                      btk_text_view_backward_display_line (view, &start);
                      btk_text_view_forward_display_line_end (view, &start);
                    }
                  btk_text_view_forward_display_line_end (view, &end);
                } 
              else
                {
                  end = start;
                }
            }
          else if (BANGO_IS_LAYOUT (layout))
            get_bango_text_offsets (BANGO_LAYOUT (layout),
                                    buffer,
                                    function,
                                    boundary_type,
                                    offset,
                                    start_offset,
                                    end_offset,
                                    &start,
                                    &end);
          break;
        }
      break;
 
    case BAIL_AT_OFFSET:
      switch (boundary_type)
        {
        case BATK_TEXT_BOUNDARY_CHAR:
          btk_text_iter_forward_char (&end);
          break;
        case BATK_TEXT_BOUNDARY_WORD_START:
          if (!btk_text_iter_starts_word (&start))
            btk_text_iter_backward_word_start (&start);
          if (btk_text_iter_inside_word (&end))
            btk_text_iter_forward_word_end (&end);
          while (!btk_text_iter_starts_word (&end))
            {
              if (!btk_text_iter_forward_char (&end))
                break;
            }
          break;
        case BATK_TEXT_BOUNDARY_WORD_END:
          if (btk_text_iter_inside_word (&start) &&
              !btk_text_iter_starts_word (&start))
            btk_text_iter_backward_word_start (&start);
          while (!btk_text_iter_ends_word (&start))
            {
              if (!btk_text_iter_backward_char (&start))
                break;
            }
          btk_text_iter_forward_word_end (&end);
          break;
        case BATK_TEXT_BOUNDARY_SENTENCE_START:
          if (!btk_text_iter_starts_sentence (&start))
            btk_text_iter_backward_sentence_start (&start);
          if (btk_text_iter_inside_sentence (&end))
            btk_text_iter_forward_sentence_end (&end);
          while (!btk_text_iter_starts_sentence (&end))
            {
              if (!btk_text_iter_forward_char (&end))
                break;
            }
          break;
        case BATK_TEXT_BOUNDARY_SENTENCE_END:
          if (btk_text_iter_inside_sentence (&start) &&
              !btk_text_iter_starts_sentence (&start))
            btk_text_iter_backward_sentence_start (&start);
          while (!btk_text_iter_ends_sentence (&start))
            {
              if (!btk_text_iter_backward_char (&start))
                break;
            }
          btk_text_iter_forward_sentence_end (&end);
          break;
        case BATK_TEXT_BOUNDARY_LINE_START:
          if (layout == NULL)
            {
              line_number = btk_text_iter_get_line (&start);
              if (line_number == 0)
                {
                  btk_text_buffer_get_iter_at_offset (buffer,
                    &start, 0);
                }
              else
                {
                  btk_text_iter_backward_line (&start);
                  btk_text_iter_forward_line (&start);
                }
              btk_text_iter_forward_line (&end);
            }
          else if BTK_IS_TEXT_VIEW (layout)
            {
              BtkTextView *view = BTK_TEXT_VIEW (layout);

              btk_text_view_backward_display_line_start (view, &start);
              /*
               * The call to btk_text_iter_forward_to_end() is needed
               * because of bug 81960
               */
              if (!btk_text_view_forward_display_line (view, &end))
                btk_text_iter_forward_to_end (&end);
            }
          else if BANGO_IS_LAYOUT (layout)
            get_bango_text_offsets (BANGO_LAYOUT (layout),
                                    buffer,
                                    function,
                                    boundary_type,
                                    offset,
                                    start_offset,
                                    end_offset,
                                    &start,
                                    &end);

          break;
        case BATK_TEXT_BOUNDARY_LINE_END:
          if (layout == NULL)
            {
              line_number = btk_text_iter_get_line (&start);
              if (line_number == 0)
                {
                  btk_text_buffer_get_iter_at_offset (buffer,
                    &start, 0);
                }
              else
                {
                  btk_text_iter_backward_line (&start);
                  btk_text_iter_forward_line (&start);
                }
              while (!btk_text_iter_ends_line (&start))
                {
                  if (!btk_text_iter_backward_char (&start))
                    break;
                }
              btk_text_iter_forward_to_line_end (&end);
            }
          else if BTK_IS_TEXT_VIEW (layout)
            {
              BtkTextView *view = BTK_TEXT_VIEW (layout);

              btk_text_view_backward_display_line_start (view, &start);
              if (!btk_text_iter_is_start (&start))
                {
                  btk_text_view_backward_display_line (view, &start);
                  btk_text_view_forward_display_line_end (view, &start);
                } 
              btk_text_view_forward_display_line_end (view, &end);
            }
          else if BANGO_IS_LAYOUT (layout)
            get_bango_text_offsets (BANGO_LAYOUT (layout),
                                    buffer,
                                    function,
                                    boundary_type,
                                    offset,
                                    start_offset,
                                    end_offset,
                                    &start,
                                    &end);
          break;
        }
      break;
  
    case BAIL_AFTER_OFFSET:
      switch (boundary_type)
        {
        case BATK_TEXT_BOUNDARY_CHAR:
          btk_text_iter_forward_char(&start);
          btk_text_iter_forward_chars(&end, 2);
          break;
        case BATK_TEXT_BOUNDARY_WORD_START:
          if (btk_text_iter_inside_word (&end))
            btk_text_iter_forward_word_end (&end);
          while (!btk_text_iter_starts_word (&end))
            {
              if (!btk_text_iter_forward_char (&end))
                break;
            }
          start = end;
          if (!btk_text_iter_is_end (&end))
            {
              btk_text_iter_forward_word_end (&end);
              while (!btk_text_iter_starts_word (&end))
                {
                  if (!btk_text_iter_forward_char (&end))
                    break;
                }
            }
          break;
        case BATK_TEXT_BOUNDARY_WORD_END:
          btk_text_iter_forward_word_end (&end);
          start = end;
          if (!btk_text_iter_is_end (&end))
            btk_text_iter_forward_word_end (&end);
          break;
        case BATK_TEXT_BOUNDARY_SENTENCE_START:
          if (btk_text_iter_inside_sentence (&end))
            btk_text_iter_forward_sentence_end (&end);
          while (!btk_text_iter_starts_sentence (&end))
            {
              if (!btk_text_iter_forward_char (&end))
                break;
            }
          start = end;
          if (!btk_text_iter_is_end (&end))
            {
              btk_text_iter_forward_sentence_end (&end);
              while (!btk_text_iter_starts_sentence (&end))
                {
                  if (!btk_text_iter_forward_char (&end))
                    break;
                }
            }
          break;
        case BATK_TEXT_BOUNDARY_SENTENCE_END:
          btk_text_iter_forward_sentence_end (&end);
          start = end;
          if (!btk_text_iter_is_end (&end))
            btk_text_iter_forward_sentence_end (&end);
          break;
        case BATK_TEXT_BOUNDARY_LINE_START:
          if (layout == NULL)
            {
              btk_text_iter_forward_line (&end);
              start = end;
              btk_text_iter_forward_line (&end);
            }
          else if BTK_IS_TEXT_VIEW (layout)
            {
              BtkTextView *view = BTK_TEXT_VIEW (layout);

              btk_text_view_forward_display_line (view, &end);
              start = end; 
              btk_text_view_forward_display_line (view, &end);
            }
          else if (BANGO_IS_LAYOUT (layout))
            get_bango_text_offsets (BANGO_LAYOUT (layout),
                                    buffer,
                                    function,
                                    boundary_type,
                                    offset,
                                    start_offset,
                                    end_offset,
                                    &start,
                                    &end);
          break;
        case BATK_TEXT_BOUNDARY_LINE_END:
          if (layout == NULL)
            {
              btk_text_iter_forward_line (&start);
              end = start;
              if (!btk_text_iter_is_end (&start))
                { 
                  while (!btk_text_iter_ends_line (&start))
                  {
                    if (!btk_text_iter_backward_char (&start))
                      break;
                  }
                  btk_text_iter_forward_to_line_end (&end);
                }
            }
          else if BTK_IS_TEXT_VIEW (layout)
            {
              BtkTextView *view = BTK_TEXT_VIEW (layout);

              btk_text_view_forward_display_line_end (view, &end);
              start = end; 
              btk_text_view_forward_display_line (view, &end);
              btk_text_view_forward_display_line_end (view, &end);
            }
          else if (BANGO_IS_LAYOUT (layout))
            get_bango_text_offsets (BANGO_LAYOUT (layout),
                                    buffer,
                                    function,
                                    boundary_type,
                                    offset,
                                    start_offset,
                                    end_offset,
                                    &start,
                                    &end);
          break;
        }
      break;
    }
  *start_offset = btk_text_iter_get_offset (&start);
  *end_offset = btk_text_iter_get_offset (&end);

  return btk_text_buffer_get_text (buffer, &start, &end, FALSE);
}

/**
 * bail_text_util_get_substring:
 * @textutil: A #BailTextUtil
 * @start_pos: The start position of the substring
 * @end_pos: The end position of the substring.
 *
 * Gets the substring indicated by @start_pos and @end_pos
 *
 * Returns: the substring indicated by @start_pos and @end_pos
 **/
gchar*
bail_text_util_get_substring (BailTextUtil *textutil,
                              gint         start_pos, 
                              gint         end_pos)
{
  BtkTextIter start, end;
  BtkTextBuffer *buffer;

  g_return_val_if_fail(BAIL_IS_TEXT_UTIL (textutil), NULL);

  buffer = textutil->buffer;
  if (buffer == NULL)
     return NULL;

  btk_text_buffer_get_iter_at_offset (buffer, &start, start_pos);
  if (end_pos < 0)
    btk_text_buffer_get_end_iter (buffer, &end);
  else
    btk_text_buffer_get_iter_at_offset (buffer, &end, end_pos);

  return btk_text_buffer_get_text (buffer, &start, &end, FALSE);
}

static void
get_bango_text_offsets (BangoLayout         *layout,
                        BtkTextBuffer       *buffer,
                        BailOffsetType      function,
                        BatkTextBoundary     boundary_type,
                        gint                offset,
                        gint                *start_offset,
                        gint                *end_offset,
                        BtkTextIter         *start_iter,
                        BtkTextIter         *end_iter)
{
  BangoLayoutIter *iter;
  BangoLayoutLine *line, *prev_line = NULL, *prev_prev_line = NULL;
  gint index, start_index, end_index;
  const gchar *text;
  gboolean found = FALSE;

  text = bango_layout_get_text (layout);
  index = g_utf8_offset_to_pointer (text, offset) - text;
  iter = bango_layout_get_iter (layout);
  do
    {
      line = bango_layout_iter_get_line (iter);
      start_index = line->start_index;
      end_index = start_index + line->length;

      if (index >= start_index && index <= end_index)
        {
          /*
           * Found line for offset
           */
          switch (function)
            {
            case BAIL_BEFORE_OFFSET:
                  /*
                   * We want the previous line
                   */
              if (prev_line)
                {
                  switch (boundary_type)
                    {
                    case BATK_TEXT_BOUNDARY_LINE_START:
                      end_index = start_index;
                      start_index = prev_line->start_index;
                      break;
                    case BATK_TEXT_BOUNDARY_LINE_END:
                      if (prev_prev_line)
                        start_index = prev_prev_line->start_index + 
                                  prev_prev_line->length;
                      end_index = prev_line->start_index + prev_line->length;
                      break;
                    default:
                      g_assert_not_reached();
                    }
                }
              else
                start_index = end_index = 0;
              break;
            case BAIL_AT_OFFSET:
              switch (boundary_type)
                {
                case BATK_TEXT_BOUNDARY_LINE_START:
                  if (bango_layout_iter_next_line (iter))
                    end_index = bango_layout_iter_get_line (iter)->start_index;
                  break;
                case BATK_TEXT_BOUNDARY_LINE_END:
                  if (prev_line)
                    start_index = prev_line->start_index + 
                                  prev_line->length;
                  break;
                default:
                  g_assert_not_reached();
                }
              break;
            case BAIL_AFTER_OFFSET:
               /*
                * We want the next line
                */
              if (bango_layout_iter_next_line (iter))
                {
                  line = bango_layout_iter_get_line (iter);
                  switch (boundary_type)
                    {
                    case BATK_TEXT_BOUNDARY_LINE_START:
                      start_index = line->start_index;
                      if (bango_layout_iter_next_line (iter))
                        end_index = bango_layout_iter_get_line (iter)->start_index;
                      else
                        end_index = start_index + line->length;
                      break;
                    case BATK_TEXT_BOUNDARY_LINE_END:
                      start_index = end_index;
                      end_index = line->start_index + line->length;
                      break;
                    default:
                      g_assert_not_reached();
                    }
                }
              else
                start_index = end_index;
              break;
            }
          found = TRUE;
          break;
        }
      prev_prev_line = prev_line; 
      prev_line = line; 
    }
  while (bango_layout_iter_next_line (iter));

  if (!found)
    {
      start_index = prev_line->start_index + prev_line->length;
      end_index = start_index;
    }
  bango_layout_iter_free (iter);
  *start_offset = g_utf8_pointer_to_offset (text, text + start_index);
  *end_offset = g_utf8_pointer_to_offset (text, text + end_index);
 
  btk_text_buffer_get_iter_at_offset (buffer, start_iter, *start_offset);
  btk_text_buffer_get_iter_at_offset (buffer, end_iter, *end_offset);
}
