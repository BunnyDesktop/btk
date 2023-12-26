/* BTK - The GIMP Toolkit
 * btktextbtree.h Copyright (C) 2000 Red Hat, Inc.
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

#ifndef __BTK_TEXT_BTREE_H__
#define __BTK_TEXT_BTREE_H__

#if 0
#define DEBUG_VALIDATION_AND_SCROLLING
#endif

#ifdef DEBUG_VALIDATION_AND_SCROLLING
#define DV(x) (x)
#else
#define DV(x)
#endif

#include <btk/btktextbuffer.h>
#include <btk/btktexttag.h>
#include <btk/btktextmark.h>
#include <btk/btktextchild.h>
#include <btk/btktextsegment.h>
#include <btk/btktextiter.h>

B_BEGIN_DECLS

BtkTextBTree  *_btk_text_btree_new        (BtkTextTagTable *table,
                                           BtkTextBuffer   *buffer);
void           _btk_text_btree_ref        (BtkTextBTree    *tree);
void           _btk_text_btree_unref      (BtkTextBTree    *tree);
BtkTextBuffer *_btk_text_btree_get_buffer (BtkTextBTree    *tree);


guint _btk_text_btree_get_chars_changed_stamp    (BtkTextBTree *tree);
guint _btk_text_btree_get_segments_changed_stamp (BtkTextBTree *tree);
void  _btk_text_btree_segments_changed           (BtkTextBTree *tree);

gboolean _btk_text_btree_is_end (BtkTextBTree       *tree,
                                 BtkTextLine        *line,
                                 BtkTextLineSegment *seg,
                                 int                 byte_index,
                                 int                 char_offset);

/* Indexable segment mutation */

void _btk_text_btree_delete        (BtkTextIter *start,
                                    BtkTextIter *end);
void _btk_text_btree_insert        (BtkTextIter *iter,
                                    const gchar *text,
                                    gint         len);
void _btk_text_btree_insert_pixbuf (BtkTextIter *iter,
                                    BdkPixbuf   *pixbuf);

void _btk_text_btree_insert_child_anchor (BtkTextIter        *iter,
                                          BtkTextChildAnchor *anchor);

void _btk_text_btree_unregister_child_anchor (BtkTextChildAnchor *anchor);

/* View stuff */
BtkTextLine *_btk_text_btree_find_line_by_y    (BtkTextBTree      *tree,
                                                gpointer           view_id,
                                                gint               ypixel,
                                                gint              *line_top_y);
gint         _btk_text_btree_find_line_top     (BtkTextBTree      *tree,
                                                BtkTextLine       *line,
                                                gpointer           view_id);
void         _btk_text_btree_add_view          (BtkTextBTree      *tree,
                                                BtkTextLayout     *layout);
void         _btk_text_btree_remove_view       (BtkTextBTree      *tree,
                                                gpointer           view_id);
void         _btk_text_btree_invalidate_rebunnyion (BtkTextBTree      *tree,
                                                const BtkTextIter *start,
                                                const BtkTextIter *end,
                                                gboolean           cursors_only);
void         _btk_text_btree_get_view_size     (BtkTextBTree      *tree,
                                                gpointer           view_id,
                                                gint              *width,
                                                gint              *height);
gboolean     _btk_text_btree_is_valid          (BtkTextBTree      *tree,
                                                gpointer           view_id);
gboolean     _btk_text_btree_validate          (BtkTextBTree      *tree,
                                                gpointer           view_id,
                                                gint               max_pixels,
                                                gint              *y,
                                                gint              *old_height,
                                                gint              *new_height);
void         _btk_text_btree_validate_line     (BtkTextBTree      *tree,
                                                BtkTextLine       *line,
                                                gpointer           view_id);

/* Tag */

void _btk_text_btree_tag (const BtkTextIter *start,
                          const BtkTextIter *end,
                          BtkTextTag        *tag,
                          gboolean           apply);

/* "Getters" */

BtkTextLine * _btk_text_btree_get_line          (BtkTextBTree      *tree,
                                                 gint               line_number,
                                                 gint              *real_line_number);
BtkTextLine * _btk_text_btree_get_line_no_last  (BtkTextBTree      *tree,
                                                 gint               line_number,
                                                 gint              *real_line_number);
BtkTextLine * _btk_text_btree_get_end_iter_line (BtkTextBTree      *tree);
BtkTextLine * _btk_text_btree_get_line_at_char  (BtkTextBTree      *tree,
                                                 gint               char_index,
                                                 gint              *line_start_index,
                                                 gint              *real_char_index);
BtkTextTag**  _btk_text_btree_get_tags          (const BtkTextIter *iter,
                                                 gint              *num_tags);
gchar        *_btk_text_btree_get_text          (const BtkTextIter *start,
                                                 const BtkTextIter *end,
                                                 gboolean           include_hidden,
                                                 gboolean           include_nonchars);
gint          _btk_text_btree_line_count        (BtkTextBTree      *tree);
gint          _btk_text_btree_char_count        (BtkTextBTree      *tree);
gboolean      _btk_text_btree_char_is_invisible (const BtkTextIter *iter);



