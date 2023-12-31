/* testtextbuffer.c -- Simplistic test suite
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
#include <string.h>

#include <btk/btk.h>
#include "btk/btktexttypes.h" /* Private header, for UNKNOWN_CHAR */

static void
btk_text_iter_spew (const BtkTextIter *iter, const bchar *desc)
{
  g_print (" %20s: line %d / char %d / line char %d / line byte %d\n",
           desc,
           btk_text_iter_get_line (iter),
           btk_text_iter_get_offset (iter),
           btk_text_iter_get_line_offset (iter),
           btk_text_iter_get_line_index (iter));
}

static void
check_get_set_text (BtkTextBuffer *buffer,
                    const char    *str)
{
  BtkTextIter start, end, iter;
  char *text;
  int n;
  
  btk_text_buffer_set_text (buffer, str, -1);
  if (btk_text_buffer_get_char_count (buffer) != g_utf8_strlen (str, -1))
    g_error ("Wrong number of chars (%d not %d)",
             btk_text_buffer_get_char_count (buffer),
             (int) g_utf8_strlen (str, -1));
  btk_text_buffer_get_bounds (buffer, &start, &end);
  text = btk_text_buffer_get_text (buffer, &start, &end, TRUE);
  if (strcmp (text, str) != 0)
    g_error ("Got '%s' as buffer contents", text);
  g_free (text);

  /* line char counts */
  iter = start;
  n = 0;
  do
    {
      n += btk_text_iter_get_chars_in_line (&iter);
    }
  while (btk_text_iter_forward_line (&iter));

  if (n != btk_text_buffer_get_char_count (buffer))
    g_error ("Sum of chars in lines is %d but buffer char count is %d",
             n, btk_text_buffer_get_char_count (buffer));

  /* line byte counts */
  iter = start;
  n = 0;
  do
    {
      n += btk_text_iter_get_bytes_in_line (&iter);
    }
  while (btk_text_iter_forward_line (&iter));

  if (n != strlen (str))
    g_error ("Sum of chars in lines is %d but buffer byte count is %d",
             n, (int) strlen (str));
  
  btk_text_buffer_set_text (buffer, "", -1);

  n = btk_text_buffer_get_line_count (buffer);
  if (n != 1)
    g_error ("%d lines, expected 1", n);

  n = btk_text_buffer_get_char_count (buffer);
  if (n != 0)
    g_error ("%d chars, expected 0", n);
}

static bint
count_toggles_at_iter (BtkTextIter *iter,
                       BtkTextTag  *of_tag)
{
  GSList *tags;
  GSList *tmp;
  bint count = 0;
  
  /* get toggle-ons and toggle-offs */
  tags = btk_text_iter_get_toggled_tags (iter, TRUE);
  tags = b_slist_concat (tags,
                         btk_text_iter_get_toggled_tags (iter, FALSE));
  
  tmp = tags;
  while (tmp != NULL)
    {
      if (of_tag == NULL)
        ++count;
      else if (of_tag == tmp->data)
        ++count;
      
      tmp = b_slist_next (tmp);
    }
  
  b_slist_free (tags);

  return count;
}

static bint
count_toggles_in_range_by_char (BtkTextBuffer     *buffer,
                                BtkTextTag        *of_tag,
                                const BtkTextIter *start,
                                const BtkTextIter *end)
{
  BtkTextIter iter;
  bint count = 0;
  
  iter = *start;
  do
    {
      count += count_toggles_at_iter (&iter, of_tag);
      if (!btk_text_iter_forward_char (&iter))
        {
          /* end iterator */
          count += count_toggles_at_iter (&iter, of_tag);
          break;
        }
    }
  while (btk_text_iter_compare (&iter, end) <= 0);
  
  return count;
}

static bint
count_toggles_in_buffer (BtkTextBuffer *buffer,
                         BtkTextTag    *of_tag)
{
  BtkTextIter start, end;

  btk_text_buffer_get_bounds (buffer, &start, &end);

  return count_toggles_in_range_by_char (buffer, of_tag, &start, &end);
}

