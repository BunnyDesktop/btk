/* BTK - The GIMP Toolkit
 * btktextlayout.h
 *
 * Copyright (c) 1992-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 2000 Red Hat, Inc.
 * Tk->Btk port by Havoc Pennington
 * Bango support by Owen Taylor
 *
 * This file can be used under your choice of two licenses, the LGPL
 * and the original Tk license.
 *
 * LGPL:
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Original Tk license:
 *
 * This software is copyrighted by the Regents of the University of
 * California, Sun Microsystems, Inc., and other parties.  The
 * following terms apply to all files associated with the software
 * unless explicitly disclaimed in individual files.
 *
 * The authors hereby grant permission to use, copy, modify,
 * distribute, and license this software and its documentation for any
 * purpose, provided that existing copyright notices are retained in
 * all copies and that this notice is included verbatim in any
 * distributions. No written agreement, license, or royalty fee is
 * required for any of the authorized uses.  Modifications to this
 * software may be copyrighted by their authors and need not follow
 * the licensing terms described here, provided that the new terms are
 * clearly indicated on the first page of each file where they apply.
 *
 * IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY
 * PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
 * DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION,
 * OR ANY DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
 * NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS,
 * AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
 * MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * GOVERNMENT USE: If you are acquiring this software on behalf of the
 * U.S. government, the Government shall have only "Restricted Rights"
 * in the software and related documentation as defined in the Federal
 * Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 * are acquiring the software on behalf of the Department of Defense,
 * the software shall be classified as "Commercial Computer Software"
 * and the Government shall have only "Restricted Rights" as defined
 * in Clause 252.227-7013 (c) (1) of DFARs.  Notwithstanding the
 * foregoing, the authors grant the U.S. Government and others acting
 * in its behalf permission to use and distribute the software in
 * accordance with the terms specified in this license.
 *
 */
/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BTK_TEXT_LAYOUT_H__
#define __BTK_TEXT_LAYOUT_H__

/* This is a "semi-private" header; it is intended for
 * use by the text widget, and the text canvas item,
 * but that's all. We may have to install it so the
 * canvas item can use it, but users are not supposed
 * to use it.
 */
#ifndef BTK_TEXT_USE_INTERNAL_UNSUPPORTED_API
#error "You are not supposed to be including this file; the equivalent public API is in btktextview.h"
#endif

#include <btk/btk.h>

B_BEGIN_DECLS

/* forward declarations that have to be here to avoid including
 * btktextbtree.h
 */
typedef struct _BtkTextLine     BtkTextLine;
typedef struct _BtkTextLineData BtkTextLineData;

#define BTK_TYPE_TEXT_LAYOUT             (btk_text_layout_get_type ())
#define BTK_TEXT_LAYOUT(obj)             (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TEXT_LAYOUT, BtkTextLayout))
#define BTK_TEXT_LAYOUT_CLASS(klass)     (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TEXT_LAYOUT, BtkTextLayoutClass))
#define BTK_IS_TEXT_LAYOUT(obj)          (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TEXT_LAYOUT))
#define BTK_IS_TEXT_LAYOUT_CLASS(klass)  (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TEXT_LAYOUT))
#define BTK_TEXT_LAYOUT_GET_CLASS(obj)   (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TEXT_LAYOUT, BtkTextLayoutClass))

typedef struct _BtkTextLayout         BtkTextLayout;
typedef struct _BtkTextLayoutClass    BtkTextLayoutClass;
typedef struct _BtkTextLineDisplay    BtkTextLineDisplay;
typedef struct _BtkTextCursorDisplay  BtkTextCursorDisplay;
typedef struct _BtkTextAttrAppearance BtkTextAttrAppearance;

struct _BtkTextLayout
{
  BObject parent_instance;

  /* width of the display area on-screen,
   * i.e. pixels we should wrap to fit inside. */
  gint screen_width;

  /* width/height of the total logical area being layed out */
  gint width;
  gint height;