/* Get iterators (these are implemented in btktextiter.c) */
void     _btk_text_btree_get_iter_at_char         (BtkTextBTree       *tree,
                                                   BtkTextIter        *iter,
                                                   gint                char_index);
void     _btk_text_btree_get_iter_at_line_char    (BtkTextBTree       *tree,
                                                   BtkTextIter        *iter,
                                                   gint                line_number,
                                                   gint                char_index);
void     _btk_text_btree_get_iter_at_line_byte    (BtkTextBTree       *tree,
                                                   BtkTextIter        *iter,
                                                   gint                line_number,
                                                   gint                byte_index);
gboolean _btk_text_btree_get_iter_from_string     (BtkTextBTree       *tree,
                                                   BtkTextIter        *iter,
                                                   const gchar        *string);
gboolean _btk_text_btree_get_iter_at_mark_name    (BtkTextBTree       *tree,
                                                   BtkTextIter        *iter,
                                                   const gchar        *mark_name);
void     _btk_text_btree_get_iter_at_mark         (BtkTextBTree       *tree,
                                                   BtkTextIter        *iter,
                                                   BtkTextMark        *mark);
void     _btk_text_btree_get_end_iter             (BtkTextBTree       *tree,
                                                   BtkTextIter        *iter);
void     _btk_text_btree_get_iter_at_line         (BtkTextBTree       *tree,
                                                   BtkTextIter        *iter,
                                                   BtkTextLine        *line,
                                                   gint                byte_offset);
gboolean _btk_text_btree_get_iter_at_first_toggle (BtkTextBTree       *tree,
                                                   BtkTextIter        *iter,
                                                   BtkTextTag         *tag);
gboolean _btk_text_btree_get_iter_at_last_toggle  (BtkTextBTree       *tree,
                                                   BtkTextIter        *iter,
                                                   BtkTextTag         *tag);

void     _btk_text_btree_get_iter_at_child_anchor  (BtkTextBTree       *tree,
                                                    BtkTextIter        *iter,
                                                    BtkTextChildAnchor *anchor);



/* Manipulate marks */
BtkTextMark        *_btk_text_btree_set_mark                (BtkTextBTree       *tree,
                                                             BtkTextMark         *existing_mark,
                                                             const gchar        *name,
                                                             gboolean            left_gravity,
                                                             const BtkTextIter  *index,
                                                             gboolean           should_exist);
void                _btk_text_btree_remove_mark_by_name     (BtkTextBTree       *tree,
                                                             const gchar        *name);
void                _btk_text_btree_remove_mark             (BtkTextBTree       *tree,
                                                             BtkTextMark        *segment);
gboolean            _btk_text_btree_get_selection_bounds    (BtkTextBTree       *tree,
                                                             BtkTextIter        *start,
                                                             BtkTextIter        *end);
void                _btk_text_btree_place_cursor            (BtkTextBTree       *tree,
                                                             const BtkTextIter  *where);
void                _btk_text_btree_select_range            (BtkTextBTree       *tree,
                                                             const BtkTextIter  *ins,
							     const BtkTextIter  *bound);
gboolean            _btk_text_btree_mark_is_insert          (BtkTextBTree       *tree,
                                                             BtkTextMark        *segment);
gboolean            _btk_text_btree_mark_is_selection_bound (BtkTextBTree       *tree,
                                                             BtkTextMark        *segment);
BtkTextMark        *_btk_text_btree_get_insert		    (BtkTextBTree       *tree);
BtkTextMark        *_btk_text_btree_get_selection_bound	    (BtkTextBTree       *tree);
BtkTextMark        *_btk_text_btree_get_mark_by_name        (BtkTextBTree       *tree,
                                                             const gchar        *name);
BtkTextLine *       _btk_text_btree_first_could_contain_tag (BtkTextBTree       *tree,
                                                             BtkTextTag         *tag);
BtkTextLine *       _btk_text_btree_last_could_contain_tag  (BtkTextBTree       *tree,
                                                             BtkTextTag         *tag);

/* Lines */

/* Chunk of data associated with a line; views can use this to store
   info at the line. They should "subclass" the header struct here. */
struct _BtkTextLineData {
  gpointer view_id;
  BtkTextLineData *next;
  gint height;
  signed int width : 24;
  guint valid : 8;		/* Actually a boolean */
};

/*
 * The data structure below defines a single line of text (from newline
 * to newline, not necessarily what appears on one line of the screen).
 *
 * You can consider this line a "paragraph" also
 */

struct _BtkTextLine {
  BtkTextBTreeNode *parent;             /* Pointer to parent node containing
                                         * line. */
  BtkTextLine *next;            /* Next in linked list of lines with
                                 * same parent node in B-tree.  NULL
                                 * means end of list. */
  BtkTextLineSegment *segments; /* First in ordered list of segments
                                 * that make up the line. */
  BtkTextLineData *views;      /* data stored here by views */
  guchar dir_strong;                /* BiDi algo dir of line */
  guchar dir_propagated_back;       /* BiDi algo dir of next line */
  guchar dir_propagated_forward;    /* BiDi algo dir of prev line */
};


