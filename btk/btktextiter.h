/* BTK - The GIMP Toolkit
 * btktextiter.h Copyright (C) 2000 Red Hat, Inc.
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

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BTK_TEXT_ITER_H__
#define __BTK_TEXT_ITER_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btktexttag.h>
#include <btk/btktextchild.h>

G_BEGIN_DECLS

typedef enum {
  BTK_TEXT_SEARCH_VISIBLE_ONLY = 1 << 0,
  BTK_TEXT_SEARCH_TEXT_ONLY    = 1 << 1
  /* Possible future plans: SEARCH_CASE_INSENSITIVE, SEARCH_REGEXP */
} BtkTextSearchFlags;

/*
 * Iter: represents a location in the text. Becomes invalid if the
 * characters/pixmaps/widgets (indexable objects) in the text buffer
 * are changed.
 */

typedef struct _BtkTextBuffer BtkTextBuffer;

#define BTK_TYPE_TEXT_ITER     (btk_text_iter_get_type ())

struct _BtkTextIter {
  /* BtkTextIter is an opaque datatype; ignore all these fields.
   * Initialize the iter with btk_text_buffer_get_iter_*
   * functions
   */
  /*< private >*/
  gpointer dummy1;
  gpointer dummy2;
  gint dummy3;
  gint dummy4;
  gint dummy5;
  gint dummy6;
  gint dummy7;
  gint dummy8;
  gpointer dummy9;
  gpointer dummy10;
  gint dummy11;
  gint dummy12;
  /* padding */
  gint dummy13;
  gpointer dummy14;
};


/* This is primarily intended for language bindings that want to avoid
   a "buffer" argument to text insertions, deletions, etc. */
BtkTextBuffer *btk_text_iter_get_buffer (const BtkTextIter *iter);

/*
 * Life cycle
 */

BtkTextIter *btk_text_iter_copy     (const BtkTextIter *iter);
void         btk_text_iter_free     (BtkTextIter       *iter);

GType        btk_text_iter_get_type (void) G_GNUC_CONST;

/*
 * Convert to different kinds of index
 */

gint btk_text_iter_get_offset      (const BtkTextIter *iter);
gint btk_text_iter_get_line        (const BtkTextIter *iter);
gint btk_text_iter_get_line_offset (const BtkTextIter *iter);
gint btk_text_iter_get_line_index  (const BtkTextIter *iter);

gint btk_text_iter_get_visible_line_offset (const BtkTextIter *iter);
gint btk_text_iter_get_visible_line_index (const BtkTextIter *iter);


/*
 * "Dereference" operators
 */
gunichar btk_text_iter_get_char          (const BtkTextIter  *iter);

/* includes the 0xFFFC char for pixmaps/widgets, so char offsets
 * into the returned string map properly into buffer char offsets
 */
gchar   *btk_text_iter_get_slice         (const BtkTextIter  *start,
                                          const BtkTextIter  *end);

/* includes only text, no 0xFFFC */
gchar   *btk_text_iter_get_text          (const BtkTextIter  *start,
                                          const BtkTextIter  *end);
/* exclude invisible chars */
gchar   *btk_text_iter_get_visible_slice (const BtkTextIter  *start,
                                          const BtkTextIter  *end);
gchar   *btk_text_iter_get_visible_text  (const BtkTextIter  *start,
                                          const BtkTextIter  *end);

BdkPixbuf* btk_text_iter_get_pixbuf (const BtkTextIter *iter);
GSList  *  btk_text_iter_get_marks  (const BtkTextIter *iter);

BtkTextChildAnchor* btk_text_iter_get_child_anchor (const BtkTextIter *iter);

/* Return list of tags toggled at this point (toggled_on determines
 * whether the list is of on-toggles or off-toggles)
 */
GSList  *btk_text_iter_get_toggled_tags  (const BtkTextIter  *iter,
                                          gboolean            toggled_on);

gboolean btk_text_iter_begins_tag        (const BtkTextIter  *iter,
                                          BtkTextTag         *tag);