  /* Pixel offsets from the left and from the top to be used when we
   * draw; these allow us to create left/top margins. We don't need
   * anything special for bottom/right margins, because those don't
   * affect drawing.
   */
  /* gint left_edge; */
  /* gint top_edge; */

  BtkTextBuffer *buffer;

  /* Default style used if no tags override it */
  BtkTextAttributes *default_style;

  /* Bango contexts used for creating layouts */
  BangoContext *ltr_context;
  BangoContext *rtl_context;

  /* A cache of one style; this is used to ensure
   * we don't constantly regenerate the style
   * over long runs with the same style. */
  BtkTextAttributes *one_style_cache;

  /* A cache of one line display. Getting the same line
   * many times in a row is the most common case.
   */
  BtkTextLineDisplay *one_display_cache;

  /* Whether we are allowed to wrap right now */
  gint wrap_loop_count;
  
  /* Whether to show the insertion cursor */
  guint cursor_visible : 1;

  /* For what BtkTextDirection to draw cursor BTK_TEXT_DIR_NONE -
   * means draw both cursors.
   */
  guint cursor_direction : 2;

  /* The keyboard direction is used to default the alignment when
     there are no strong characters.
  */
  guint keyboard_direction : 2;

  /* The preedit string and attributes, if any */

  gchar *preedit_string;
  BangoAttrList *preedit_attrs;
  gint preedit_len;
  gint preedit_cursor;

  guint overwrite_mode : 1;
};

struct _BtkTextLayoutClass
{
  BObjectClass parent_class;

  /* Some portion of the layout was invalidated
   */
  void  (*invalidated)  (BtkTextLayout *layout);

  /* A range of the layout changed appearance and possibly height
   */
  void  (*changed)              (BtkTextLayout     *layout,
                                 gint               y,
                                 gint               old_height,
                                 gint               new_height);
  BtkTextLineData* (*wrap)      (BtkTextLayout     *layout,
                                 BtkTextLine       *line,
                                 BtkTextLineData   *line_data); /* may be NULL */
  void  (*get_log_attrs)        (BtkTextLayout     *layout,
                                 BtkTextLine       *line,
                                 BangoLogAttr     **attrs,
                                 gint              *n_attrs);
  void  (*invalidate)           (BtkTextLayout     *layout,
                                 const BtkTextIter *start,
                                 const BtkTextIter *end);
  void  (*free_line_data)       (BtkTextLayout     *layout,
                                 BtkTextLine       *line,
                                 BtkTextLineData   *line_data);

  void (*allocate_child)        (BtkTextLayout     *layout,
                                 BtkWidget         *child,
                                 gint               x,
                                 gint               y);

  void (*invalidate_cursors)    (BtkTextLayout     *layout,
                                 const BtkTextIter *start,
                                 const BtkTextIter *end);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
};

struct _BtkTextAttrAppearance
{
  BangoAttribute attr;
  BtkTextAppearance appearance;
};
struct _BtkTextCursorDisplay
{
  gint x;
  gint y;
  gint height;
  guint is_strong : 1;
  guint is_weak : 1;
};
struct _BtkTextLineDisplay
{
  BangoLayout *layout;
  GSList *cursors;
  GSList *shaped_objects;	/* Only for backwards compatibility */
  
  BtkTextDirection direction;

  gint width;                   /* Width of layout */
  gint total_width;             /* width - margins, if no width set on layout, if width set on layout, -1 */
  gint height;
  /* Amount layout is shifted from left edge - this is the left margin
   * plus any other factors, such as alignment or indentation.
   */
  gint x_offset;
  gint left_margin;
  gint right_margin;
  gint top_margin;
  gint bottom_margin;
  gint insert_index;		/* Byte index of insert cursor within para or -1 */

  gboolean size_only;
  BtkTextLine *line;
  
  BdkColor *pg_bg_color;

  BdkRectangle block_cursor;
  guint cursors_invalid : 1;
  guint has_block_cursor : 1;
  guint cursor_at_line_end : 1;
};

