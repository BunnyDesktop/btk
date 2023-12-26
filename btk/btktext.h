/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

#ifdef BTK_ENABLE_BROKEN

#ifndef __BTK_TEXT_H__
#define __BTK_TEXT_H__


#include <btk/btkoldeditable.h>


B_BEGIN_DECLS

#define BTK_TYPE_TEXT                  (btk_text_get_type ())
#define BTK_TEXT(obj)                  (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TEXT, BtkText))
#define BTK_TEXT_CLASS(klass)          (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TEXT, BtkTextClass))
#define BTK_IS_TEXT(obj)               (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TEXT))
#define BTK_IS_TEXT_CLASS(klass)       (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TEXT))
#define BTK_TEXT_GET_CLASS(obj)        (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TEXT, BtkTextClass))


typedef struct _BtkTextFont       BtkTextFont;
typedef struct _BtkPropertyMark   BtkPropertyMark;
typedef struct _BtkText           BtkText;
typedef struct _BtkTextClass      BtkTextClass;

struct _BtkPropertyMark
{
  /* Position in list. */
  GList* property;

  /* Offset into that property. */
  buint offset;

  /* Current index. */
  buint index;
};

struct _BtkText
{
  BtkOldEditable old_editable;

  BdkWindow *text_area;

  BtkAdjustment *hadj;
  BtkAdjustment *vadj;

  BdkGC *gc;

  BdkPixmap* line_wrap_bitmap;
  BdkPixmap* line_arrow_bitmap;

		      /* GAPPED TEXT SEGMENT */

  /* The text, a single segment of text a'la emacs, with a gap
   * where insertion occurs. */
  union { BdkWChar *wc; buchar  *ch; } text;
  /* The allocated length of the text segment. */
  buint text_len;
  /* The gap position, index into address where a char
   * should be inserted. */
  buint gap_position;
  /* The gap size, s.t. *(text + gap_position + gap_size) is
   * the first valid character following the gap. */
  buint gap_size;
  /* The last character position, index into address where a
   * character should be appeneded.  Thus, text_end - gap_size
   * is the length of the actual data. */
  buint text_end;
			/* LINE START CACHE */

  /* A cache of line-start information.  Data is a LineParam*. */
  GList *line_start_cache;
  /* Index to the start of the first visible line. */
  buint first_line_start_index;
  /* The number of pixels cut off of the top line. */
  buint first_cut_pixels;
  /* First visible horizontal pixel. */
  buint first_onscreen_hor_pixel;
  /* First visible vertical pixel. */
  buint first_onscreen_ver_pixel;

			     /* FLAGS */

  /* True iff this buffer is wrapping lines, otherwise it is using a
   * horizontal scrollbar. */
  buint line_wrap : 1;
  buint word_wrap : 1;
 /* If a fontset is supplied for the widget, use_wchar become true,
   * and we use BdkWchar as the encoding of text. */
  buint use_wchar : 1;

  /* Frozen, don't do updates. @@@ fixme */
  buint freeze_count;
			/* TEXT PROPERTIES */

  /* A doubly-linked-list containing TextProperty objects. */
  GList *text_properties;
  /* The end of this list. */
  GList *text_properties_end;
  /* The first node before or on the point along with its offset to
   * the point and the buffer's current point.  This is the only
   * PropertyMark whose index is guaranteed to remain correct
   * following a buffer insertion or deletion. */
  BtkPropertyMark point;

			  /* SCRATCH AREA */

  union { BdkWChar *wc; buchar *ch; } scratch_buffer;
  buint   scratch_buffer_len;

			   /* SCROLLING */

  bint last_ver_value;

			     /* CURSOR */

  bint            cursor_pos_x;       /* Position of cursor. */
  bint            cursor_pos_y;       /* Baseline of line cursor is drawn on. */
  BtkPropertyMark cursor_mark;        /* Where it is in the buffer. */
  BdkWChar        cursor_char;        /* Character to redraw. */
  bchar           cursor_char_offset; /* Distance from baseline of the font. */
  bint            cursor_virtual_x;   /* Where it would be if it could be. */
  bint            cursor_drawn_level; /* How many people have undrawn. */

			  /* Current Line */

  GList *current_line;

			   /* Tab Stops */

  GList *tab_stops;
  bint default_tab_width;

  BtkTextFont *current_font;	/* Text font for current style */

  /* Timer used for auto-scrolling off ends */
  bint timer;
  
  buint button;			/* currently pressed mouse button */
  BdkGC *bg_gc;			/* gc for drawing background pixmap */
};

struct _BtkTextClass
{
  BtkOldEditableClass parent_class;

  void  (*set_scroll_adjustments)   (BtkText	    *text,
				     BtkAdjustment  *hadjustment,
				     BtkAdjustment  *vadjustment);
};


GType      btk_text_get_type        (void) B_GNUC_CONST;
BtkWidget* btk_text_new             (BtkAdjustment *hadj,
				     BtkAdjustment *vadj);
void       btk_text_set_editable    (BtkText       *text,
				     bboolean       editable);
void       btk_text_set_word_wrap   (BtkText       *text,
				     bboolean       word_wrap);
void       btk_text_set_line_wrap   (BtkText       *text,
				     bboolean       line_wrap);
void       btk_text_set_adjustments (BtkText       *text,
				     BtkAdjustment *hadj,
				     BtkAdjustment *vadj);
void       btk_text_set_point       (BtkText       *text,
				     buint          index);
buint      btk_text_get_point       (BtkText       *text);
buint      btk_text_get_length      (BtkText       *text);
void       btk_text_freeze          (BtkText       *text);
void       btk_text_thaw            (BtkText       *text);
void       btk_text_insert          (BtkText        *text,
				     BdkFont        *font,
				     const BdkColor *fore,
				     const BdkColor *back,
				     const char     *chars,
				     bint            length);
bboolean   btk_text_backward_delete (BtkText       *text,
				     buint          nchars);
bboolean   btk_text_forward_delete  (BtkText       *text,
				     buint          nchars);

#define BTK_TEXT_INDEX(t, index)	(((t)->use_wchar) \
	? ((index) < (t)->gap_position ? (t)->text.wc[index] : \
					(t)->text.wc[(index)+(t)->gap_size]) \
	: ((index) < (t)->gap_position ? (t)->text.ch[index] : \
					(t)->text.ch[(index)+(t)->gap_size]))

B_END_DECLS

#endif /* __BTK_TEXT_H__ */

#endif /* BTK_ENABLE_BROKEN */