gboolean btk_text_iter_ends_tag          (const BtkTextIter  *iter,
                                          BtkTextTag         *tag);

gboolean btk_text_iter_toggles_tag       (const BtkTextIter  *iter,
                                          BtkTextTag         *tag);

gboolean btk_text_iter_has_tag           (const BtkTextIter   *iter,
                                          BtkTextTag          *tag);
GSList  *btk_text_iter_get_tags          (const BtkTextIter   *iter);

gboolean btk_text_iter_editable          (const BtkTextIter   *iter,
                                          gboolean             default_setting);
gboolean btk_text_iter_can_insert        (const BtkTextIter   *iter,
                                          gboolean             default_editability);

gboolean btk_text_iter_starts_word        (const BtkTextIter   *iter);
gboolean btk_text_iter_ends_word          (const BtkTextIter   *iter);
gboolean btk_text_iter_inside_word        (const BtkTextIter   *iter);
gboolean btk_text_iter_starts_sentence    (const BtkTextIter   *iter);
gboolean btk_text_iter_ends_sentence      (const BtkTextIter   *iter);
gboolean btk_text_iter_inside_sentence    (const BtkTextIter   *iter);
gboolean btk_text_iter_starts_line        (const BtkTextIter   *iter);
gboolean btk_text_iter_ends_line          (const BtkTextIter   *iter);
gboolean btk_text_iter_is_cursor_position (const BtkTextIter   *iter);

gint     btk_text_iter_get_chars_in_line (const BtkTextIter   *iter);
gint     btk_text_iter_get_bytes_in_line (const BtkTextIter   *iter);

gboolean       btk_text_iter_get_attributes (const BtkTextIter *iter,
					     BtkTextAttributes *values);
BangoLanguage* btk_text_iter_get_language   (const BtkTextIter *iter);
gboolean       btk_text_iter_is_end         (const BtkTextIter *iter);
gboolean       btk_text_iter_is_start       (const BtkTextIter *iter);

/*
 * Moving around the buffer
 */

gboolean btk_text_iter_forward_char         (BtkTextIter *iter);
gboolean btk_text_iter_backward_char        (BtkTextIter *iter);
gboolean btk_text_iter_forward_chars        (BtkTextIter *iter,
                                             gint         count);
gboolean btk_text_iter_backward_chars       (BtkTextIter *iter,
                                             gint         count);
gboolean btk_text_iter_forward_line         (BtkTextIter *iter);
gboolean btk_text_iter_backward_line        (BtkTextIter *iter);
gboolean btk_text_iter_forward_lines        (BtkTextIter *iter,
                                             gint         count);
gboolean btk_text_iter_backward_lines       (BtkTextIter *iter,
                                             gint         count);
gboolean btk_text_iter_forward_word_end     (BtkTextIter *iter);
gboolean btk_text_iter_backward_word_start  (BtkTextIter *iter);
gboolean btk_text_iter_forward_word_ends    (BtkTextIter *iter,
                                             gint         count);
gboolean btk_text_iter_backward_word_starts (BtkTextIter *iter,
                                             gint         count);
                                             
gboolean btk_text_iter_forward_visible_line   (BtkTextIter *iter);
gboolean btk_text_iter_backward_visible_line  (BtkTextIter *iter);
gboolean btk_text_iter_forward_visible_lines  (BtkTextIter *iter,
                                               gint         count);
gboolean btk_text_iter_backward_visible_lines (BtkTextIter *iter,
                                               gint         count);

gboolean btk_text_iter_forward_visible_word_end     (BtkTextIter *iter);
gboolean btk_text_iter_backward_visible_word_start  (BtkTextIter *iter);
gboolean btk_text_iter_forward_visible_word_ends    (BtkTextIter *iter,
                                             gint         count);
gboolean btk_text_iter_backward_visible_word_starts (BtkTextIter *iter,
                                             gint         count);

gboolean btk_text_iter_forward_sentence_end     (BtkTextIter *iter);
gboolean btk_text_iter_backward_sentence_start  (BtkTextIter *iter);
gboolean btk_text_iter_forward_sentence_ends    (BtkTextIter *iter,
                                                 gint         count);