static void
check_specific_tag_in_range (BtkTextBuffer     *buffer,
                             const bchar       *tag_name,
                             const BtkTextIter *start,
                             const BtkTextIter *end)
{
  BtkTextIter iter;
  BtkTextTag *tag;
  bboolean state;
  bint count;
  bint buffer_count;
  bint last_offset;

  if (btk_text_iter_compare (start, end) > 0)
    {
      g_print ("  (inverted range for checking tags, skipping)\n");
      return;
    }
  
  tag = btk_text_tag_table_lookup (btk_text_buffer_get_tag_table (buffer),
                                   tag_name);

  buffer_count = count_toggles_in_range_by_char (buffer, tag, start, end);
  
  state = FALSE;
  count = 0;

  last_offset = -1;
  iter = *start;
  if (btk_text_iter_toggles_tag (&iter, tag) ||
      btk_text_iter_forward_to_tag_toggle (&iter, tag))
    {
      do
        {
          bint this_offset;
          
          ++count;

          this_offset = btk_text_iter_get_offset (&iter);

          if (this_offset <= last_offset)
            g_error ("forward_to_tag_toggle moved in wrong direction");

          last_offset = this_offset;
          
          if (btk_text_iter_begins_tag (&iter, tag))
            {
              if (state)
                g_error ("Tag %p is already on, and was toggled on?", tag);
              state = TRUE;
            }          
          else if (btk_text_iter_ends_tag (&iter, tag))
            {
              if (!state)
                g_error ("Tag %p toggled off, but wasn't toggled on?", tag);
              state = FALSE;
            }
          else
            g_error ("forward_to_tag_toggle went to a location without a toggle");
        }
      while (btk_text_iter_forward_to_tag_toggle (&iter, tag) &&
             btk_text_iter_compare (&iter, end) <= 0);
    }

  if (count != buffer_count)
    g_error ("Counted %d tags iterating by char, %d iterating forward by tag toggle\n",
             buffer_count, count);
  
  state = FALSE;
  count = 0;
  
  iter = *end;
  last_offset = btk_text_iter_get_offset (&iter);
  if (btk_text_iter_toggles_tag (&iter, tag) ||
      btk_text_iter_backward_to_tag_toggle (&iter, tag))
    {
      do
        {
          bint this_offset;
          
          ++count;

          this_offset = btk_text_iter_get_offset (&iter);
          
          if (this_offset >= last_offset)
            g_error ("backward_to_tag_toggle moved in wrong direction");
          
          last_offset = this_offset;

          if (btk_text_iter_begins_tag (&iter, tag))
            {
              if (!state)
                g_error ("Tag %p wasn't on when we got to the on toggle going backward?", tag);
              state = FALSE;
            }
          else if (btk_text_iter_ends_tag (&iter, tag))
            {
              if (state)
                g_error ("Tag %p off toggle, but we were already inside a tag?", tag);
              state = TRUE;
            }
          else
            g_error ("backward_to_tag_toggle went to a location without a toggle");
        }
      while (btk_text_iter_backward_to_tag_toggle (&iter, tag) &&
             btk_text_iter_compare (&iter, start) >= 0);
    }

  if (count != buffer_count)
    g_error ("Counted %d tags iterating by char, %d iterating backward by tag toggle\n",
             buffer_count, count);
}

static void
check_specific_tag (BtkTextBuffer *buffer,
                    const bchar   *tag_name)
{
  BtkTextIter start, end;

  btk_text_buffer_get_bounds (buffer, &start, &end);
  check_specific_tag_in_range (buffer, tag_name, &start, &end);
  btk_text_iter_forward_chars (&start, 2);
  btk_text_iter_backward_chars (&end, 2);
  if (btk_text_iter_compare (&start, &end) < 0)
    check_specific_tag_in_range (buffer, tag_name, &start, &end);
}