extern BangoAttrType btk_text_attr_appearance_type;

GType         btk_text_layout_get_type    (void) B_GNUC_CONST;

BtkTextLayout*     btk_text_layout_new                   (void);
void               btk_text_layout_set_buffer            (BtkTextLayout     *layout,
							  BtkTextBuffer     *buffer);
BtkTextBuffer     *btk_text_layout_get_buffer            (BtkTextLayout     *layout);
void               btk_text_layout_set_default_style     (BtkTextLayout     *layout,
							  BtkTextAttributes *values);
void               btk_text_layout_set_contexts          (BtkTextLayout     *layout,
							  BangoContext      *ltr_context,
							  BangoContext      *rtl_context);
void               btk_text_layout_set_cursor_direction  (BtkTextLayout     *layout,
                                                          BtkTextDirection   direction);
void		   btk_text_layout_set_overwrite_mode	 (BtkTextLayout     *layout,
							  gboolean           overwrite);
void               btk_text_layout_set_keyboard_direction (BtkTextLayout     *layout,
							   BtkTextDirection keyboard_dir);
void               btk_text_layout_default_style_changed (BtkTextLayout     *layout);

void btk_text_layout_set_screen_width       (BtkTextLayout     *layout,
                                             gint               width);
void btk_text_layout_set_preedit_string     (BtkTextLayout     *layout,
 					     const gchar       *preedit_string,
 					     BangoAttrList     *preedit_attrs,
 					     gint               cursor_pos);

void     btk_text_layout_set_cursor_visible (BtkTextLayout     *layout,
                                             gboolean           cursor_visible);
gboolean btk_text_layout_get_cursor_visible (BtkTextLayout     *layout);

/* Getting the size or the lines potentially results in a call to
 * recompute, which is pretty massively expensive. Thus it should
 * basically only be done in an idle handler.
 *
 * Long-term, we would really like to be able to do these without
 * a full recompute so they may get cheaper over time.
 */
void    btk_text_layout_get_size  (BtkTextLayout  *layout,
                                   gint           *width,
                                   gint           *height);
GSList* btk_text_layout_get_lines (BtkTextLayout  *layout,
                                   /* [top_y, bottom_y) */
                                   gint            top_y,
                                   gint            bottom_y,
                                   gint           *first_line_y);

void btk_text_layout_wrap_loop_start (BtkTextLayout *layout);
void btk_text_layout_wrap_loop_end   (BtkTextLayout *layout);

BtkTextLineDisplay* btk_text_layout_get_line_display  (BtkTextLayout      *layout,
                                                       BtkTextLine        *line,
                                                       gboolean            size_only);
void                btk_text_layout_free_line_display (BtkTextLayout      *layout,
                                                       BtkTextLineDisplay *display);

void btk_text_layout_get_line_at_y     (BtkTextLayout     *layout,
                                        BtkTextIter       *target_iter,
                                        gint               y,
                                        gint              *line_top);
void btk_text_layout_get_iter_at_pixel (BtkTextLayout     *layout,
                                        BtkTextIter       *iter,
                                        gint               x,
                                        gint               y);
void btk_text_layout_get_iter_at_position (BtkTextLayout     *layout,
					   BtkTextIter       *iter,
					   gint              *trailing,
					   gint               x,
					   gint               y);
void btk_text_layout_invalidate        (BtkTextLayout     *layout,
                                        const BtkTextIter *start,
                                        const BtkTextIter *end);
void btk_text_layout_invalidate_cursors(BtkTextLayout     *layout,
                                        const BtkTextIter *start,
                                        const BtkTextIter *end);
void btk_text_layout_free_line_data    (BtkTextLayout     *layout,
                                        BtkTextLine       *line,
                                        BtkTextLineData   *line_data);

gboolean btk_text_layout_is_valid        (BtkTextLayout *layout);
void     btk_text_layout_validate_yrange (BtkTextLayout *layout,
                                          BtkTextIter   *anchor_line,
                                          gint           y0_,
                                          gint           y1_);
