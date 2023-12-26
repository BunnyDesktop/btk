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


G_BEGIN_DECLS

#define BTK_TYPE_TEXT                  (btk_text_get_type ())
#define BTK_TEXT(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TEXT, BtkText))
#define BTK_TEXT_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TEXT, BtkTextClass))
#define BTK_IS_TEXT(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TEXT))
#define BTK_IS_TEXT_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TEXT))
#define BTK_TEXT_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TEXT, BtkTextClass))


typedef struct _BtkTextFont       BtkTextFont;
typedef struct _BtkPropertyMark   BtkPropertyMark;
typedef struct _BtkText           BtkText;
typedef struct _BtkTextClass      BtkTextClass;

struct _BtkPropertyMark
{
  /* Position in list. */
  GList* property;

  /* Offset into that property. */
  guint offset;

  /* Current index. */
  guint index;
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
  union { BdkWChar *wc; guchar  *ch; } text;
  /* The allocated length of the text segment. */
  guint text_len;
  /* The gap position, index into address where a char
   * should be inserted. */
  guint gap_position;
  /* The gap size, s.t. *(text + gap_position + gap_size) is
   * the first valid character following the gap. */
  guint gap_size;
  /* The last character position, index into address where a
   * character should be appeneded.  Thus, text_end - gap_size
   * is the length of the actual data. */
  guint text_end;
			/* LINE START CACHE */

  /* A cache of line-start information.  Data is a LineParam*. */
  GList *line_start_cache;
  /* Index to the start of the first visible line. */
  guint first_line_start_index;
  /* The number of pixels cut off of the top line. */
  guint first_cut_pixels;
  /* First visible horizontal pixel. */
  guint first_onscreen_hor_pixel;
  /* First visible vertical pixel. */
  guint first_onscreen_ver_pixel;

			     /* FLAGS */

  /* True iff this buffer is wrapping lines, otherwise it is using a
   * horizontal scrollbar. */
  guint line_wrap : 1;
  guint word_wrap : 1;
 /* If a fontset is supplied for the widget, use_wchar become true,
   * and we use BdkWchar as the encoding of text. */
  guint use_wchar : 1;

  /* Frozen, don't do updates. @@@ fixme */
  guint freeze_count;
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

  union { BdkWChar *wc; guchar *ch; } scratch_buffer;
  guint   scratch_buffer_len;

			   /* SCROLLING */

  gint last_ver_value;

			     /* CURSOR */

  gint            cursor_pos_x;       /* Position of cursor. */
  gint            cursor_pos_y;       /* Baseline of line cursor is drawn on. */
  BtkPropertyMark cursor_mark;        /* Where it is in the buffer. */
  BdkWChar        cursor_char;        /* Character to redraw. */
  gchar           cursor_char_offset; /* Distance from baseline of the font. */
  gint            cursor_virtual_x;   /* Where it would be if it could be. */
  gint            cursor_drawn_level; /* How many people have undrawn. */

			  /* Current Line */

  GList *current_line;

			   /* Tab Stops */

  GList *tab_stops;
  gint default_tab_width;

  BtkTextFont *current_font;	/* Text font for current style */

  /* Timer used for auto-scrolling off ends */
  gint timer;
  
  guint button;			/* currently pressed mouse button */
  BdkGC *bg_gc;			/* gc for drawing background pixmap */
};

struct _BtkTextClass
{
  BtkOldEditableClass parent_class;

  void  (*set_scroll_adjustments)   (BtkText	    *text,
				     BtkAdjustment  *hadjustment,
				     BtkAdjustment  *vadjustment);
};


GType      btk_text_get_type        (void) G_GNUC_CONST;
BtkWidget* btk_text_new             (BtkAdjustment *hadj,
				     BtkAdjustment *vadj);
void       btk_text_set_editable    (BtkText       *text,
				     gboolean       editable);
void       btk_text_set_word_wrap   (BtkText       *text,
				     gboolean       word_wrap);
void       btk_text_set_line_wrap   (BtkText       *text,
				     gboolean       line_wrap);
void       btk_text_set_adjustments (BtkText       *text,
				     BtkAdjustment *hadj,
				     BtkAdjustment *vadj);
void       btk_text_set_point       (BtkText       *text,
				     guint          index);
guint      btk_text_get_point       (BtkText       *text);
guint      btk_text_get_length      (BtkText       *text);
void       btk_text_freeze          (BtkText       *text);
void       btk_text_thaw            (BtkText       *text);
void       btk_text_insert          (BtkText        *text,
				     BdkFont        *font,
				     const BdkColor *fore,
				     const BdkColor *back,
				     const char     *chars,
				     gint            length);
gboolean   btk_text_backward_delete (BtkText       *text,
				     guint          nchars);
gboolean   btk_text_forward_delete  (BtkText       *text,
				     guint          nchars);

#define BTK_TEXT_INDEX(t, index)	(((t)->use_wchar) \
	? ((index) < (t)->gap_position ? (t)->text.wc[index] : \
					(t)->text.wc[(index)+(t)->gap_size]) \
	: ((index) < (t)->gap_position ? (t)->text.ch[index] : \
					(t)->text.ch[(index)+(t)->gap_size]))

G_END_DECLS

#endif /* __BTK_TEXT_H__ */

#endif /* BTK_ENABLE_BROKEN */