static void
run_tests (BtkTextBuffer *buffer)
{
  BtkTextIter iter;
  BtkTextIter start;
  BtkTextIter end;
  BtkTextIter mark;
  bint i, j;
  bint num_chars;
  BtkTextMark *bar_mark;
  BtkTextTag *tag;
  GHashTable *tag_states;
  bint count;
  bint buffer_count;
  
  btk_text_buffer_get_bounds (buffer, &start, &end);

  /* Check that walking the tree via chars and via iterators produces
   * the same number of indexable locations.
   */
  num_chars = btk_text_buffer_get_char_count (buffer);
  iter = start;
  bar_mark = btk_text_buffer_create_mark (buffer, "bar", &iter, FALSE);
  i = 0;
  while (i < num_chars)
    {
      BtkTextIter current;
      BtkTextMark *foo_mark;

      btk_text_buffer_get_iter_at_offset (buffer, &current, i);

      if (!btk_text_iter_equal (&iter, &current))
        {
          g_error ("get_char_index didn't return current iter");
        }

      j = btk_text_iter_get_offset (&iter);

      if (i != j)
        {
          g_error ("iter converted to %d not %d", j, i);
        }

      /* get/set mark */
      btk_text_buffer_get_iter_at_mark (buffer, &mark, bar_mark);

      if (!btk_text_iter_equal (&iter, &mark))
        {
          btk_text_iter_spew (&iter, "iter");
          btk_text_iter_spew (&mark, "mark");
          g_error ("Mark not moved to the right place.");
        }

      foo_mark = btk_text_buffer_create_mark (buffer, "foo", &iter, FALSE);
      btk_text_buffer_get_iter_at_mark (buffer, &mark, foo_mark);
      btk_text_buffer_delete_mark (buffer, foo_mark);

      if (!btk_text_iter_equal (&iter, &mark))
        {
          btk_text_iter_spew (&iter, "iter");
          btk_text_iter_spew (&mark, "mark");
          g_error ("Mark not created in the right place.");
        }

      if (btk_text_iter_is_end (&iter))
        g_error ("iterators ran out before chars (offset %d of %d)",
                 i, num_chars);

      btk_text_iter_forward_char (&iter);

      btk_text_buffer_move_mark (buffer, bar_mark, &iter);

      ++i;
    }

  if (!btk_text_iter_equal (&iter, &end))
    g_error ("Iterating over all chars didn't end with the end iter");

  /* Do the tree-walk backward
   */
  num_chars = btk_text_buffer_get_char_count (buffer);
  btk_text_buffer_get_iter_at_offset (buffer, &iter, -1);

  btk_text_buffer_move_mark (buffer, bar_mark, &iter);

  i = num_chars;

  if (!btk_text_iter_equal (&iter, &end))
    g_error ("iter at char -1 is not equal to the end iterator");

  while (i >= 0)
    {
      BtkTextIter current;
      BtkTextMark *foo_mark;

      btk_text_buffer_get_iter_at_offset (buffer, &current, i);

      if (!btk_text_iter_equal (&iter, &current))
        {
          g_error ("get_char_index didn't return current iter while going backward");
        }
      j = btk_text_iter_get_offset (&iter);

      if (i != j)
        {
          g_error ("going backward, iter converted to %d not %d", j, i);
        }

      /* get/set mark */
      btk_text_buffer_get_iter_at_mark (buffer, &mark, bar_mark);

      if (!btk_text_iter_equal (&iter, &mark))
        {
          btk_text_iter_spew (&iter, "iter");
          btk_text_iter_spew (&mark, "mark");
          g_error ("Mark not moved to the right place.");
        }

      foo_mark = btk_text_buffer_create_mark (buffer, "foo", &iter, FALSE);
      btk_text_buffer_get_iter_at_mark (buffer, &mark, foo_mark);
      btk_text_buffer_delete_mark (buffer, foo_mark);

      if (!btk_text_iter_equal (&iter, &mark))
        {
          btk_text_iter_spew (&iter, "iter");
          btk_text_iter_spew (&mark, "mark");
          g_error ("Mark not created in the right place.");
        }

      if (i > 0)
        {
          if (!btk_text_iter_backward_char (&iter))
            g_error ("iterators ran out before char indexes");

          btk_text_buffer_move_mark (buffer, bar_mark, &iter);
        }
      else
        {
          if (btk_text_iter_backward_char (&iter))
            g_error ("went backward from 0?");
        }

      --i;
    }

  if (!btk_text_iter_equal (&iter, &start))
    g_error ("Iterating backward over all chars didn't end with the start iter");

  /*
   * Check that get_line_count returns the same number of lines
   * as walking the tree by line
   */
  i = 1; /* include current (first) line */
  btk_text_buffer_get_iter_at_line (buffer, &iter, 0);
  while (btk_text_iter_forward_line (&iter))
    ++i;

  if (i != btk_text_buffer_get_line_count (buffer))
    g_error ("Counted %d lines, buffer has %d", i,
             btk_text_buffer_get_line_count (buffer));

  /*
   * Check that moving over tag toggles thinks about working.
   */

  buffer_count = count_toggles_in_buffer (buffer, NULL);
  
  tag_states = g_hash_table_new (NULL, NULL);
  count = 0;
  
  btk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
  if (btk_text_iter_toggles_tag (&iter, NULL) ||
      btk_text_iter_forward_to_tag_toggle (&iter, NULL))
    {
      do
        {
          GSList *tags;
          GSList *tmp;
          bboolean found_some = FALSE;
          
          /* get toggled-on tags */
          tags = btk_text_iter_get_toggled_tags (&iter, TRUE);

          if (tags)
            found_some = TRUE;
          
          tmp = tags;
          while (tmp != NULL)
            {
              ++count;
              
              tag = tmp->data;
              
              if (g_hash_table_lookup (tag_states, tag))
                g_error ("Tag %p is already on, and was toggled on?", tag);

              g_hash_table_insert (tag_states, tag, BINT_TO_POINTER (TRUE));
          
              tmp = b_slist_next (tmp);
            }

          b_slist_free (tags);
      
          /* get toggled-off tags */
          tags = btk_text_iter_get_toggled_tags (&iter, FALSE);

          if (tags)
            found_some = TRUE;
          
          tmp = tags;
          while (tmp != NULL)
            {
              ++count;
              
              tag = tmp->data;

              if (!g_hash_table_lookup (tag_states, tag))
                g_error ("Tag %p is already off, and was toggled off?", tag);

              g_hash_table_remove (tag_states, tag);
          
              tmp = b_slist_next (tmp);
            }

          b_slist_free (tags);

          if (!found_some)
            g_error ("No tags found going forward to tag toggle.");

        }
      while (btk_text_iter_forward_to_tag_toggle (&iter, NULL));
    }
  
  g_hash_table_destroy (tag_states);

  if (count != buffer_count)
    g_error ("Counted %d tags iterating by char, %d iterating by tag toggle\n",
             buffer_count, count);
  
  /* Go backward; here TRUE in the hash means we saw
   * an off toggle last.
   */
  
  tag_states = g_hash_table_new (NULL, NULL);
  count = 0;
  
  btk_text_buffer_get_end_iter (buffer, &iter);
  if (btk_text_iter_toggles_tag (&iter, NULL) ||
      btk_text_iter_backward_to_tag_toggle (&iter, NULL))
    {
      do
        {
          GSList *tags;
          GSList *tmp;
          bboolean found_some = FALSE;
          
          /* get toggled-off tags */
          tags = btk_text_iter_get_toggled_tags (&iter, FALSE);

          if (tags)
            found_some = TRUE;
          
          tmp = tags;
          while (tmp != NULL)
            {
              ++count;
              
              tag = tmp->data;

              if (g_hash_table_lookup (tag_states, tag))
                g_error ("Tag %p has two off-toggles in a row?", tag);
          
              g_hash_table_insert (tag_states, tag, BINT_TO_POINTER (TRUE));
          
              tmp = b_slist_next (tmp);
            }

          b_slist_free (tags);
      
          /* get toggled-on tags */
          tags = btk_text_iter_get_toggled_tags (&iter, TRUE);

          if (tags)
            found_some = TRUE;
          
          tmp = tags;
          while (tmp != NULL)
            {
              ++count;
              
              tag = tmp->data;

              if (!g_hash_table_lookup (tag_states, tag))
                g_error ("Tag %p was toggled on, but saw no off-toggle?", tag);

              g_hash_table_remove (tag_states, tag);
          
              tmp = b_slist_next (tmp);
            }

          b_slist_free (tags);

          if (!found_some)
            g_error ("No tags found going backward to tag toggle.");
        }
      while (btk_text_iter_backward_to_tag_toggle (&iter, NULL));
    }
  
  g_hash_table_destroy (tag_states);

  if (count != buffer_count)
    g_error ("Counted %d tags iterating by char, %d iterating by tag toggle\n",
             buffer_count, count);

  check_specific_tag (buffer, "fg_red");
  check_specific_tag (buffer, "bg_green");
  check_specific_tag (buffer, "front_tag");
  check_specific_tag (buffer, "center_tag");
  check_specific_tag (buffer, "end_tag");
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

static void
fill_buffer (BtkTextBuffer *buffer)
{
  BtkTextTag *tag;
  BdkColor color, color2;
  BtkTextIter iter;
  BtkTextIter iter2;
  BdkPixbuf *pixbuf;
  int i;

  color.red = color.green = 0;
  color.blue = 0xffff;
  color2.red = 0xfff;
  color2.blue = 0x0;
  color2.green = 0;
  
  btk_text_buffer_create_tag (buffer, "fg_blue",
                              "foreground_bdk", &color,
                              "background_bdk", &color2,
                              "font", "-*-courier-bold-r-*-*-30-*-*-*-*-*-*-*",
                              NULL);

  color.blue = color.green = 0;
  color.red = 0xffff;
  
  btk_text_buffer_create_tag (buffer, "fg_red",
                              "rise", -4,
                              "foreground_bdk", &color,
                              NULL);

  color.blue = color.red = 0;
  color.green = 0xffff;
  
  btk_text_buffer_create_tag (buffer, "bg_green",
                              "background_bdk", &color,
                              "font", "-*-courier-bold-r-*-*-10-*-*-*-*-*-*-*",
                              NULL);

  pixbuf = bdk_pixbuf_new_from_xpm_data (book_closed_xpm);

  g_assert (pixbuf != NULL);

  i = 0;
  while (i < 10)
    {
      bchar *str;

      btk_text_buffer_get_iter_at_offset (buffer, &iter, 0);

      btk_text_buffer_insert_pixbuf (buffer, &iter, pixbuf);

      btk_text_buffer_get_iter_at_offset (buffer, &iter, 1);

      btk_text_buffer_insert_pixbuf (buffer, &iter, pixbuf);

      str = g_strdup_printf ("%d Hello World!\nwoo woo woo woo woo woo woo woo\n",
                            i);

      btk_text_buffer_insert (buffer, &iter, str, -1);

      g_free (str);

      btk_text_buffer_insert (buffer, &iter,
                              "(Hello World!)\nfoo foo Hello this is some text we are using to text word wrap. It has punctuation! gee; blah - hmm, great.\nnew line\n\n"
                              /* This is UTF8 stuff, Emacs doesn't
                                 really know how to display it */
                              "Spanish (Espa\303\261ol) \302\241Hola! / French (Fran\303\247ais) Bonjour, Salut / German (Deutsch S\303\274d) Gr\303\274\303\237 Gott (testing Latin-1 chars encoded in UTF8)\nThai (we can't display this, just making sure we don't crash)  (\340\270\240\340\270\262\340\270\251\340\270\262\340\271\204\340\270\227\340\270\242)  \340\270\252\340\270\247\340\270\261\340\270\252\340\270\224\340\270\265\340\270\204\340\270\243\340\270\261\340\270\232, \340\270\252\340\270\247\340\270\261\340\270\252\340\270\224\340\270\265\340\270\204\340\271\210\340\270\260\n",
                              -1);

      btk_text_buffer_insert_pixbuf (buffer, &iter, pixbuf);
      btk_text_buffer_insert_pixbuf (buffer, &iter, pixbuf);

      btk_text_buffer_get_iter_at_offset (buffer, &iter, 4);

      btk_text_buffer_insert_pixbuf (buffer, &iter, pixbuf);

      btk_text_buffer_get_iter_at_offset (buffer, &iter, 7);

      btk_text_buffer_insert_pixbuf (buffer, &iter, pixbuf);

      btk_text_buffer_get_iter_at_offset (buffer, &iter, 8);

      btk_text_buffer_insert_pixbuf (buffer, &iter, pixbuf);

      btk_text_buffer_get_iter_at_line_offset (buffer, &iter, 0, 8);
      iter2 = iter;
      btk_text_iter_forward_chars (&iter2, 10);

      btk_text_buffer_apply_tag_by_name (buffer, "fg_blue", &iter, &iter2);

      btk_text_iter_forward_chars (&iter, 7);
      btk_text_iter_forward_chars (&iter2, 10);

      btk_text_buffer_apply_tag_by_name (buffer, "bg_green", &iter, &iter2);

      btk_text_iter_forward_chars (&iter, 12);
      btk_text_iter_forward_chars (&iter2, 10);

      btk_text_buffer_apply_tag_by_name (buffer, "bg_green", &iter, &iter2);

      btk_text_iter_forward_chars (&iter, 10);
      btk_text_iter_forward_chars (&iter2, 15);

      btk_text_buffer_apply_tag_by_name (buffer, "fg_red", &iter, &iter2);
      btk_text_buffer_apply_tag_by_name (buffer, "fg_blue", &iter, &iter2);

      btk_text_iter_forward_chars (&iter, 20);
      btk_text_iter_forward_chars (&iter2, 20);

      btk_text_buffer_apply_tag_by_name (buffer, "fg_red", &iter, &iter2);
      btk_text_buffer_apply_tag_by_name (buffer, "fg_blue", &iter, &iter2);

      btk_text_iter_backward_chars (&iter, 25);
      btk_text_iter_forward_chars (&iter2, 5);

      btk_text_buffer_apply_tag_by_name (buffer, "fg_red", &iter, &iter2);
      btk_text_buffer_apply_tag_by_name (buffer, "fg_blue", &iter, &iter2);

      btk_text_iter_forward_chars (&iter, 15);
      btk_text_iter_backward_chars (&iter2, 10);

      btk_text_buffer_remove_tag_by_name (buffer, "fg_red", &iter, &iter2);
      btk_text_buffer_remove_tag_by_name (buffer, "fg_blue", &iter, &iter2);

      ++i;
    }

  /* Put in tags that are just at the beginning, and just near the end,
   * and just near the middle.
   */
  tag = btk_text_buffer_create_tag (buffer, "front_tag", NULL);
  btk_text_buffer_get_iter_at_offset (buffer, &iter, 3);
  btk_text_buffer_get_iter_at_offset (buffer, &iter2, 300);

  btk_text_buffer_apply_tag (buffer, tag, &iter, &iter2);  
  
  tag = btk_text_buffer_create_tag (buffer, "end_tag", NULL);
  btk_text_buffer_get_end_iter (buffer, &iter2);
  btk_text_iter_backward_chars (&iter2, 12);
  iter = iter2;
  btk_text_iter_backward_chars (&iter, 157);

  btk_text_buffer_apply_tag (buffer, tag, &iter, &iter2);
  
  tag = btk_text_buffer_create_tag (buffer, "center_tag", NULL);
  btk_text_buffer_get_iter_at_offset (buffer, &iter,
                                      btk_text_buffer_get_char_count (buffer)/2);
  btk_text_iter_backward_chars (&iter, 37);
  iter2 = iter;
  btk_text_iter_forward_chars (&iter2, 57);

  btk_text_buffer_apply_tag (buffer, tag, &iter, &iter2);  

  g_object_unref (pixbuf);
}


/*
 * Line separator tests (initially to avoid regression on bugzilla #57428)
 */

static void
test_line_separation (const char* str,
                      bboolean    expect_next_line,
                      bboolean    expect_end_iter,
                      int         expected_line_count,
                      int         expected_line_break,
                      int         expected_next_line_start)
{
  BtkTextIter iter;
  BtkTextBuffer* buffer;
  bboolean on_next_line;
  bboolean on_end_iter;
  bint new_pos;

  buffer = btk_text_buffer_new (NULL);

  btk_text_buffer_set_text (buffer, str, -1);
  btk_text_buffer_get_iter_at_offset (buffer, &iter, expected_line_break);

  g_assert (btk_text_iter_ends_line (&iter) || btk_text_iter_is_end (&iter));

  g_assert (btk_text_buffer_get_line_count (buffer) == expected_line_count);
  
  on_next_line = btk_text_iter_forward_line (&iter);

  g_assert (expect_next_line == on_next_line);

  on_end_iter = btk_text_iter_is_end (&iter);

  g_assert (on_end_iter == expect_end_iter);
  
  new_pos = btk_text_iter_get_offset (&iter);
    
  if (on_next_line)
    g_assert (expected_next_line_start == new_pos);

  ++expected_line_break;
  while (expected_line_break < expected_next_line_start)
    {
      btk_text_buffer_get_iter_at_offset (buffer, &iter, expected_line_break);

      g_assert (!btk_text_iter_ends_line (&iter));

      on_next_line = btk_text_iter_forward_line (&iter);
        
      g_assert (expect_next_line == on_next_line);
        
      new_pos = btk_text_iter_get_offset (&iter);
        
      if (on_next_line)
        g_assert (expected_next_line_start == new_pos);
        
      ++expected_line_break;
    }

  /* FIXME tests for backward line */
  
  g_object_unref (buffer);
}

/* there are cases where \r and \n should not be treated like \r\n,
 * originally bug #337022. */
static void
split_r_n_separators_test (void)
{
  BtkTextBuffer *buffer;
  BtkTextIter iter;

  buffer = btk_text_buffer_new (NULL);

  btk_text_buffer_set_text (buffer, "foo\ra\nbar\n", -1);

  /* delete 'a' so that we have

     1 foo\r
     2 \n
     3 bar\n

   * and both \r and \n are line separators */

  btk_text_buffer_get_iter_at_offset (buffer, &iter, 5);
  btk_text_buffer_backspace (buffer, &iter, TRUE, TRUE);

  g_assert (btk_text_iter_ends_line (&iter));

  btk_text_buffer_get_iter_at_offset (buffer, &iter, 3);
  g_assert (btk_text_iter_ends_line (&iter));

  g_object_unref (buffer);
}

static void
test_line_separator (void)
{
  char *str;
  char buf[7] = { '\0', };

  /* Only one character has type G_UNICODE_PARAGRAPH_SEPARATOR in
   * Unicode 3.0; update this if that changes.
   */
#define PARAGRAPH_SEPARATOR 0x2029
  
  test_line_separation ("line", FALSE, TRUE, 1, 4, 4);
  test_line_separation ("line\r\n", FALSE, TRUE, 2, 4, 6);
  test_line_separation ("line\r", FALSE, TRUE, 2, 4, 5);
  test_line_separation ("line\n", FALSE, TRUE, 2, 4, 5);
  test_line_separation ("line\rqw", TRUE, FALSE, 2, 4, 5);
  test_line_separation ("line\nqw", TRUE, FALSE, 2, 4, 5);
  test_line_separation ("line\r\nqw", TRUE, FALSE, 2, 4, 6);
  
  g_unichar_to_utf8 (PARAGRAPH_SEPARATOR, buf);
  
  str = g_strdup_printf ("line%s", buf);
  test_line_separation (str, FALSE, TRUE, 2, 4, 5);
  g_free (str);
  str = g_strdup_printf ("line%sqw", buf);
  test_line_separation (str, TRUE, FALSE, 2, 4, 5);
  g_free (str);

  split_r_n_separators_test ();
}

static void
test_backspace (void)
{
  BtkTextBuffer *buffer;
  BtkTextIter iter;
  bboolean ret;

  buffer = btk_text_buffer_new (NULL);

  btk_text_buffer_set_text (buffer, "foo", -1);
  btk_text_buffer_get_iter_at_offset (buffer, &iter, 2);
  ret = btk_text_buffer_backspace (buffer, &iter, TRUE, TRUE);
  g_assert (ret);
  g_assert_cmpint (1, ==, btk_text_iter_get_offset (&iter));
  g_assert_cmpint (2, ==, btk_text_buffer_get_char_count (buffer));

  btk_text_buffer_set_text (buffer, "foo", -1);
  btk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
  ret = btk_text_buffer_backspace (buffer, &iter, TRUE, TRUE);
  g_assert (!ret);
  g_assert_cmpint (0, ==, btk_text_iter_get_offset (&iter));
  g_assert_cmpint (3, ==, btk_text_buffer_get_char_count (buffer));

  /* test bug #544724 */
  btk_text_buffer_set_text (buffer, "foo\r\n\r\nbar", -1);
  btk_text_buffer_get_iter_at_offset (buffer, &iter, 5);
  ret = btk_text_buffer_backspace (buffer, &iter, TRUE, TRUE);
  g_assert (ret);
  g_assert_cmpint (0, ==, btk_text_iter_get_line (&iter));
  g_assert_cmpint (8, ==, btk_text_buffer_get_char_count (buffer));

  g_object_unref (buffer);
}

static void
test_logical_motion (void)
{
  char *str;
  char buf1[7] = { '\0', };
  char buf2[7] = { '\0', };
  char buf3[7] = { '\0', };
  int expected[30];
  int expected_steps;
  int i;
  BtkTextBuffer *buffer;
  BtkTextIter iter;
  
  buffer = btk_text_buffer_new (NULL);
  
#define LEADING_JAMO 0x1111
#define VOWEL_JAMO 0x1167
#define TRAILING_JAMO 0x11B9
  
  g_unichar_to_utf8 (LEADING_JAMO, buf1);
  g_unichar_to_utf8 (VOWEL_JAMO, buf2);
  g_unichar_to_utf8 (TRAILING_JAMO, buf3);

  /* Build the string "abc<leading><vowel><trailing>def\r\nxyz" */
  str = g_strconcat ("abc", buf1, buf2, buf3, "def\r\nxyz", NULL);
  btk_text_buffer_set_text (buffer, str, -1);
  g_free (str);
  
  /* Check cursor positions */
  memset (expected, 0, sizeof (expected));
  expected[0] = 0;    /* before 'a' */
  expected[1] = 1;    /* before 'b' */
  expected[2] = 2;    /* before 'c' */
  expected[3] = 3;    /* before jamo */
  expected[4] = 6;    /* before 'd' */
  expected[5] = 7;    /* before 'e' */
  expected[6] = 8;    /* before 'f' */
  expected[7] = 9;    /* before '\r' */
  expected[8] = 11;   /* before 'x' */
  expected[9] = 12;   /* before 'y' */
  expected[10] = 13;  /* before 'z' */
  expected[11] = 14;  /* after 'z' (only matters going backward) */
  expected_steps = 11;
  
  btk_text_buffer_get_start_iter (buffer, &iter);
  i = 0;
  do
    {
      int pos;

      pos = btk_text_iter_get_offset (&iter);
      
      if (pos != expected[i])
        {
          g_error ("Cursor position %d, expected %d",
                   pos, expected[i]);
        }

      ++i;      
    }
  while (btk_text_iter_forward_cursor_position (&iter));

  if (!btk_text_iter_is_end (&iter))
    g_error ("Expected to stop at the end iterator\n");

  if (!btk_text_iter_is_cursor_position (&iter))
    g_error ("Should be a cursor position before the end iterator");
  
  if (i != expected_steps)
    g_error ("Expected %d steps, there were actually %d\n", expected_steps, i);

  i = expected_steps;
  do
    {
      int pos;

      pos = btk_text_iter_get_offset (&iter);
      
      if (pos != expected[i])
        {
          g_error ("Moving backward, cursor position %d, expected %d",
                   pos, expected[i]);
        }

      /* g_print ("%d = %d\n", pos, expected[i]); */
      
      --i;
    }
  while (btk_text_iter_backward_cursor_position (&iter));

  if (i != -1)
    g_error ("Expected %d steps, there were actually %d\n", expected_steps - i, i);

  if (!btk_text_iter_is_start (&iter))
    g_error ("Expected to stop at the start iterator\n");


  /* Check sentence boundaries */
  
  btk_text_buffer_set_text (buffer, "Hi.\nHi. \nHi! Hi. Hi? Hi.", -1);

  memset (expected, 0, sizeof (expected));

  expected[0] = 0;    /* before first Hi */
  expected[1] = 3;    /* After first . */
  expected[2] = 7;    /* After second . */
  expected[3] = 12;   /* After ! */
  expected[4] = 16;   /* After third . */
  expected[5] = 20;   /* After ? */
  
  expected_steps = 6;
  
  btk_text_buffer_get_start_iter (buffer, &iter);
  i = 0;
  do
    {
      int pos;

      pos = btk_text_iter_get_offset (&iter);

      if (pos != expected[i])
        {
          g_error ("Sentence position %d, expected %d",
                   pos, expected[i]);
        }

      if (i != 0 &&
          !btk_text_iter_is_end (&iter) &&
          !btk_text_iter_ends_sentence (&iter))
        g_error ("Iterator at %d should end a sentence", pos);
      
      ++i;
    }
  while (btk_text_iter_forward_sentence_end (&iter));

  if (i != expected_steps)
    g_error ("Expected %d steps, there were actually %d\n", expected_steps, i);

  if (!btk_text_iter_is_end (&iter))
    g_error ("Expected to stop at the end iterator\n");
  
  btk_text_buffer_set_text (buffer, "Hi.\nHi. \nHi! Hi. Hi? Hi.", -1);

  memset (expected, 0, sizeof (expected));

  expected[0] = 24;
  expected[1] = 21;
  expected[2] = 17;
  expected[3] = 13;
  expected[4] = 9;
  expected[5] = 4;
  expected[6] = 0;
  
  expected_steps = 7;
  
  btk_text_buffer_get_end_iter (buffer, &iter);
  i = 0;
  do
    {
      int pos;

      pos = btk_text_iter_get_offset (&iter);

      if (pos != expected[i])
        {
          g_error ("Sentence position %d, expected %d",
                   pos, expected[i]);
        }

      if (pos != 0 &&
          !btk_text_iter_is_end (&iter) &&
          !btk_text_iter_starts_sentence (&iter))
        g_error ("Iterator at %d should start a sentence", pos);
      
      ++i;
    }
  while (btk_text_iter_backward_sentence_start (&iter));

  if (i != expected_steps)
    g_error ("Expected %d steps, there were actually %d\n", expected_steps, i);

  if (btk_text_iter_get_offset (&iter) != 0)
    g_error ("Expected to stop at the start iterator\n");
  
  g_object_unref (buffer);
}

static void
test_marks (void)
{
  BtkTextBuffer *buf1, *buf2;
  BtkTextMark *mark;
  BtkTextIter iter;

  buf1 = btk_text_buffer_new (NULL);
  buf2 = btk_text_buffer_new (NULL);

  btk_text_buffer_get_start_iter (buf1, &iter);
  mark = btk_text_buffer_create_mark (buf1, "foo", &iter, TRUE);
  g_object_ref (mark);
  btk_text_mark_set_visible (mark, TRUE);
  btk_text_buffer_delete_mark (buf1, mark);

  g_assert (btk_text_mark_get_visible (mark));
  g_assert (btk_text_mark_get_left_gravity (mark));
  g_assert (!strcmp ("foo", btk_text_mark_get_name (mark)));
  g_assert (btk_text_mark_get_buffer (mark) == NULL);
  g_assert (btk_text_mark_get_deleted (mark));
  g_assert (btk_text_buffer_get_mark (buf1, "foo") == NULL);

  btk_text_buffer_get_start_iter (buf2, &iter);
  btk_text_buffer_add_mark (buf2, mark, &iter);
  btk_text_buffer_insert (buf2, &iter, "ewfwefwefwe", -1);
  btk_text_buffer_get_iter_at_mark (buf2, &iter, mark);

  g_assert (btk_text_mark_get_visible (mark));
  g_assert (btk_text_iter_is_start (&iter));
  g_assert (btk_text_mark_get_left_gravity (mark));
  g_assert (!strcmp ("foo", btk_text_mark_get_name (mark)));
  g_assert (btk_text_mark_get_buffer (mark) == buf2);
  g_assert (!btk_text_mark_get_deleted (mark));
  g_assert (btk_text_buffer_get_mark (buf2, "foo") == mark);

  btk_text_buffer_delete_mark (buf2, mark);
  btk_text_mark_set_visible (mark, FALSE);
  g_object_unref (mark);

  mark = btk_text_mark_new ("blah", TRUE);
  btk_text_buffer_get_start_iter (buf1, &iter);
  btk_text_mark_set_visible (mark, TRUE);
  btk_text_buffer_add_mark (buf1, mark, &iter);

  g_assert (btk_text_mark_get_visible (mark));
  g_assert (btk_text_mark_get_buffer (mark) == buf1);
  g_assert (!btk_text_mark_get_deleted (mark));
  g_assert (btk_text_buffer_get_mark (buf1, "blah") == mark);
  g_assert (!strcmp ("blah", btk_text_mark_get_name (mark)));

  btk_text_mark_set_visible (mark, FALSE);
  btk_text_buffer_delete_mark (buf1, mark);
  g_assert (!btk_text_mark_get_visible (mark));
  g_assert (btk_text_buffer_get_mark (buf1, "blah") == NULL);
  g_assert (btk_text_mark_get_buffer (mark) == NULL);
  g_assert (btk_text_mark_get_deleted (mark));

  btk_text_buffer_get_start_iter (buf2, &iter);
  btk_text_buffer_add_mark (buf2, mark, &iter);
  g_assert (btk_text_mark_get_buffer (mark) == buf2);
  g_assert (!btk_text_mark_get_deleted (mark));
  g_assert (btk_text_buffer_get_mark (buf2, "blah") == mark);
  g_assert (!strcmp ("blah", btk_text_mark_get_name (mark)));

  g_object_unref (mark);
  g_object_unref (buf1);
  g_object_unref (buf2);
}

static void
test_utf8 (void)
{
  gunichar ch;

  /* Check UTF8 unknown char thing */
  g_assert (g_utf8_strlen (btk_text_unknown_char_utf8, 3) == 1);
  ch = g_utf8_get_char (btk_text_unknown_char_utf8);
  g_assert (ch == BTK_TEXT_UNKNOWN_CHAR);
}

static void
test_empty_buffer (void)
{
  BtkTextBuffer *buffer;
  int n;
  BtkTextIter start;

  buffer = btk_text_buffer_new (NULL);

  /* Check that buffer starts with one empty line and zero chars */
  n = btk_text_buffer_get_line_count (buffer);
  if (n != 1)
    g_error ("%d lines, expected 1", n);

  n = btk_text_buffer_get_char_count (buffer);
  if (n != 0)
    g_error ("%d chars, expected 0", n);

  /* empty first line contains 0 chars */
  btk_text_buffer_get_start_iter (buffer, &start);
  n = btk_text_iter_get_chars_in_line (&start);
  if (n != 0)
    g_error ("%d chars in first line, expected 0", n);
  n = btk_text_iter_get_bytes_in_line (&start);
  if (n != 0)
    g_error ("%d bytes in first line, expected 0", n);
  
  /* Run gruesome alien test suite on buffer */
  run_tests (buffer);

  g_object_unref (buffer);
}

static void
test_get_set(void)
{
  BtkTextBuffer *buffer;

  buffer = btk_text_buffer_new (NULL);

  check_get_set_text (buffer, "Hello");
  check_get_set_text (buffer, "Hello\n");
  check_get_set_text (buffer, "Hello\r\n");
  check_get_set_text (buffer, "Hello\r");
  check_get_set_text (buffer, "Hello\nBar\nFoo");
  check_get_set_text (buffer, "Hello\nBar\nFoo\n");

  g_object_unref (buffer);
}

static void
test_fill_empty (void)
{
  BtkTextBuffer *buffer;
  int n;
  BtkTextIter start, end;
  
  buffer = btk_text_buffer_new (NULL);

  /* Put stuff in the buffer */
  fill_buffer (buffer);

  /* Subject stuff-bloated buffer to further torment */
  run_tests (buffer);

  /* Delete all stuff from the buffer */
  btk_text_buffer_get_bounds (buffer, &start, &end);
  btk_text_buffer_delete (buffer, &start, &end);

  /* Check buffer for emptiness (note that a single
     empty line always remains in the buffer) */
  n = btk_text_buffer_get_line_count (buffer);
  if (n != 1)
    g_error ("%d lines, expected 1", n);

  n = btk_text_buffer_get_char_count (buffer);
  if (n != 0)
    g_error ("%d chars, expected 0", n);

  run_tests (buffer);

  g_object_unref (buffer);
}

static void
test_tag (void)
{
  BtkTextBuffer *buffer;
  BtkTextIter start, end;
  
  buffer = btk_text_buffer_new (NULL);

  fill_buffer (buffer);

  btk_text_buffer_set_text (buffer, "adcdef", -1);
  btk_text_buffer_get_iter_at_offset (buffer, &start, 1);
  btk_text_buffer_get_iter_at_offset (buffer, &end, 3);
  btk_text_buffer_apply_tag_by_name (buffer, "fg_blue", &start, &end);
  
  run_tests (buffer);
  
  g_object_unref (buffer);
}

extern void pixbuf_init (void);

int
main (int argc, char** argv)
{
  /* First, we turn on btree debugging. */
  btk_debug_flags |= BTK_DEBUG_TEXT;

  btk_test_init (&argc, &argv);
  pixbuf_init ();

  g_test_add_func ("/TextBuffer/UTF8 unknown char", test_utf8);
  g_test_add_func ("/TextBuffer/Line separator", test_line_separator);
  g_test_add_func ("/TextBuffer/Backspace", test_backspace);
  g_test_add_func ("/TextBuffer/Logical motion", test_logical_motion);
  g_test_add_func ("/TextBuffer/Marks", test_marks);
  g_test_add_func ("/TextBuffer/Empty buffer", test_empty_buffer);
  g_test_add_func ("/TextBuffer/Get and Set", test_get_set);
  g_test_add_func ("/TextBuffer/Fill and Empty", test_fill_empty);
  g_test_add_func ("/TextBuffer/Tag", test_tag);
  
  return g_test_run();
}