gint                _btk_text_line_get_number                 (BtkTextLine         *line);
gboolean            _btk_text_line_char_has_tag               (BtkTextLine         *line,
                                                               BtkTextBTree        *tree,
                                                               gint                 char_in_line,
                                                               BtkTextTag          *tag);
gboolean            _btk_text_line_byte_has_tag               (BtkTextLine         *line,
                                                               BtkTextBTree        *tree,
                                                               gint                 byte_in_line,
                                                               BtkTextTag          *tag);
gboolean            _btk_text_line_is_last                    (BtkTextLine         *line,
                                                               BtkTextBTree        *tree);
gboolean            _btk_text_line_contains_end_iter          (BtkTextLine         *line,
                                                               BtkTextBTree        *tree);
BtkTextLine *       _btk_text_line_next                       (BtkTextLine         *line);
BtkTextLine *       _btk_text_line_next_excluding_last        (BtkTextLine         *line);
BtkTextLine *       _btk_text_line_previous                   (BtkTextLine         *line);
void                _btk_text_line_add_data                   (BtkTextLine         *line,
                                                               BtkTextLineData     *data);
gpointer            _btk_text_line_remove_data                (BtkTextLine         *line,
                                                               gpointer             view_id);
gpointer            _btk_text_line_get_data                   (BtkTextLine         *line,
                                                               gpointer             view_id);
void                _btk_text_line_invalidate_wrap            (BtkTextLine         *line,
                                                               BtkTextLineData     *ld);
gint                _btk_text_line_char_count                 (BtkTextLine         *line);
gint                _btk_text_line_byte_count                 (BtkTextLine         *line);
gint                _btk_text_line_char_index                 (BtkTextLine         *line);
BtkTextLineSegment *_btk_text_line_byte_to_segment            (BtkTextLine         *line,
                                                               gint                 byte_offset,
                                                               gint                *seg_offset);
BtkTextLineSegment *_btk_text_line_char_to_segment            (BtkTextLine         *line,
                                                               gint                 char_offset,
                                                               gint                *seg_offset);
gboolean            _btk_text_line_byte_locate                (BtkTextLine         *line,
                                                               gint                 byte_offset,
                                                               BtkTextLineSegment **segment,
                                                               BtkTextLineSegment **any_segment,
                                                               gint                *seg_byte_offset,
                                                               gint                *line_byte_offset);
gboolean            _btk_text_line_char_locate                (BtkTextLine         *line,
                                                               gint                 char_offset,
                                                               BtkTextLineSegment **segment,
                                                               BtkTextLineSegment **any_segment,
                                                               gint                *seg_char_offset,
                                                               gint                *line_char_offset);
void                _btk_text_line_byte_to_char_offsets       (BtkTextLine         *line,
                                                               gint                 byte_offset,
                                                               gint                *line_char_offset,
                                                               gint                *seg_char_offset);
void                _btk_text_line_char_to_byte_offsets       (BtkTextLine         *line,
                                                               gint                 char_offset,
                                                               gint                *line_byte_offset,
                                                               gint                *seg_byte_offset);
BtkTextLineSegment *_btk_text_line_byte_to_any_segment        (BtkTextLine         *line,
                                                               gint                 byte_offset,
                                                               gint                *seg_offset);
BtkTextLineSegment *_btk_text_line_char_to_any_segment        (BtkTextLine         *line,
                                                               gint                 char_offset,
                                                               gint                *seg_offset);
gint                _btk_text_line_byte_to_char               (BtkTextLine         *line,
                                                               gint                 byte_offset);
gint                _btk_text_line_char_to_byte               (BtkTextLine         *line,
                                                               gint                 char_offset);
BtkTextLine    *    _btk_text_line_next_could_contain_tag     (BtkTextLine         *line,
                                                               BtkTextBTree        *tree,
                                                               BtkTextTag          *tag);
BtkTextLine    *    _btk_text_line_previous_could_contain_tag (BtkTextLine         *line,
                                                               BtkTextBTree        *tree,
                                                               BtkTextTag          *tag);

BtkTextLineData    *_btk_text_line_data_new                   (BtkTextLayout     *layout,
                                                               BtkTextLine       *line);

/* Debug */
void _btk_text_btree_check (BtkTextBTree *tree);
void _btk_text_btree_spew (BtkTextBTree *tree);
extern gboolean _btk_text_view_debug_btree;

/* ignore, exported only for btktextsegment.c */
void _btk_toggle_segment_check_func (BtkTextLineSegment *segPtr,
                                     BtkTextLine        *line);
void _btk_change_node_toggle_count  (BtkTextBTreeNode   *node,
                                     BtkTextTagInfo     *info,
                                     gint                delta);

/* for btktextmark.c */
void _btk_text_btree_release_mark_segment (BtkTextBTree       *tree,
                                           BtkTextLineSegment *segment);

/* for coordination with the tag table */
void _btk_text_btree_notify_will_remove_tag (BtkTextBTree *tree,
                                             BtkTextTag   *tag);

B_END_DECLS

#endif