void     btk_text_layout_validate        (BtkTextLayout *layout,
                                          gint           max_pixels);

/* This function should return the passed-in line data,
 * OR remove the existing line data from the line, and
 * return a NEW line data after adding it to the line.
 * That is, invariant after calling the callback is that
 * there should be exactly one line data for this view
 * stored on the btree line.
 */
BtkTextLineData* btk_text_layout_wrap  (BtkTextLayout   *layout,
                                        BtkTextLine     *line,
                                        BtkTextLineData *line_data); /* may be NULL */
void     btk_text_layout_changed              (BtkTextLayout     *layout,
                                               gint               y,
                                               gint               old_height,
                                               gint               new_height);
void     btk_text_layout_cursors_changed      (BtkTextLayout     *layout,
                                               gint               y,
                                               gint               old_height,
                                               gint               new_height);
void     btk_text_layout_get_iter_location    (BtkTextLayout     *layout,
                                               const BtkTextIter *iter,
                                               BdkRectangle      *rect);
void     btk_text_layout_get_line_yrange      (BtkTextLayout     *layout,
                                               const BtkTextIter *iter,
                                               gint              *y,
                                               gint              *height);
void     _btk_text_layout_get_line_xrange     (BtkTextLayout     *layout,
                                               const BtkTextIter *iter,
                                               gint              *x,
                                               gint              *width);
void     btk_text_layout_get_cursor_locations (BtkTextLayout     *layout,
                                               BtkTextIter       *iter,
                                               BdkRectangle      *strong_pos,
                                               BdkRectangle      *weak_pos);
gboolean _btk_text_layout_get_block_cursor    (BtkTextLayout     *layout,
					       BdkRectangle      *pos);
gboolean btk_text_layout_clamp_iter_to_vrange (BtkTextLayout     *layout,
                                               BtkTextIter       *iter,
                                               gint               top,
                                               gint               bottom);

gboolean btk_text_layout_move_iter_to_line_end      (BtkTextLayout *layout,
                                                     BtkTextIter   *iter,
                                                     gint           direction);
gboolean btk_text_layout_move_iter_to_previous_line (BtkTextLayout *layout,
                                                     BtkTextIter   *iter);
gboolean btk_text_layout_move_iter_to_next_line     (BtkTextLayout *layout,
                                                     BtkTextIter   *iter);
void     btk_text_layout_move_iter_to_x             (BtkTextLayout *layout,
                                                     BtkTextIter   *iter,
                                                     gint           x);
gboolean btk_text_layout_move_iter_visually         (BtkTextLayout *layout,
                                                     BtkTextIter   *iter,
                                                     gint           count);

gboolean btk_text_layout_iter_starts_line           (BtkTextLayout       *layout,
                                                     const BtkTextIter   *iter);

void     btk_text_layout_get_iter_at_line           (BtkTextLayout *layout,
                                                     BtkTextIter    *iter,
                                                     BtkTextLine    *line,
                                                     gint            byte_offset);

/* Don't use these. Use btk_text_view_add_child_at_anchor().
 * These functions are defined in btktextchild.c, but here
 * since they are semi-public and require BtkTextLayout to
 * be declared.
 */
void btk_text_child_anchor_register_child   (BtkTextChildAnchor *anchor,
                                             BtkWidget          *child,
                                             BtkTextLayout      *layout);
void btk_text_child_anchor_unregister_child (BtkTextChildAnchor *anchor,
                                             BtkWidget          *child);

void btk_text_child_anchor_queue_resize     (BtkTextChildAnchor *anchor,
                                             BtkTextLayout      *layout);

void btk_text_anchored_child_set_layout     (BtkWidget          *child,
                                             BtkTextLayout      *layout);

void btk_text_layout_spew (BtkTextLayout *layout);

B_END_DECLS

#endif  /* __BTK_TEXT_LAYOUT_H__ */