gboolean btk_text_iter_backward_sentence_starts (BtkTextIter *iter,
                                                 gint         count);
/* cursor positions are almost equivalent to chars, but not quite;
 * in some languages, you can't put the cursor between certain
 * chars. Also, you can't put the cursor between \r\n at the end
 * of a line.
 */
gboolean btk_text_iter_forward_cursor_position   (BtkTextIter *iter);
gboolean btk_text_iter_backward_cursor_position  (BtkTextIter *iter);
gboolean btk_text_iter_forward_cursor_positions  (BtkTextIter *iter,
                                                  gint         count);
gboolean btk_text_iter_backward_cursor_positions (BtkTextIter *iter,
                                                  gint         count);

gboolean btk_text_iter_forward_visible_cursor_position   (BtkTextIter *iter);
gboolean btk_text_iter_backward_visible_cursor_position  (BtkTextIter *iter);
gboolean btk_text_iter_forward_visible_cursor_positions  (BtkTextIter *iter,
                                                          gint         count);
gboolean btk_text_iter_backward_visible_cursor_positions (BtkTextIter *iter,
                                                          gint         count);


void     btk_text_iter_set_offset         (BtkTextIter *iter,
                                           gint         char_offset);
void     btk_text_iter_set_line           (BtkTextIter *iter,
                                           gint         line_number);
void     btk_text_iter_set_line_offset    (BtkTextIter *iter,
                                           gint         char_on_line);
void     btk_text_iter_set_line_index     (BtkTextIter *iter,
                                           gint         byte_on_line);
void     btk_text_iter_forward_to_end     (BtkTextIter *iter);
gboolean btk_text_iter_forward_to_line_end (BtkTextIter *iter);

void     btk_text_iter_set_visible_line_offset (BtkTextIter *iter,
                                                gint         char_on_line);
void     btk_text_iter_set_visible_line_index  (BtkTextIter *iter,
                                                gint         byte_on_line);

/* returns TRUE if a toggle was found; NULL for the tag pointer
 * means "any tag toggle", otherwise the next toggle of the
 * specified tag is located.
 */
gboolean btk_text_iter_forward_to_tag_toggle (BtkTextIter *iter,
                                              BtkTextTag  *tag);

gboolean btk_text_iter_backward_to_tag_toggle (BtkTextIter *iter,
                                               BtkTextTag  *tag);

typedef gboolean (* BtkTextCharPredicate) (gunichar ch, gpointer user_data);

gboolean btk_text_iter_forward_find_char  (BtkTextIter          *iter,
                                           BtkTextCharPredicate  pred,
                                           gpointer              user_data,
                                           const BtkTextIter    *limit);
gboolean btk_text_iter_backward_find_char (BtkTextIter          *iter,
                                           BtkTextCharPredicate  pred,
                                           gpointer              user_data,
                                           const BtkTextIter    *limit);

gboolean btk_text_iter_forward_search  (const BtkTextIter *iter,
                                        const gchar       *str,
                                        BtkTextSearchFlags flags,
                                        BtkTextIter       *match_start,
                                        BtkTextIter       *match_end,
                                        const BtkTextIter *limit);

gboolean btk_text_iter_backward_search (const BtkTextIter *iter,
                                        const gchar       *str,
                                        BtkTextSearchFlags flags,
                                        BtkTextIter       *match_start,
                                        BtkTextIter       *match_end,
                                        const BtkTextIter *limit);


/*
 * Comparisons
 */
gboolean btk_text_iter_equal           (const BtkTextIter *lhs,
                                        const BtkTextIter *rhs);
gint     btk_text_iter_compare         (const BtkTextIter *lhs,
                                        const BtkTextIter *rhs);
gboolean btk_text_iter_in_range        (const BtkTextIter *iter,
                                        const BtkTextIter *start,
                                        const BtkTextIter *end);

/* Put these two in ascending order */
void     btk_text_iter_order           (BtkTextIter *first,
                                        BtkTextIter *second);

G_END_DECLS

#endif


