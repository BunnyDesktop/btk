/* BTK - The GIMP Toolkit
 * btktextlayout.c - calculate the layout of the text
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

#define BTK_TEXT_USE_INTERNAL_UNSUPPORTED_API
#include "config.h"
#include "btkmarshalers.h"
#include "btktextlayout.h"
#include "btktextbtree.h"
#include "btktextiterprivate.h"
#include "btktextutil.h"
#include "btkintl.h"
#include "btkalias.h"

#include <stdlib.h>
#include <string.h>

#define BTK_TEXT_LAYOUT_GET_PRIVATE(o)  (B_TYPE_INSTANCE_GET_PRIVATE ((o), BTK_TYPE_TEXT_LAYOUT, BtkTextLayoutPrivate))

typedef struct _BtkTextLayoutPrivate BtkTextLayoutPrivate;

struct _BtkTextLayoutPrivate
{
  /* Cache the line that the cursor is positioned on, as the keyboard
     direction only influences the direction of the cursor line.
  */
  BtkTextLine *cursor_line;
};

static BtkTextLineData *btk_text_layout_real_wrap (BtkTextLayout *layout,
                                                   BtkTextLine *line,
                                                   /* may be NULL */
                                                   BtkTextLineData *line_data);

static void btk_text_layout_invalidated     (BtkTextLayout     *layout);

static void btk_text_layout_real_invalidate        (BtkTextLayout     *layout,
						    const BtkTextIter *start,
						    const BtkTextIter *end);
static void btk_text_layout_real_invalidate_cursors(BtkTextLayout     *layout,
						    const BtkTextIter *start,
						    const BtkTextIter *end);
static void btk_text_layout_invalidate_cache       (BtkTextLayout     *layout,
						    BtkTextLine       *line,
						    gboolean           cursors_only);
static void btk_text_layout_invalidate_cursor_line (BtkTextLayout     *layout,
						    gboolean           cursors_only);
static void btk_text_layout_real_free_line_data    (BtkTextLayout     *layout,
						    BtkTextLine       *line,
						    BtkTextLineData   *line_data);
static void btk_text_layout_emit_changed           (BtkTextLayout     *layout,
						    gint               y,
						    gint               old_height,
						    gint               new_height);

static void btk_text_layout_invalidate_all (BtkTextLayout *layout);

static BangoAttribute *btk_text_attr_appearance_new (const BtkTextAppearance *appearance);

static void btk_text_layout_mark_set_handler    (BtkTextBuffer     *buffer,
						 const BtkTextIter *location,
						 BtkTextMark       *mark,
						 gpointer           data);
static void btk_text_layout_buffer_insert_text  (BtkTextBuffer     *textbuffer,
						 BtkTextIter       *iter,
						 gchar             *str,
						 gint               len,
						 gpointer           data);
static void btk_text_layout_buffer_delete_range (BtkTextBuffer     *textbuffer,
						 BtkTextIter       *start,
						 BtkTextIter       *end,
						 gpointer           data);

static void btk_text_layout_update_cursor_line (BtkTextLayout *layout);

static void line_display_index_to_iter (BtkTextLayout      *layout,
	                                BtkTextLineDisplay *display,
			                BtkTextIter        *iter,
                                        gint                index,
                                        gint                trailing);

static gint line_display_iter_to_index (BtkTextLayout      *layout,
                                        BtkTextLineDisplay *display,
                                        const BtkTextIter  *iter);

enum {
  INVALIDATED,
  CHANGED,
  ALLOCATE_CHILD,
  LAST_SIGNAL
};

enum {
  ARG_0,
  LAST_ARG
};

#define PIXEL_BOUND(d) (((d) + BANGO_SCALE - 1) / BANGO_SCALE)

static void btk_text_layout_finalize (BObject *object);

static guint signals[LAST_SIGNAL] = { 0 };

BangoAttrType btk_text_attr_appearance_type = 0;

G_DEFINE_TYPE (BtkTextLayout, btk_text_layout, B_TYPE_OBJECT)

static void
btk_text_layout_class_init (BtkTextLayoutClass *klass)
{
  BObjectClass *object_class = B_OBJECT_CLASS (klass);

  object_class->finalize = btk_text_layout_finalize;

  klass->wrap = btk_text_layout_real_wrap;
  klass->invalidate = btk_text_layout_real_invalidate;
  klass->invalidate_cursors = btk_text_layout_real_invalidate_cursors;
  klass->free_line_data = btk_text_layout_real_free_line_data;

  signals[INVALIDATED] =
    g_signal_new (I_("invalidated"),
                  B_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkTextLayoutClass, invalidated),
                  NULL, NULL,
                  _btk_marshal_VOID__VOID,
                  B_TYPE_NONE,
                  0);

  signals[CHANGED] =
    g_signal_new (I_("changed"),
                  B_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkTextLayoutClass, changed),
                  NULL, NULL,
                  _btk_marshal_VOID__INT_INT_INT,
                  B_TYPE_NONE,
                  3,
                  B_TYPE_INT,
                  B_TYPE_INT,
                  B_TYPE_INT);

  signals[ALLOCATE_CHILD] =
    g_signal_new (I_("allocate-child"),
                  B_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkTextLayoutClass, allocate_child),
                  NULL, NULL,
                  _btk_marshal_VOID__OBJECT_INT_INT,
                  B_TYPE_NONE,
                  3,
                  BTK_TYPE_OBJECT,
                  B_TYPE_INT,
                  B_TYPE_INT);
  
  g_type_class_add_private (object_class, sizeof (BtkTextLayoutPrivate));
}

static void
btk_text_layout_init (BtkTextLayout *text_layout)
{
  text_layout->cursor_visible = TRUE;
}

BtkTextLayout*
btk_text_layout_new (void)
{
  return g_object_new (BTK_TYPE_TEXT_LAYOUT, NULL);
}

static void
free_style_cache (BtkTextLayout *text_layout)
{
  if (text_layout->one_style_cache)
    {
      btk_text_attributes_unref (text_layout->one_style_cache);
      text_layout->one_style_cache = NULL;
    }
}

static void
btk_text_layout_finalize (BObject *object)
{
  BtkTextLayout *layout;

  layout = BTK_TEXT_LAYOUT (object);

  btk_text_layout_set_buffer (layout, NULL);

  if (layout->default_style)
    btk_text_attributes_unref (layout->default_style);
  layout->default_style = NULL;

  if (layout->ltr_context)
    {
      g_object_unref (layout->ltr_context);
      layout->ltr_context = NULL;
    }
  if (layout->rtl_context)
    {
      g_object_unref (layout->rtl_context);
      layout->rtl_context = NULL;
    }
  
  if (layout->one_display_cache) 
    {
      BtkTextLineDisplay *tmp_display = layout->one_display_cache;
      layout->one_display_cache = NULL;
      btk_text_layout_free_line_display (layout, tmp_display);
    }

  if (layout->preedit_string)
    {
      g_free (layout->preedit_string);
      layout->preedit_string = NULL;
    }

  if (layout->preedit_attrs)
    {
      bango_attr_list_unref (layout->preedit_attrs);
      layout->preedit_attrs = NULL;
    }


  B_OBJECT_CLASS (btk_text_layout_parent_class)->finalize (object);
}

/**
 * btk_text_layout_set_buffer:
 * @buffer: (allow-none):
 */
void
btk_text_layout_set_buffer (BtkTextLayout *layout,
                            BtkTextBuffer *buffer)
{
  g_return_if_fail (BTK_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (buffer == NULL || BTK_IS_TEXT_BUFFER (buffer));

  if (layout->buffer == buffer)
    return;

  free_style_cache (layout);

  if (layout->buffer)
    {
      _btk_text_btree_remove_view (_btk_text_buffer_get_btree (layout->buffer),
                                  layout);

      g_signal_handlers_disconnect_by_func (layout->buffer, 
                                            G_CALLBACK (btk_text_layout_mark_set_handler), 
                                            layout);
      g_signal_handlers_disconnect_by_func (layout->buffer, 
                                            G_CALLBACK (btk_text_layout_buffer_insert_text), 
                                            layout);
      g_signal_handlers_disconnect_by_func (layout->buffer, 
                                            G_CALLBACK (btk_text_layout_buffer_delete_range), 
                                            layout);

      g_object_unref (layout->buffer);
      layout->buffer = NULL;
    }

  if (buffer)
    {
      layout->buffer = buffer;

      g_object_ref (buffer);

      _btk_text_btree_add_view (_btk_text_buffer_get_btree (buffer), layout);

      /* Bind to all signals that move the insert mark. */
      g_signal_connect_after (layout->buffer, "mark-set",
                              G_CALLBACK (btk_text_layout_mark_set_handler), layout);
      g_signal_connect_after (layout->buffer, "insert-text",
                              G_CALLBACK (btk_text_layout_buffer_insert_text), layout);
      g_signal_connect_after (layout->buffer, "delete-range",
                              G_CALLBACK (btk_text_layout_buffer_delete_range), layout);

      btk_text_layout_update_cursor_line (layout);
    }
}

void
btk_text_layout_default_style_changed (BtkTextLayout *layout)
{
  g_return_if_fail (BTK_IS_TEXT_LAYOUT (layout));

  DV (g_print ("invalidating all due to default style change (%s)\n", B_STRLOC));
  btk_text_layout_invalidate_all (layout);
}

void
btk_text_layout_set_default_style (BtkTextLayout *layout,
                                   BtkTextAttributes *values)
{
  g_return_if_fail (BTK_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (values != NULL);

  if (values == layout->default_style)
    return;

  btk_text_attributes_ref (values);

  if (layout->default_style)
    btk_text_attributes_unref (layout->default_style);

  layout->default_style = values;

  btk_text_layout_default_style_changed (layout);
}

void
btk_text_layout_set_contexts (BtkTextLayout *layout,
                              BangoContext  *ltr_context,
                              BangoContext  *rtl_context)
{
  g_return_if_fail (BTK_IS_TEXT_LAYOUT (layout));

  if (layout->ltr_context != ltr_context)
    {
      if (layout->ltr_context)
	g_object_unref (layout->ltr_context);

      layout->ltr_context = ltr_context;
      g_object_ref (layout->ltr_context);
    }

  if (layout->rtl_context != rtl_context)
    {
      if (layout->rtl_context)
	g_object_unref (layout->rtl_context);

      layout->rtl_context = rtl_context;
      g_object_ref (layout->rtl_context);
    }

  DV (g_print ("invalidating all due to new bango contexts (%s)\n", B_STRLOC));
  btk_text_layout_invalidate_all (layout);
}

/**
 * btk_text_layout_set_overwrite_mode:
 * @layout: a #BtkTextLayout
 * @overwrite: overwrite mode
 *
 * Sets overwrite mode
 */
void
btk_text_layout_set_overwrite_mode (BtkTextLayout *layout,
				    gboolean       overwrite)
{
  overwrite = overwrite != 0;
  if (overwrite != layout->overwrite_mode)
    {
      layout->overwrite_mode = overwrite;
      btk_text_layout_invalidate_cursor_line (layout, TRUE);
    }
}

/**
 * btk_text_layout_set_cursor_direction:
 * @direction: the new direction(s) for which to draw cursors.
 *             %BTK_TEXT_DIR_NONE means draw cursors for both
 *             left-to-right insertion and right-to-left insertion.
 *             (The two cursors will be visually distinguished.)
 * 
 * Sets which text directions (left-to-right and/or right-to-left) for
 * which cursors will be drawn for the insertion point. The visual
 * point at which new text is inserted depends on whether the new
 * text is right-to-left or left-to-right, so it may be desired to
 * make the drawn position of the cursor depend on the keyboard state.
 */
void
btk_text_layout_set_cursor_direction (BtkTextLayout   *layout,
				      BtkTextDirection direction)
{
  if (direction != layout->cursor_direction)
    {
      layout->cursor_direction = direction;
      btk_text_layout_invalidate_cursor_line (layout, TRUE);
    }
}

/**
 * btk_text_layout_set_keyboard_direction:
 * @keyboard_dir: the current direction of the keyboard.
 *
 * Sets the keyboard direction; this is used as for the bidirectional
 * base direction for the line with the cursor if the line contains
 * only neutral characters.
 */
void
btk_text_layout_set_keyboard_direction (BtkTextLayout   *layout,
					BtkTextDirection keyboard_dir)
{
  if (keyboard_dir != layout->keyboard_direction)
    {
      layout->keyboard_direction = keyboard_dir;
      btk_text_layout_invalidate_cursor_line (layout, TRUE);
    }
}

/**
 * btk_text_layout_get_buffer:
 * @layout: a #BtkTextLayout
 *
 * Gets the text buffer used by the layout. See
 * btk_text_layout_set_buffer().
 *
 * Return value: the text buffer used by the layout.
 */
BtkTextBuffer *
btk_text_layout_get_buffer (BtkTextLayout *layout)
{
  g_return_val_if_fail (BTK_IS_TEXT_LAYOUT (layout), NULL);

  return layout->buffer;
}

void
btk_text_layout_set_screen_width (BtkTextLayout *layout, gint width)
{
  g_return_if_fail (BTK_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (width >= 0);
  g_return_if_fail (layout->wrap_loop_count == 0);

  if (layout->screen_width == width)
    return;

  layout->screen_width = width;

  DV (g_print ("invalidating all due to new screen width (%s)\n", B_STRLOC));
  btk_text_layout_invalidate_all (layout);
}

/**
 * btk_text_layout_set_cursor_visible:
 * @layout: a #BtkTextLayout
 * @cursor_visible: If %FALSE, then the insertion cursor will not
 *   be shown, even if the text is editable.
 *
 * Sets whether the insertion cursor should be shown. Generally,
 * widgets using #BtkTextLayout will hide the cursor when the
 * widget does not have the input focus.
 */
void
btk_text_layout_set_cursor_visible (BtkTextLayout *layout,
                                    gboolean       cursor_visible)
{
  cursor_visible = (cursor_visible != FALSE);

  if (layout->cursor_visible != cursor_visible)
    {
      BtkTextIter iter;
      gint y, height;

      layout->cursor_visible = cursor_visible;

      /* Now queue a redraw on the paragraph containing the cursor
       */
      btk_text_buffer_get_iter_at_mark (layout->buffer, &iter,
                                        btk_text_buffer_get_insert (layout->buffer));

      btk_text_layout_get_line_yrange (layout, &iter, &y, &height);
      btk_text_layout_emit_changed (layout, y, height, height);

      btk_text_layout_invalidate_cache (layout, _btk_text_iter_get_text_line (&iter), TRUE);
    }
}

/**
 * btk_text_layout_get_cursor_visible:
 * @layout: a #BtkTextLayout
 *
 * Returns whether the insertion cursor will be shown.
 *
 * Return value: if %FALSE, the insertion cursor will not be
 *     shown, even if the text is editable.
 */
gboolean
btk_text_layout_get_cursor_visible (BtkTextLayout *layout)
{
  return layout->cursor_visible;
}

/**
 * btk_text_layout_set_preedit_string:
 * @layout: a #BangoLayout
 * @preedit_string: a string to display at the insertion point
 * @preedit_attrs: a #BangoAttrList of attributes that apply to @preedit_string
 * @cursor_pos: position of cursor within preedit string in chars
 * 
 * Set the preedit string and attributes. The preedit string is a
 * string showing text that is currently being edited and not
 * yet committed into the buffer.
 */
void
btk_text_layout_set_preedit_string (BtkTextLayout *layout,
				    const gchar   *preedit_string,
				    BangoAttrList *preedit_attrs,
				    gint           cursor_pos)
{
  g_return_if_fail (BTK_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (preedit_attrs != NULL || preedit_string == NULL);

  g_free (layout->preedit_string);

  if (layout->preedit_attrs)
    bango_attr_list_unref (layout->preedit_attrs);

  if (preedit_string)
    {
      layout->preedit_string = g_strdup (preedit_string);
      layout->preedit_len = strlen (layout->preedit_string);
      bango_attr_list_ref (preedit_attrs);
      layout->preedit_attrs = preedit_attrs;

      cursor_pos = CLAMP (cursor_pos, 0, g_utf8_strlen (layout->preedit_string, -1));
      layout->preedit_cursor = g_utf8_offset_to_pointer (layout->preedit_string, cursor_pos) - layout->preedit_string;
    }
  else
    {
      layout->preedit_string = NULL;
      layout->preedit_len = 0;
      layout->preedit_attrs = NULL;
      layout->preedit_cursor = 0;
    }

  btk_text_layout_invalidate_cursor_line (layout, FALSE);
}

void
btk_text_layout_get_size (BtkTextLayout *layout,
                          gint *width,
                          gint *height)
{
  g_return_if_fail (BTK_IS_TEXT_LAYOUT (layout));

  if (width)
    *width = layout->width;

  if (height)
    *height = layout->height;
}

static void
btk_text_layout_invalidated (BtkTextLayout *layout)
{
  g_signal_emit (layout, signals[INVALIDATED], 0);
}

static void
btk_text_layout_emit_changed (BtkTextLayout *layout,
			      gint           y,
			      gint           old_height,
			      gint           new_height)
{
  g_signal_emit (layout, signals[CHANGED], 0, y, old_height, new_height);
}

static void
text_layout_changed (BtkTextLayout *layout,
                     gint           y,
                     gint           old_height,
                     gint           new_height,
                     gboolean       cursors_only)
{
  /* Check if the range intersects our cached line display,
   * and invalidate the cached line if so.
   */
  if (layout->one_display_cache)
    {
      BtkTextLine *line = layout->one_display_cache->line;
      gint cache_y = _btk_text_btree_find_line_top (_btk_text_buffer_get_btree (layout->buffer),
						    line, layout);
      gint cache_height = layout->one_display_cache->height;

      if (cache_y + cache_height > y && cache_y < y + old_height)
	btk_text_layout_invalidate_cache (layout, line, cursors_only);
    }

  btk_text_layout_emit_changed (layout, y, old_height, new_height);
}

void
btk_text_layout_changed (BtkTextLayout *layout,
                         gint           y,
                         gint           old_height,
                         gint           new_height)
{
  text_layout_changed (layout, y, old_height, new_height, FALSE);
}

void
btk_text_layout_cursors_changed (BtkTextLayout *layout,
                                 gint           y,
				 gint           old_height,
				 gint           new_height)
{
  text_layout_changed (layout, y, old_height, new_height, TRUE);
}

void
btk_text_layout_free_line_data (BtkTextLayout     *layout,
                                BtkTextLine       *line,
                                BtkTextLineData   *line_data)
{
  BTK_TEXT_LAYOUT_GET_CLASS (layout)->free_line_data (layout, line, line_data);
}

void
btk_text_layout_invalidate (BtkTextLayout *layout,
                            const BtkTextIter *start_index,
                            const BtkTextIter *end_index)
{
  BTK_TEXT_LAYOUT_GET_CLASS (layout)->invalidate (layout, start_index, end_index);
}

void
btk_text_layout_invalidate_cursors (BtkTextLayout *layout,
				    const BtkTextIter *start_index,
				    const BtkTextIter *end_index)
{
  BTK_TEXT_LAYOUT_GET_CLASS (layout)->invalidate_cursors (layout, start_index, end_index);
}

BtkTextLineData*
btk_text_layout_wrap (BtkTextLayout *layout,
                      BtkTextLine  *line,
                      /* may be NULL */
                      BtkTextLineData *line_data)
{
  return BTK_TEXT_LAYOUT_GET_CLASS (layout)->wrap (layout, line, line_data);
}


/**
 * btk_text_layout_get_lines:
 *
 * Return value: (element-type BtkTextLine) (transfer container):
 */
GSList*
btk_text_layout_get_lines (BtkTextLayout *layout,
                           /* [top_y, bottom_y) */
                           gint top_y,
                           gint bottom_y,
                           gint *first_line_y)
{
  BtkTextLine *first_btree_line;
  BtkTextLine *last_btree_line;
  BtkTextLine *line;
  GSList *retval;

  g_return_val_if_fail (BTK_IS_TEXT_LAYOUT (layout), NULL);
  g_return_val_if_fail (bottom_y > top_y, NULL);

  retval = NULL;

  first_btree_line =
    _btk_text_btree_find_line_by_y (_btk_text_buffer_get_btree (layout->buffer),
                                   layout, top_y, first_line_y);
  if (first_btree_line == NULL)
    {
      /* off the bottom */
      return NULL;
    }

  /* -1 since bottom_y is one past */
  last_btree_line =
    _btk_text_btree_find_line_by_y (_btk_text_buffer_get_btree (layout->buffer),
                                    layout, bottom_y - 1, NULL);

  if (!last_btree_line)
    last_btree_line =
      _btk_text_btree_get_end_iter_line (_btk_text_buffer_get_btree (layout->buffer));

  g_assert (last_btree_line != NULL);

  line = first_btree_line;
  while (TRUE)
    {
      retval = b_slist_prepend (retval, line);

      if (line == last_btree_line)
        break;

      line = _btk_text_line_next_excluding_last (line);
    }

  retval = b_slist_reverse (retval);

  return retval;
}

static void
invalidate_cached_style (BtkTextLayout *layout)
{
  free_style_cache (layout);
}

/* These should be called around a loop which wraps a CONTIGUOUS bunch
 * of display lines. If the lines aren't contiguous you can't call
 * these.
 */
void
btk_text_layout_wrap_loop_start (BtkTextLayout *layout)
{
  g_return_if_fail (BTK_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (layout->one_style_cache == NULL);

  layout->wrap_loop_count += 1;
}

void
btk_text_layout_wrap_loop_end (BtkTextLayout *layout)
{
  g_return_if_fail (layout->wrap_loop_count > 0);

  layout->wrap_loop_count -= 1;

  if (layout->wrap_loop_count == 0)
    {
      /* We cache a some stuff if we're iterating over some lines wrapping
       * them. This cleans it up.
       */
      /* Nuke our cached style */
      invalidate_cached_style (layout);
      g_assert (layout->one_style_cache == NULL);
    }
}

static void
btk_text_layout_invalidate_all (BtkTextLayout *layout)
{
  BtkTextIter start;
  BtkTextIter end;

  if (layout->buffer == NULL)
    return;

  btk_text_buffer_get_bounds (layout->buffer, &start, &end);

  btk_text_layout_invalidate (layout, &start, &end);
}

static void
btk_text_layout_invalidate_cache (BtkTextLayout *layout,
                                  BtkTextLine   *line,
				  gboolean       cursors_only)
{
  if (layout->one_display_cache && line == layout->one_display_cache->line)
    {
      BtkTextLineDisplay *display = layout->one_display_cache;

      if (cursors_only)
	{
	  b_slist_foreach (display->cursors, (GFunc)g_free, NULL);
	  b_slist_free (display->cursors);
	  display->cursors = NULL;
	  display->cursors_invalid = TRUE;
	  display->has_block_cursor = FALSE;
	}
      else
	{
	  layout->one_display_cache = NULL;
	  btk_text_layout_free_line_display (layout, display);
	}
    }
}

/* Now invalidate the paragraph containing the cursor
 */
static void
btk_text_layout_invalidate_cursor_line (BtkTextLayout *layout,
					gboolean cursors_only)
{
  BtkTextLayoutPrivate *priv = BTK_TEXT_LAYOUT_GET_PRIVATE (layout);
  BtkTextLineData *line_data;

  if (priv->cursor_line == NULL)
    return;

  line_data = _btk_text_line_get_data (priv->cursor_line, layout);
  if (line_data)
    {
      if (cursors_only)
	  btk_text_layout_invalidate_cache (layout, priv->cursor_line, TRUE);
      else
	{
	  btk_text_layout_invalidate_cache (layout, priv->cursor_line, FALSE);
	  _btk_text_line_invalidate_wrap (priv->cursor_line, line_data);
	}

      btk_text_layout_invalidated (layout);
    }
}

static void
btk_text_layout_update_cursor_line(BtkTextLayout *layout)
{
  BtkTextLayoutPrivate *priv = BTK_TEXT_LAYOUT_GET_PRIVATE (layout);
  BtkTextIter iter;

  btk_text_buffer_get_iter_at_mark (layout->buffer, &iter,
                                    btk_text_buffer_get_insert (layout->buffer));

  priv->cursor_line = _btk_text_iter_get_text_line (&iter);
}

static void
btk_text_layout_real_invalidate (BtkTextLayout *layout,
                                 const BtkTextIter *start,
                                 const BtkTextIter *end)
{
  BtkTextLine *line;
  BtkTextLine *last_line;

  g_return_if_fail (BTK_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (layout->wrap_loop_count == 0);

  /* Because we may be invalidating a mark, it's entirely possible
   * that btk_text_iter_equal (start, end) in which case we
   * should still invalidate the line they are both on. i.e.
   * we always invalidate the line with "start" even
   * if there's an empty range.
   */
  
#if 0
  btk_text_view_index_spew (start_index, "invalidate start");
  btk_text_view_index_spew (end_index, "invalidate end");
#endif

  last_line = _btk_text_iter_get_text_line (end);
  line = _btk_text_iter_get_text_line (start);

  while (TRUE)
    {
      BtkTextLineData *line_data = _btk_text_line_get_data (line, layout);

      btk_text_layout_invalidate_cache (layout, line, FALSE);
      
      if (line_data)
        _btk_text_line_invalidate_wrap (line, line_data);

      if (line == last_line)
        break;

      line = _btk_text_line_next_excluding_last (line);
    }

  btk_text_layout_invalidated (layout);
}

static void
btk_text_layout_real_invalidate_cursors (BtkTextLayout     *layout,
					 const BtkTextIter *start,
					 const BtkTextIter *end)
{
  /* Check if the range intersects our cached line display,
   * and invalidate the cached line if so.
   */
  if (layout->one_display_cache)
    {
      BtkTextIter line_start, line_end;
      BtkTextLine *line = layout->one_display_cache->line;

      _btk_text_btree_get_iter_at_line (_btk_text_buffer_get_btree (layout->buffer),
                                        &line_start, line, 0);
      line_end = line_start;
      if (!btk_text_iter_ends_line (&line_end))
	btk_text_iter_forward_to_line_end (&line_end);

      if (btk_text_iter_compare (start, end) > 0)
	{
	  const BtkTextIter *tmp = start;
	  start = end;
	  end = tmp;
	}

      if (btk_text_iter_compare (&line_start, end) <= 0 &&
	  btk_text_iter_compare (start, &line_end) <= 0)
	{
	  btk_text_layout_invalidate_cache (layout, line, TRUE);
	}
    }

  btk_text_layout_invalidated (layout);
}

static void
btk_text_layout_real_free_line_data (BtkTextLayout     *layout,
                                     BtkTextLine       *line,
                                     BtkTextLineData   *line_data)
{
  btk_text_layout_invalidate_cache (layout, line, FALSE);

  g_free (line_data);
}

/**
 * btk_text_layout_is_valid:
 * @layout: a #BtkTextLayout
 *
 * Check if there are any invalid rebunnyions in a #BtkTextLayout's buffer
 *
 * Return value: %TRUE if any invalid rebunnyions were found
 */
gboolean
btk_text_layout_is_valid (BtkTextLayout *layout)
{
  g_return_val_if_fail (BTK_IS_TEXT_LAYOUT (layout), FALSE);

  return _btk_text_btree_is_valid (_btk_text_buffer_get_btree (layout->buffer),
                                  layout);
}

static void
update_layout_size (BtkTextLayout *layout)
{
  _btk_text_btree_get_view_size (_btk_text_buffer_get_btree (layout->buffer),
				layout,
				&layout->width, &layout->height);
}

/**
 * btk_text_layout_validate_yrange:
 * @layout: a #BtkTextLayout
 * @anchor: iter pointing into a line that will be used as the
 *          coordinate origin
 * @y0_: offset from the top of the line pointed to by @anchor at
 *       which to begin validation. (The offset here is in pixels
 *       after validation.)
 * @y1_: offset from the top of the line pointed to by @anchor at
 *       which to end validation. (The offset here is in pixels
 *       after validation.)
 *
 * Ensure that a rebunnyion of a #BtkTextLayout is valid. The ::changed
 * signal will be emitted if any lines are validated.
 */
void
btk_text_layout_validate_yrange (BtkTextLayout *layout,
                                 BtkTextIter   *anchor,
                                 gint           y0,
                                 gint           y1)
{
  BtkTextLine *line;
  BtkTextLine *first_line = NULL;
  BtkTextLine *last_line = NULL;
  gint seen;
  gint delta_height = 0;
  gint first_line_y = 0;        /* Quiet GCC */
  gint last_line_y = 0;         /* Quiet GCC */

  g_return_if_fail (BTK_IS_TEXT_LAYOUT (layout));

  if (y0 > 0)
    y0 = 0;
  if (y1 < 0)
    y1 = 0;
  
  /* Validate backwards from the anchor line to y0
   */
  line = _btk_text_iter_get_text_line (anchor);
  line = _btk_text_line_previous (line);
  seen = 0;
  while (line && seen < -y0)
    {
      BtkTextLineData *line_data = _btk_text_line_get_data (line, layout);
      if (!line_data || !line_data->valid)
        {
          gint old_height, new_height;
	  
	  old_height = line_data ? line_data->height : 0;

          _btk_text_btree_validate_line (_btk_text_buffer_get_btree (layout->buffer),
                                         line, layout);
          line_data = _btk_text_line_get_data (line, layout);

	  new_height = line_data ? line_data->height : 0;

          delta_height += new_height - old_height;
          
          first_line = line;
          first_line_y = -seen - new_height;
          if (!last_line)
            {
              last_line = line;
              last_line_y = -seen;
            }
        }

      seen += line_data ? line_data->height : 0;
      line = _btk_text_line_previous (line);
    }

  /* Validate forwards to y1 */
  line = _btk_text_iter_get_text_line (anchor);
  seen = 0;
  while (line && seen < y1)
    {
      BtkTextLineData *line_data = _btk_text_line_get_data (line, layout);
      if (!line_data || !line_data->valid)
        {
          gint old_height, new_height;
	  
	  old_height = line_data ? line_data->height : 0;

          _btk_text_btree_validate_line (_btk_text_buffer_get_btree (layout->buffer),
                                         line, layout);
          line_data = _btk_text_line_get_data (line, layout);
	  new_height = line_data ? line_data->height : 0;

          delta_height += new_height - old_height;
          
          if (!first_line)
            {
              first_line = line;
              first_line_y = seen;
            }
          last_line = line;
          last_line_y = seen + new_height;
        }

      seen += line_data ? line_data->height : 0;
      line = _btk_text_line_next_excluding_last (line);
    }

  /* If we found and validated any invalid lines, update size and
   * emit the changed signal
   */
  if (first_line)
    {
      gint line_top;

      update_layout_size (layout);

      line_top = _btk_text_btree_find_line_top (_btk_text_buffer_get_btree (layout->buffer),
                                                first_line, layout);

      btk_text_layout_emit_changed (layout,
				    line_top,
				    last_line_y - first_line_y - delta_height,
				    last_line_y - first_line_y);
    }
}

/**
 * btk_text_layout_validate:
 * @tree: a #BtkTextLayout
 * @max_pixels: the maximum number of pixels to validate. (No more
 *              than one paragraph beyond this limit will be validated)
 *
 * Validate rebunnyions of a #BtkTextLayout. The ::changed signal will
 * be emitted for each rebunnyion validated.
 **/
void
btk_text_layout_validate (BtkTextLayout *layout,
                          gint           max_pixels)
{
  gint y, old_height, new_height;

  g_return_if_fail (BTK_IS_TEXT_LAYOUT (layout));

  while (max_pixels > 0 &&
         _btk_text_btree_validate (_btk_text_buffer_get_btree (layout->buffer),
                                   layout,  max_pixels,
                                   &y, &old_height, &new_height))
    {
      max_pixels -= new_height;

      update_layout_size (layout);
      btk_text_layout_emit_changed (layout, y, old_height, new_height);
    }
}

static BtkTextLineData*
btk_text_layout_real_wrap (BtkTextLayout   *layout,
                           BtkTextLine     *line,
                           /* may be NULL */
                           BtkTextLineData *line_data)
{
  BtkTextLineDisplay *display;

  g_return_val_if_fail (BTK_IS_TEXT_LAYOUT (layout), NULL);
  g_return_val_if_fail (line != NULL, NULL);
  
  if (line_data == NULL)
    {
      line_data = _btk_text_line_data_new (layout, line);
      _btk_text_line_add_data (line, line_data);
    }

  display = btk_text_layout_get_line_display (layout, line, TRUE);
  line_data->width = display->width;
  line_data->height = display->height;
  line_data->valid = TRUE;
  btk_text_layout_free_line_display (layout, display);

  return line_data;
}

/*
 * Layout utility functions
 */

/* If you get the style with get_style () you need to call
   release_style () to free it. */
static BtkTextAttributes*
get_style (BtkTextLayout *layout,
	   GPtrArray     *tags)
{
  BtkTextAttributes *style;

  /* If we have the one-style cache, then it means
     that we haven't seen a toggle since we filled in the
     one-style cache.
  */
  if (layout->one_style_cache != NULL)
    {
      btk_text_attributes_ref (layout->one_style_cache);
      return layout->one_style_cache;
    }

  g_assert (layout->one_style_cache == NULL);

  /* No tags, use default style */
  if (tags == NULL || tags->len == 0)
    {
      /* One ref for the return value, one ref for the
         layout->one_style_cache reference */
      btk_text_attributes_ref (layout->default_style);
      btk_text_attributes_ref (layout->default_style);
      layout->one_style_cache = layout->default_style;

      return layout->default_style;
    }

  style = btk_text_attributes_new ();

  btk_text_attributes_copy_values (layout->default_style,
                                   style);

  _btk_text_attributes_fill_from_tags (style,
                                       (BtkTextTag**) tags->pdata,
                                       tags->len);

  g_assert (style->refcount == 1);

  /* Leave this style as the last one seen */
  g_assert (layout->one_style_cache == NULL);
  btk_text_attributes_ref (style); /* ref held by layout->one_style_cache */
  layout->one_style_cache = style;

  /* Returning yet another refcount */
  return style;
}

static void
release_style (BtkTextLayout *layout,
               BtkTextAttributes *style)
{
  g_return_if_fail (style != NULL);
  g_return_if_fail (style->refcount > 0);

  btk_text_attributes_unref (style);
}

/*
 * Lines
 */

/* This function tries to optimize the case where a line
   is completely invisible */
static gboolean
totally_invisible_line (BtkTextLayout *layout,
                        BtkTextLine   *line,
                        BtkTextIter   *iter)
{
  BtkTextLineSegment *seg;
  int bytes = 0;

  /* Check if the first char is visible, if so we are partially visible.  
   * Note that we have to check this since we don't know the current 
   * invisible/noninvisible toggle state; this function can use the whole btree 
   * to get it right.
   */
  _btk_text_btree_get_iter_at_line (_btk_text_buffer_get_btree (layout->buffer),
				    iter, line, 0);
  
  if (!_btk_text_btree_char_is_invisible (iter))
    return FALSE;

  bytes = 0;
  seg = line->segments;

  while (seg != NULL)
    {
      if (seg->byte_count > 0)
        bytes += seg->byte_count;

      /* Note that these two tests can cause us to bail out
       * when we shouldn't, because a higher-priority tag
       * may override these settings. However the important
       * thing is to only invisible really-invisible lines, rather
       * than to invisible all really-invisible lines.
       */

      else if (seg->type == &btk_text_toggle_on_type)
        {
          invalidate_cached_style (layout);

          /* Bail out if an elision-unsetting tag begins */
          if (seg->body.toggle.info->tag->invisible_set &&
              !seg->body.toggle.info->tag->values->invisible)
            break;
        }
      else if (seg->type == &btk_text_toggle_off_type)
        {
          invalidate_cached_style (layout);

          /* Bail out if an elision-setting tag ends */
          if (seg->body.toggle.info->tag->invisible_set &&
              seg->body.toggle.info->tag->values->invisible)
            break;
        }

      seg = seg->next;
    }

  if (seg != NULL)       /* didn't reach line end */
    return FALSE;

  return TRUE;
}

static void
set_para_values (BtkTextLayout      *layout,
                 BangoDirection      base_dir,
                 BtkTextAttributes  *style,
                 BtkTextLineDisplay *display)
{
  BangoAlignment bango_align = BANGO_ALIGN_LEFT;
  BangoWrapMode bango_wrap = BANGO_WRAP_WORD;

  switch (base_dir)
    {
    /* If no base direction was found, then use the style direction */
    case BANGO_DIRECTION_NEUTRAL :
      display->direction = style->direction;

      /* Override the base direction */
      if (display->direction == BTK_TEXT_DIR_RTL)
        base_dir = BANGO_DIRECTION_RTL;
      else
        base_dir = BANGO_DIRECTION_LTR;
      
      break;
    case BANGO_DIRECTION_RTL :
      display->direction = BTK_TEXT_DIR_RTL;
      break;
    default:
      display->direction = BTK_TEXT_DIR_LTR;
      break;
    }
  
  if (display->direction == BTK_TEXT_DIR_RTL)
    display->layout = bango_layout_new (layout->rtl_context);
  else
    display->layout = bango_layout_new (layout->ltr_context);

  switch (style->justification)
    {
    case BTK_JUSTIFY_LEFT:
      bango_align = (base_dir == BANGO_DIRECTION_LTR) ? BANGO_ALIGN_LEFT : BANGO_ALIGN_RIGHT;
      break;
    case BTK_JUSTIFY_RIGHT:
      bango_align = (base_dir == BANGO_DIRECTION_LTR) ? BANGO_ALIGN_RIGHT : BANGO_ALIGN_LEFT;
      break;
    case BTK_JUSTIFY_CENTER:
      bango_align = BANGO_ALIGN_CENTER;
      break;
    case BTK_JUSTIFY_FILL:
      bango_align = (base_dir == BANGO_DIRECTION_LTR) ? BANGO_ALIGN_LEFT : BANGO_ALIGN_RIGHT;
      bango_layout_set_justify (display->layout, TRUE);
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  bango_layout_set_alignment (display->layout, bango_align);
  bango_layout_set_spacing (display->layout,
                            style->pixels_inside_wrap * BANGO_SCALE);

  if (style->tabs)
    bango_layout_set_tabs (display->layout, style->tabs);

  display->top_margin = style->pixels_above_lines;
  display->height = style->pixels_above_lines + style->pixels_below_lines;
  display->bottom_margin = style->pixels_below_lines;
  display->left_margin = style->left_margin;
  display->right_margin = style->right_margin;
  
  display->x_offset = display->left_margin;

  bango_layout_set_indent (display->layout,
                           style->indent * BANGO_SCALE);

  switch (style->wrap_mode)
    {
    case BTK_WRAP_CHAR:
      bango_wrap = BANGO_WRAP_CHAR;
      break;
    case BTK_WRAP_WORD:
      bango_wrap = BANGO_WRAP_WORD;
      break;

    case BTK_WRAP_WORD_CHAR:
      bango_wrap = BANGO_WRAP_WORD_CHAR;
      break;

    case BTK_WRAP_NONE:
      break;
    }

  if (style->wrap_mode != BTK_WRAP_NONE)
    {
      int layout_width = (layout->screen_width - display->left_margin - display->right_margin);
      bango_layout_set_width (display->layout, layout_width * BANGO_SCALE);
      bango_layout_set_wrap (display->layout, bango_wrap);
    }

  display->total_width = MAX (layout->screen_width, layout->width) - display->left_margin - display->right_margin;
  
  if (style->pg_bg_color)
    display->pg_bg_color = bdk_color_copy (style->pg_bg_color);
  else
    display->pg_bg_color = NULL;  
}

static BangoAttribute *
btk_text_attr_appearance_copy (const BangoAttribute *attr)
{
  const BtkTextAttrAppearance *appearance_attr = (const BtkTextAttrAppearance *)attr;

  return btk_text_attr_appearance_new (&appearance_attr->appearance);
}

static void
btk_text_attr_appearance_destroy (BangoAttribute *attr)
{
  BtkTextAttrAppearance *appearance_attr = (BtkTextAttrAppearance *)attr;

  if (appearance_attr->appearance.bg_stipple)
    g_object_unref (appearance_attr->appearance.bg_stipple);
  if (appearance_attr->appearance.fg_stipple)
    g_object_unref (appearance_attr->appearance.fg_stipple);

  g_slice_free (BtkTextAttrAppearance, appearance_attr);
}

static gboolean
btk_text_attr_appearance_compare (const BangoAttribute *attr1,
                                  const BangoAttribute *attr2)
{
  const BtkTextAppearance *appearance1 = &((const BtkTextAttrAppearance *)attr1)->appearance;
  const BtkTextAppearance *appearance2 = &((const BtkTextAttrAppearance *)attr2)->appearance;

  return (bdk_color_equal (&appearance1->fg_color, &appearance2->fg_color) &&
          bdk_color_equal (&appearance1->bg_color, &appearance2->bg_color) &&
          appearance1->fg_stipple ==  appearance2->fg_stipple &&
          appearance1->bg_stipple ==  appearance2->bg_stipple &&
          appearance1->underline == appearance2->underline &&
          appearance1->strikethrough == appearance2->strikethrough &&
          appearance1->draw_bg == appearance2->draw_bg);
}

/*
 * btk_text_attr_appearance_new:
 * @desc:
 *
 * Create a new font description attribute. (This attribute
 * allows setting family, style, weight, variant, stretch,
 * and size simultaneously.)
 *
 * Return value:
 */
static BangoAttribute *
btk_text_attr_appearance_new (const BtkTextAppearance *appearance)
{
  static BangoAttrClass klass = {
    0,
    btk_text_attr_appearance_copy,
    btk_text_attr_appearance_destroy,
    btk_text_attr_appearance_compare
  };

  BtkTextAttrAppearance *result;

  if (!klass.type)
    klass.type = btk_text_attr_appearance_type =
      bango_attr_type_register ("BtkTextAttrAppearance");

  result = g_slice_new (BtkTextAttrAppearance);
  result->attr.klass = &klass;

  result->appearance = *appearance;

  if (appearance->bg_stipple)
    g_object_ref (appearance->bg_stipple);
  if (appearance->fg_stipple)
    g_object_ref (appearance->fg_stipple);

  return (BangoAttribute *)result;
}

static void
add_generic_attrs (BtkTextLayout      *layout,
                   BtkTextAppearance  *appearance,
                   gint                byte_count,
                   BangoAttrList      *attrs,
                   gint                start,
                   gboolean            size_only,
                   gboolean            is_text)
{
  BangoAttribute *attr;

  if (appearance->underline != BANGO_UNDERLINE_NONE)
    {
      attr = bango_attr_underline_new (appearance->underline);
      
      attr->start_index = start;
      attr->end_index = start + byte_count;
      
      bango_attr_list_insert (attrs, attr);
    }

  if (appearance->strikethrough)
    {
      attr = bango_attr_strikethrough_new (appearance->strikethrough);
      
      attr->start_index = start;
      attr->end_index = start + byte_count;
      
      bango_attr_list_insert (attrs, attr);
    }

  if (appearance->rise != 0)
    {
      attr = bango_attr_rise_new (appearance->rise);
      
      attr->start_index = start;
      attr->end_index = start + byte_count;
      
      bango_attr_list_insert (attrs, attr);
    }
  
  if (!size_only)
    {
      attr = btk_text_attr_appearance_new (appearance);
      
      attr->start_index = start;
      attr->end_index = start + byte_count;

      ((BtkTextAttrAppearance *)attr)->appearance.is_text = is_text;
      
      bango_attr_list_insert (attrs, attr);
    }
}

static void
add_text_attrs (BtkTextLayout      *layout,
                BtkTextAttributes  *style,
                gint                byte_count,
                BangoAttrList      *attrs,
                gint                start,
                gboolean            size_only)
{
  BangoAttribute *attr;

  attr = bango_attr_font_desc_new (style->font);
  attr->start_index = start;
  attr->end_index = start + byte_count;

  bango_attr_list_insert (attrs, attr);

  if (style->font_scale != 1.0)
    {
      attr = bango_attr_scale_new (style->font_scale);

      attr->start_index = start;
      attr->end_index = start + byte_count;
      
      bango_attr_list_insert (attrs, attr);
    }
}

static void
add_pixbuf_attrs (BtkTextLayout      *layout,
                  BtkTextLineDisplay *display,
                  BtkTextAttributes  *style,
                  BtkTextLineSegment *seg,
                  BangoAttrList      *attrs,
                  gint                start)
{
  BangoAttribute *attr;
  BangoRectangle logical_rect;
  BtkTextPixbuf *pixbuf = &seg->body.pixbuf;
  gint width, height;

  width = bdk_pixbuf_get_width (pixbuf->pixbuf);
  height = bdk_pixbuf_get_height (pixbuf->pixbuf);

  logical_rect.x = 0;
  logical_rect.y = -height * BANGO_SCALE;
  logical_rect.width = width * BANGO_SCALE;
  logical_rect.height = height * BANGO_SCALE;

  attr = bango_attr_shape_new_with_data (&logical_rect, &logical_rect,
					 pixbuf->pixbuf, NULL, NULL);
  attr->start_index = start;
  attr->end_index = start + seg->byte_count;
  bango_attr_list_insert (attrs, attr);

  display->shaped_objects =
    b_slist_append (display->shaped_objects, pixbuf->pixbuf);
}

static void
add_child_attrs (BtkTextLayout      *layout,
                 BtkTextLineDisplay *display,
                 BtkTextAttributes  *style,
                 BtkTextLineSegment *seg,
                 BangoAttrList      *attrs,
                 gint                start)
{
  BangoAttribute *attr;
  BangoRectangle logical_rect;
  gint width, height;
  GSList *tmp_list;
  BtkWidget *widget;

  width = 1;
  height = 1;
  
  tmp_list = seg->body.child.widgets;
  while (tmp_list != NULL)
    {
      BtkWidget *child = tmp_list->data;

      if (_btk_anchored_child_get_layout (child) == layout)
        {
          /* Found it */
          BtkRequisition req;

          btk_widget_get_child_requisition (child, &req);
          
          width = req.width;
          height = req.height;

	  widget = child;
          
          break;
        }
      
      tmp_list = b_slist_next (tmp_list);
    }

  if (tmp_list == NULL)
    {
      /* If tmp_list == NULL then there is no widget at this anchor in
       * this display; not an error. We make up an arbitrary size
       * to use, just so the programmer can see the blank spot.
       * We also put a NULL in the shaped objects list, to keep
       * the correspondence between the list and the shaped chars in
       * the layout. A bad hack, yes.
       */

      width = 30;
      height = 20;

      widget = NULL;
    }

  display->shaped_objects = b_slist_append (display->shaped_objects, widget);
  
  logical_rect.x = 0;
  logical_rect.y = -height * BANGO_SCALE;
  logical_rect.width = width * BANGO_SCALE;
  logical_rect.height = height * BANGO_SCALE;

  attr = bango_attr_shape_new_with_data (&logical_rect, &logical_rect,
					 widget, NULL, NULL);
  attr->start_index = start;
  attr->end_index = start + seg->byte_count;
  bango_attr_list_insert (attrs, attr);
}

/*
 * get_block_cursor:
 * @layout: a #BtkTextLayout
 * @display: a #BtkTextLineDisplay
 * @insert_iter: iter pointing to the cursor location
 * @insert_index: cursor offset in the @display's layout, it may
 * be different from @insert_iter's offset in case when preedit
 * string is present.
 * @pos: location to store cursor position
 * @cursor_at_line_end: whether cursor is at the end of line
 *
 * Checks whether layout should display block cursor at given position.
 * For this layout must be in overwrite mode and text at @insert_iter 
 * must be editable.
 */
static gboolean
get_block_cursor (BtkTextLayout      *layout,
		  BtkTextLineDisplay *display,
		  const BtkTextIter  *insert_iter,
		  gint                insert_index,
		  BdkRectangle       *pos,
		  gboolean           *cursor_at_line_end)
{
  BangoRectangle bango_pos;

  if (layout->overwrite_mode &&
      btk_text_iter_editable (insert_iter, TRUE) &&
      _btk_text_util_get_block_cursor_location (display->layout,
						insert_index,
						&bango_pos,
    					        cursor_at_line_end))
    {
      if (pos)
	{
	  pos->x = BANGO_PIXELS (bango_pos.x);
	  pos->y = BANGO_PIXELS (bango_pos.y);
	  pos->width = BANGO_PIXELS (bango_pos.width);
	  pos->height = BANGO_PIXELS (bango_pos.height);
	}

      return TRUE;
    }
  else
    return FALSE;
}

static void
add_cursor (BtkTextLayout      *layout,
            BtkTextLineDisplay *display,
            BtkTextLineSegment *seg,
            gint                start)
{
  BangoRectangle strong_pos, weak_pos;
  BtkTextCursorDisplay *cursor = NULL; /* Quiet GCC */
  gboolean add_weak = FALSE;
  gboolean add_strong = FALSE;
  
  /* Hide insertion cursor when we have a selection or the layout
   * user has hidden the cursor.
   */
  if (_btk_text_btree_mark_is_insert (_btk_text_buffer_get_btree (layout->buffer),
                                     seg->body.mark.obj) &&
      (!layout->cursor_visible ||
       btk_text_buffer_get_selection_bounds (layout->buffer, NULL, NULL)))
    return;

  if (layout->overwrite_mode &&
      _btk_text_btree_mark_is_insert (_btk_text_buffer_get_btree (layout->buffer),
				      seg->body.mark.obj))
    {
      BtkTextIter iter;
      gboolean cursor_at_line_end;

      _btk_text_btree_get_iter_at_mark (_btk_text_buffer_get_btree (layout->buffer),
					&iter, seg->body.mark.obj);

      if (get_block_cursor (layout, display, &iter, start,
			    &display->block_cursor,
			    &cursor_at_line_end))
	{
	  display->has_block_cursor = TRUE;
	  display->cursor_at_line_end = cursor_at_line_end;
	  return;
	}
    }

  bango_layout_get_cursor_pos (display->layout, start, &strong_pos, &weak_pos);

  if (layout->cursor_direction == BTK_TEXT_DIR_NONE)
    {
      add_strong = TRUE;
      add_weak = TRUE;
    }
  else if (display->direction == layout->cursor_direction)
    add_strong = TRUE;
  else
    add_weak = TRUE;

  if (add_strong)
    {
      cursor = g_new (BtkTextCursorDisplay, 1);

      cursor->x = BANGO_PIXELS (strong_pos.x);
      cursor->y = BANGO_PIXELS (strong_pos.y);
      cursor->height = BANGO_PIXELS (strong_pos.height);
      cursor->is_strong = TRUE;
      cursor->is_weak = (layout->cursor_direction == BTK_TEXT_DIR_NONE) ? FALSE : TRUE;
      display->cursors = b_slist_prepend (display->cursors, cursor);
    }
  
  if (add_weak)
    {
      if (weak_pos.x == strong_pos.x && add_strong)
	cursor->is_weak = TRUE;
      else
	{
	  cursor = g_new (BtkTextCursorDisplay, 1);
	  
	  cursor->x = BANGO_PIXELS (weak_pos.x);
	  cursor->y = BANGO_PIXELS (weak_pos.y);
	  cursor->height = BANGO_PIXELS (weak_pos.height);
	  cursor->is_strong = (layout->cursor_direction == BTK_TEXT_DIR_NONE) ? FALSE : TRUE;
	  cursor->is_weak = TRUE;
	  display->cursors = b_slist_prepend (display->cursors, cursor);
	}
    }
}

static gboolean
is_shape (BangoLayoutRun *run)
{
  GSList *tmp_list = run->item->analysis.extra_attrs;
    
  while (tmp_list)
    {
      BangoAttribute *attr = tmp_list->data;

      if (attr->klass->type == BANGO_ATTR_SHAPE)
        return TRUE;

      tmp_list = tmp_list->next;
    }

  return FALSE;
}

static void
allocate_child_widgets (BtkTextLayout      *text_layout,
                        BtkTextLineDisplay *display)
{
  BangoLayout *layout = display->layout;
  BangoLayoutIter *run_iter;

  run_iter = bango_layout_get_iter (layout);
  do
    {
      BangoLayoutRun *run = bango_layout_iter_get_run_readonly (run_iter);

      if (run && is_shape (run))
        {
          gint byte_index;
          BtkTextIter text_iter;
          BtkTextChildAnchor *anchor = NULL;
          GList *widgets = NULL;
          GList *l;

          /* The bango iterator iterates in visual order.
           * We use the byte index to find the child widget.
           */
          byte_index = bango_layout_iter_get_index (run_iter);
          line_display_index_to_iter (text_layout, display, &text_iter, byte_index, 0);
          anchor = btk_text_iter_get_child_anchor (&text_iter);
	  if (anchor)
            widgets = btk_text_child_anchor_get_widgets (anchor);

          for (l = widgets; l; l = l->next)
            {
              BangoRectangle extents;
              BtkWidget *child = l->data;

              if (_btk_anchored_child_get_layout (child) == text_layout)
                {

                  /* We emit "allocate_child" with the x,y of
                   * the widget with respect to the top of the line
                   * and the left side of the buffer
                   */
                  bango_layout_iter_get_run_extents (run_iter,
                                                     NULL,
                                                     &extents);

                  g_signal_emit (text_layout,
                                 signals[ALLOCATE_CHILD],
                                 0,
                                 child,
                                 BANGO_PIXELS (extents.x) + display->x_offset,
                                 BANGO_PIXELS (extents.y) + display->top_margin);
                }
            }

          g_list_free (widgets);
        }
    }
  while (bango_layout_iter_next_run (run_iter));

  bango_layout_iter_free (run_iter);
}

static void
convert_color (BdkColor       *result,
	       BangoAttrColor *attr)
{
  result->red = attr->color.red;
  result->blue = attr->color.blue;
  result->green = attr->color.green;
}

/* This function is used to convert the preedit string attributes, which are
 * standard BangoAttributes, into the custom attributes used by the text
 * widget and insert them into a attr list with a given offset.
 */
static void
add_preedit_attrs (BtkTextLayout     *layout,
		   BtkTextAttributes *style,
		   BangoAttrList     *attrs,
		   gint               offset,
		   gboolean           size_only)
{
  BangoAttrIterator *iter = bango_attr_list_get_iterator (layout->preedit_attrs);

  do
    {
      BtkTextAppearance appearance = style->appearance;
      BangoFontDescription *font_desc = bango_font_description_copy_static (style->font);
      BangoAttribute *insert_attr;
      GSList *extra_attrs = NULL;
      GSList *tmp_list;
      BangoLanguage *language;
      gint start, end;

      bango_attr_iterator_range (iter, &start, &end);

      if (end == G_MAXINT)
	end = layout->preedit_len;
      
      if (end == start)
	continue;

      bango_attr_iterator_get_font (iter, font_desc, &language, &extra_attrs);
      
      tmp_list = extra_attrs;
      while (tmp_list)
	{
	  BangoAttribute *attr = tmp_list->data;
	  
	  switch (attr->klass->type)
	    {
	    case BANGO_ATTR_FOREGROUND:
	      convert_color (&appearance.fg_color, (BangoAttrColor *)attr);
	      break;
	    case BANGO_ATTR_BACKGROUND:
	      convert_color (&appearance.bg_color, (BangoAttrColor *)attr);
	      appearance.draw_bg = TRUE;
	      break;
	    case BANGO_ATTR_UNDERLINE:
	      appearance.underline = ((BangoAttrInt *)attr)->value;
	      break;
	    case BANGO_ATTR_STRIKETHROUGH:
	      appearance.strikethrough = ((BangoAttrInt *)attr)->value;
	      break;
            case BANGO_ATTR_RISE:
              appearance.rise = ((BangoAttrInt *)attr)->value;
              break;
	    default:
	      break;
	    }
	  
	  bango_attribute_destroy (attr);
	  tmp_list = tmp_list->next;
	}
      
      b_slist_free (extra_attrs);
      
      insert_attr = bango_attr_font_desc_new (font_desc);
      insert_attr->start_index = start + offset;
      insert_attr->end_index = end + offset;
      
      bango_attr_list_insert (attrs, insert_attr);

      if (language)
	{
	  insert_attr = bango_attr_language_new (language);
	  insert_attr->start_index = start + offset;
	  insert_attr->end_index = end + offset;
	  
	  bango_attr_list_insert (attrs, insert_attr);
	}

      add_generic_attrs (layout, &appearance, end - start,
                         attrs, start + offset,
                         size_only, TRUE);
      
      bango_font_description_free (font_desc);
    }
  while (bango_attr_iterator_next (iter));

  bango_attr_iterator_destroy (iter);
}

/* Iterate over the line and fill in display->cursors.
 * It's a stripped copy of btk_text_layout_get_line_display() */
static void
update_text_display_cursors (BtkTextLayout      *layout,
			     BtkTextLine        *line,
			     BtkTextLineDisplay *display)
{
  BtkTextLineSegment *seg;
  BtkTextIter iter;
  gint layout_byte_offset, buffer_byte_offset;
  GSList *cursor_byte_offsets = NULL;
  GSList *cursor_segs = NULL;
  GSList *tmp_list1, *tmp_list2;

  if (!display->cursors_invalid)
    return;

  display->cursors_invalid = FALSE;

  _btk_text_btree_get_iter_at_line (_btk_text_buffer_get_btree (layout->buffer),
                                    &iter, line, 0);

  /* Special-case optimization for completely
   * invisible lines; makes it faster to deal
   * with sequences of invisible lines.
   */
  if (totally_invisible_line (layout, line, &iter))
    return;

  /* Iterate over segments */
  layout_byte_offset = 0; /* position in the layout text (includes preedit, does not include invisible text) */
  buffer_byte_offset = 0; /* position in the buffer line */
  seg = _btk_text_iter_get_any_segment (&iter);
  while (seg != NULL)
    {
      /* Displayable segments */
      if (seg->type == &btk_text_char_type ||
          seg->type == &btk_text_pixbuf_type ||
          seg->type == &btk_text_child_type)
        {
          _btk_text_btree_get_iter_at_line (_btk_text_buffer_get_btree (layout->buffer),
                                            &iter, line,
                                            buffer_byte_offset);

	  if (!_btk_text_btree_char_is_invisible (&iter))
            layout_byte_offset += seg->byte_count;

	  buffer_byte_offset += seg->byte_count;
        }

      /* Marks */
      else if (seg->type == &btk_text_right_mark_type ||
               seg->type == &btk_text_left_mark_type)
        {
	  gint cursor_offset = 0;

	  /* At the insertion point, add the preedit string, if any */

	  if (_btk_text_btree_mark_is_insert (_btk_text_buffer_get_btree (layout->buffer),
					      seg->body.mark.obj))
	    {
	      display->insert_index = layout_byte_offset;

	      if (layout->preedit_len > 0)
		{
		  layout_byte_offset += layout->preedit_len;
                  /* DO NOT increment the buffer byte offset for preedit */
		  cursor_offset = layout->preedit_cursor - layout->preedit_len;
		}
	    }

          /* Display visible marks */

          if (seg->body.mark.visible)
            {
              cursor_byte_offsets = b_slist_prepend (cursor_byte_offsets,
                                                     GINT_TO_POINTER (layout_byte_offset + cursor_offset));
              cursor_segs = b_slist_prepend (cursor_segs, seg);
            }
        }

      /* Toggles */
      else if (seg->type == &btk_text_toggle_on_type ||
               seg->type == &btk_text_toggle_off_type)
        {
        }

      else
        g_error ("Unknown segment type: %s", seg->type->name);

      seg = seg->next;
    }

  tmp_list1 = cursor_byte_offsets;
  tmp_list2 = cursor_segs;
  while (tmp_list1)
    {
      add_cursor (layout, display, tmp_list2->data,
                  GPOINTER_TO_INT (tmp_list1->data));
      tmp_list1 = tmp_list1->next;
      tmp_list2 = tmp_list2->next;
    }
  b_slist_free (cursor_byte_offsets);
  b_slist_free (cursor_segs);
}

/* Same as _btk_text_btree_get_tags(), except it returns GPtrArray,
 * to be used in btk_text_layout_get_line_display(). */
static GPtrArray *
get_tags_array_at_iter (BtkTextIter *iter)
{
  BtkTextTag **tags;
  GPtrArray *array = NULL;
  gint n_tags;

  tags = _btk_text_btree_get_tags (iter, &n_tags);

  if (n_tags > 0)
    {
      array = g_ptr_array_sized_new (n_tags);
      g_ptr_array_set_size (array, n_tags);
      memcpy (array->pdata, tags, n_tags * sizeof (BtkTextTag*));
    }

  g_free (tags);
  return array;
}

/* Add the tag to the array if it's not there already, and remove
 * it otherwise. It keeps the array sorted by tags priority. */
static GPtrArray *
tags_array_toggle_tag (GPtrArray  *array,
		       BtkTextTag *tag)
{
  gint pos;
  BtkTextTag **tags;

  if (array == NULL)
    array = g_ptr_array_new ();

  tags = (BtkTextTag**) array->pdata;

  for (pos = 0; pos < array->len && tags[pos]->priority < tag->priority; pos++) ;

  if (pos < array->len && tags[pos] == tag)
    g_ptr_array_remove_index (array, pos);
  else
    {
      g_ptr_array_set_size (array, array->len + 1);
      if (pos < array->len - 1)
	memmove (array->pdata + pos + 1, array->pdata + pos,
		 (array->len - pos - 1) * sizeof (BtkTextTag*));
      array->pdata[pos] = tag;
    }

  return array;
}

BtkTextLineDisplay *
btk_text_layout_get_line_display (BtkTextLayout *layout,
                                  BtkTextLine   *line,
                                  gboolean       size_only)
{
  BtkTextLayoutPrivate *priv = BTK_TEXT_LAYOUT_GET_PRIVATE (layout);
  BtkTextLineDisplay *display;
  BtkTextLineSegment *seg;
  BtkTextIter iter;
  BtkTextAttributes *style;
  gchar *text;
  BangoAttrList *attrs;
  gint text_allocated, layout_byte_offset, buffer_byte_offset;
  BangoRectangle extents;
  gboolean para_values_set = FALSE;
  GSList *cursor_byte_offsets = NULL;
  GSList *cursor_segs = NULL;
  GSList *tmp_list1, *tmp_list2;
  gboolean saw_widget = FALSE;
  BangoDirection base_dir;
  GPtrArray *tags;
  gboolean initial_toggle_segments;
  
  g_return_val_if_fail (line != NULL, NULL);

  if (layout->one_display_cache)
    {
      if (line == layout->one_display_cache->line &&
          (size_only || !layout->one_display_cache->size_only))
	{
	  if (!size_only)
            update_text_display_cursors (layout, line, layout->one_display_cache);
	  return layout->one_display_cache;
	}
      else
        {
          BtkTextLineDisplay *tmp_display = layout->one_display_cache;
          layout->one_display_cache = NULL;
          btk_text_layout_free_line_display (layout, tmp_display);
        }
    }

  DV (g_print ("creating one line display cache (%s)\n", B_STRLOC));

  display = g_new0 (BtkTextLineDisplay, 1);

  display->size_only = size_only;
  display->line = line;
  display->insert_index = -1;

  _btk_text_btree_get_iter_at_line (_btk_text_buffer_get_btree (layout->buffer),
                                    &iter, line, 0);

  /* Special-case optimization for completely
   * invisible lines; makes it faster to deal
   * with sequences of invisible lines.
   */
  if (totally_invisible_line (layout, line, &iter))
    {
      if (display->direction == BTK_TEXT_DIR_RTL)
	display->layout = bango_layout_new (layout->rtl_context);
      else
	display->layout = bango_layout_new (layout->ltr_context);
      
      return display;
    }

  /* Find the bidi base direction */
  base_dir = line->dir_propagated_forward;
  if (base_dir == BANGO_DIRECTION_NEUTRAL)
    base_dir = line->dir_propagated_back;

  if (line == priv->cursor_line &&
      line->dir_strong == BANGO_DIRECTION_NEUTRAL)
    {
      base_dir = (layout->keyboard_direction == BTK_TEXT_DIR_LTR) ?
	 BANGO_DIRECTION_LTR : BANGO_DIRECTION_RTL;
    }
  
  /* Allocate space for flat text for buffer
   */
  text_allocated = _btk_text_line_byte_count (line);
  text = g_malloc (text_allocated);

  attrs = bango_attr_list_new ();

  /* Iterate over segments, creating display chunks for them, and updating the tags array. */
  layout_byte_offset = 0; /* current length of layout text (includes preedit, does not include invisible text) */
  buffer_byte_offset = 0; /* position in the buffer line */
  seg = _btk_text_iter_get_any_segment (&iter);
  tags = get_tags_array_at_iter (&iter);
  initial_toggle_segments = TRUE;
  while (seg != NULL)
    {
      /* Displayable segments */
      if (seg->type == &btk_text_char_type ||
          seg->type == &btk_text_pixbuf_type ||
          seg->type == &btk_text_child_type)
        {
          style = get_style (layout, tags);
	  initial_toggle_segments = FALSE;

          /* We have to delay setting the paragraph values until we
           * hit the first pixbuf or text segment because toggles at
           * the beginning of the paragraph should affect the
           * paragraph-global values
           */
          if (!para_values_set)
            {
              set_para_values (layout, base_dir, style, display);
              para_values_set = TRUE;
            }

          /* First see if the chunk is invisible, and ignore it if so. Tk
           * looked at tabs, wrap mode, etc. before doing this, but
           * that made no sense to me, so I am just skipping the
           * invisible chunks
           */
          if (!style->invisible)
            {
              if (seg->type == &btk_text_char_type)
                {
                  /* We don't want to split segments because of marks,
                   * so we scan forward for more segments only
                   * separated from us by marks. In theory, we should
                   * also merge segments with identical styles, even
                   * if there are toggles in-between
                   */

                  gint bytes = 0;
 		  BtkTextLineSegment *prev_seg = NULL;
  
 		  while (seg)
                    {
                      if (seg->type == &btk_text_char_type)
                        {
                          memcpy (text + layout_byte_offset, seg->body.chars, seg->byte_count);
                          layout_byte_offset += seg->byte_count;
                          buffer_byte_offset += seg->byte_count;
                          bytes += seg->byte_count;
                        }
 		      else if (seg->type == &btk_text_right_mark_type ||
 			       seg->type == &btk_text_left_mark_type)
                        {
 			  /* If we have preedit string, break out of this loop - we'll almost
 			   * certainly have different attributes on the preedit string
 			   */

 			  if (layout->preedit_len > 0 &&
 			      _btk_text_btree_mark_is_insert (_btk_text_buffer_get_btree (layout->buffer),
 							     seg->body.mark.obj))
			    break;

 			  if (seg->body.mark.visible)
 			    {
			      cursor_byte_offsets = b_slist_prepend (cursor_byte_offsets, GINT_TO_POINTER (layout_byte_offset));
			      cursor_segs = b_slist_prepend (cursor_segs, seg);
			      if (_btk_text_btree_mark_is_insert (_btk_text_buffer_get_btree (layout->buffer),
								  seg->body.mark.obj))
				display->insert_index = layout_byte_offset;
			    }
                        }
		      else
			break;

 		      prev_seg = seg;
                      seg = seg->next;
                    }

 		  seg = prev_seg; /* Back up one */
                  add_generic_attrs (layout, &style->appearance,
                                     bytes,
                                     attrs, layout_byte_offset - bytes,
                                     size_only, TRUE);
                  add_text_attrs (layout, style, bytes, attrs,
                                  layout_byte_offset - bytes, size_only);
                }
              else if (seg->type == &btk_text_pixbuf_type)
                {
                  add_generic_attrs (layout,
                                     &style->appearance,
                                     seg->byte_count,
                                     attrs, layout_byte_offset,
                                     size_only, FALSE);
                  add_pixbuf_attrs (layout, display, style,
                                    seg, attrs, layout_byte_offset);
                  memcpy (text + layout_byte_offset, btk_text_unknown_char_utf8,
                          seg->byte_count);
                  layout_byte_offset += seg->byte_count;
                  buffer_byte_offset += seg->byte_count;
                }
              else if (seg->type == &btk_text_child_type)
                {
                  saw_widget = TRUE;
                  
                  add_generic_attrs (layout, &style->appearance,
                                     seg->byte_count,
                                     attrs, layout_byte_offset,
                                     size_only, FALSE);
                  add_child_attrs (layout, display, style,
                                   seg, attrs, layout_byte_offset);
                  memcpy (text + layout_byte_offset, btk_text_unknown_char_utf8,
                          seg->byte_count);
                  layout_byte_offset += seg->byte_count;
                  buffer_byte_offset += seg->byte_count;
                }
              else
                {
                  /* We don't know this segment type */
                  g_assert_not_reached ();
                }
              
            } /* if (segment was visible) */
          else
            {
              /* Invisible segment */
              buffer_byte_offset += seg->byte_count;
            }

          release_style (layout, style);
        }

      /* Toggles */
      else if (seg->type == &btk_text_toggle_on_type ||
               seg->type == &btk_text_toggle_off_type)
        {
          /* Style may have changed, drop our
             current cached style */
          invalidate_cached_style (layout);
	  /* Add the tag only after we have seen some non-toggle non-mark segment,
	   * otherwise the tag is already accounted for by _btk_text_btree_get_tags(). */
	  if (!initial_toggle_segments)
	    tags = tags_array_toggle_tag (tags, seg->body.toggle.info->tag);
        }

      /* Marks */
      else if (seg->type == &btk_text_right_mark_type ||
               seg->type == &btk_text_left_mark_type)
        {
	  gint cursor_offset = 0;
 	  
	  /* At the insertion point, add the preedit string, if any */
	  
	  if (_btk_text_btree_mark_is_insert (_btk_text_buffer_get_btree (layout->buffer),
					     seg->body.mark.obj))
	    {
	      display->insert_index = layout_byte_offset;
	      
	      if (layout->preedit_len > 0)
		{
		  text_allocated += layout->preedit_len;
		  text = g_realloc (text, text_allocated);

		  style = get_style (layout, tags);
		  add_preedit_attrs (layout, style, attrs, layout_byte_offset, size_only);
		  release_style (layout, style);
                  
		  memcpy (text + layout_byte_offset, layout->preedit_string, layout->preedit_len);
		  layout_byte_offset += layout->preedit_len;
                  /* DO NOT increment the buffer byte offset for preedit */
                  
		  cursor_offset = layout->preedit_cursor - layout->preedit_len;
		}
	    }
	  

          /* Display visible marks */

          if (seg->body.mark.visible)
            {
              cursor_byte_offsets = b_slist_prepend (cursor_byte_offsets,
                                                     GINT_TO_POINTER (layout_byte_offset + cursor_offset));
              cursor_segs = b_slist_prepend (cursor_segs, seg);
            }
        }

      else
        g_error ("Unknown segment type: %s", seg->type->name);

      seg = seg->next;
    }
  
  if (!para_values_set)
    {
      style = get_style (layout, tags);
      set_para_values (layout, base_dir, style, display);
      release_style (layout, style);
    }
  
  /* Bango doesn't want the trailing paragraph delimiters */

  {
    /* Only one character has type G_UNICODE_PARAGRAPH_SEPARATOR in
     * Unicode 3.0; update this if that changes.
     */
#define PARAGRAPH_SEPARATOR 0x2029
    gunichar ch = 0;

    if (layout_byte_offset > 0)
      {
        const char *prev = g_utf8_prev_char (text + layout_byte_offset);
        ch = g_utf8_get_char (prev);
        if (ch == PARAGRAPH_SEPARATOR || ch == '\r' || ch == '\n')
          layout_byte_offset = prev - text; /* chop off */

        if (ch == '\n' && layout_byte_offset > 0)
          {
            /* Possibly chop a CR as well */
            prev = g_utf8_prev_char (text + layout_byte_offset);
            if (*prev == '\r')
              --layout_byte_offset;
          }
      }
  }
  
  bango_layout_set_text (display->layout, text, layout_byte_offset);
  bango_layout_set_attributes (display->layout, attrs);

  tmp_list1 = cursor_byte_offsets;
  tmp_list2 = cursor_segs;
  while (tmp_list1)
    {
      add_cursor (layout, display, tmp_list2->data,
                  GPOINTER_TO_INT (tmp_list1->data));
      tmp_list1 = tmp_list1->next;
      tmp_list2 = tmp_list2->next;
    }
  b_slist_free (cursor_byte_offsets);
  b_slist_free (cursor_segs);

  bango_layout_get_extents (display->layout, NULL, &extents);

  display->width = PIXEL_BOUND (extents.width) + display->left_margin + display->right_margin;
  display->height += BANGO_PIXELS (extents.height);

  /* If we aren't wrapping, we need to do the alignment of each
   * paragraph ourselves.
   */
  if (bango_layout_get_width (display->layout) < 0)
    {
      gint excess = display->total_width - display->width;

      switch (bango_layout_get_alignment (display->layout))
	{
	case BANGO_ALIGN_LEFT:
	  break;
	case BANGO_ALIGN_CENTER:
	  display->x_offset += excess / 2;
	  break;
	case BANGO_ALIGN_RIGHT:
	  display->x_offset += excess;
	  break;
	}
    }
  
  /* Free this if we aren't in a loop */
  if (layout->wrap_loop_count == 0)
    invalidate_cached_style (layout);

  g_free (text);
  bango_attr_list_unref (attrs);
  if (tags != NULL)
    g_ptr_array_free (tags, TRUE);

  layout->one_display_cache = display;

  if (saw_widget)
    allocate_child_widgets (layout, display);
  
  return display;
}

void
btk_text_layout_free_line_display (BtkTextLayout      *layout,
                                   BtkTextLineDisplay *display)
{
  if (display != layout->one_display_cache)
    {
      if (display->layout)
        g_object_unref (display->layout);

      if (display->cursors)
        {
          b_slist_foreach (display->cursors, (GFunc)g_free, NULL);
          b_slist_free (display->cursors);
        }
      b_slist_free (display->shaped_objects);
      
      if (display->pg_bg_color)
        bdk_color_free (display->pg_bg_color);

      g_free (display);
    }
}

/* Functions to convert iter <=> index for the line of a BtkTextLineDisplay
 * taking into account the preedit string and invisible text if necessary.
 */
static gint
line_display_iter_to_index (BtkTextLayout      *layout,
			    BtkTextLineDisplay *display,
			    const BtkTextIter  *iter)
{
  gint index;

  g_return_val_if_fail (_btk_text_iter_get_text_line (iter) == display->line, 0);

  index = btk_text_iter_get_visible_line_index (iter);
  
  if (layout->preedit_len > 0 && display->insert_index >= 0)
    {
      if (index >= display->insert_index)
	index += layout->preedit_len;
    }

  return index;
}

static void
line_display_index_to_iter (BtkTextLayout      *layout,
			    BtkTextLineDisplay *display,
			    BtkTextIter        *iter,
			    gint                index,
			    gint                trailing)
{
  g_return_if_fail (!_btk_text_line_is_last (display->line,
                                             _btk_text_buffer_get_btree (layout->buffer)));
  
  if (layout->preedit_len > 0 && display->insert_index >= 0)
    {
      if (index >= display->insert_index + layout->preedit_len)
	index -= layout->preedit_len;
      else if (index > display->insert_index)
	{
	  index = display->insert_index;
	  trailing = 0;
	}
    }

  _btk_text_btree_get_iter_at_line (_btk_text_buffer_get_btree (layout->buffer),
                                    iter, display->line, 0);

  btk_text_iter_set_visible_line_index (iter, index);
  
  if (_btk_text_iter_get_text_line (iter) != display->line)
    {
      /* Clamp to end of line - really this clamping should have been done
       * before here, maybe in Bango, this is a broken band-aid I think
       */
      _btk_text_btree_get_iter_at_line (_btk_text_buffer_get_btree (layout->buffer),
                                        iter, display->line, 0);

      if (!btk_text_iter_ends_line (iter))
        btk_text_iter_forward_to_line_end (iter);
    }
  
  btk_text_iter_forward_chars (iter, trailing);
}

static void
get_line_at_y (BtkTextLayout *layout,
               gint           y,
               BtkTextLine  **line,
               gint          *line_top)
{
  if (y < 0)
    y = 0;
  if (y > layout->height)
    y = layout->height;

  *line = _btk_text_btree_find_line_by_y (_btk_text_buffer_get_btree (layout->buffer),
                                         layout, y, line_top);
  if (*line == NULL)
    {
      *line = _btk_text_btree_get_end_iter_line (_btk_text_buffer_get_btree (layout->buffer));
      
      if (line_top)
        *line_top =
          _btk_text_btree_find_line_top (_btk_text_buffer_get_btree (layout->buffer),
                                        *line, layout);
    }
}

/**
 * btk_text_layout_get_line_at_y:
 * @layout: a #BtkLayout
 * @target_iter: the iterator in which the result is stored
 * @y: the y positition
 * @line_top: location to store the y coordinate of the
 *            top of the line. (Can by %NULL)
 *
 * Get the iter at the beginning of the line which is displayed
 * at the given y.
 */
void
btk_text_layout_get_line_at_y (BtkTextLayout *layout,
                               BtkTextIter   *target_iter,
                               gint           y,
                               gint          *line_top)
{
  BtkTextLine *line;

  g_return_if_fail (BTK_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (target_iter != NULL);

  get_line_at_y (layout, y, &line, line_top);
  _btk_text_btree_get_iter_at_line (_btk_text_buffer_get_btree (layout->buffer),
                                   target_iter, line, 0);
}

void
btk_text_layout_get_iter_at_pixel (BtkTextLayout *layout,
                                   BtkTextIter   *target_iter,
                                   gint           x, 
				   gint           y)
{
  gint trailing;

  btk_text_layout_get_iter_at_position (layout, target_iter, &trailing, x, y);

  btk_text_iter_forward_chars (target_iter, trailing);  
}

void btk_text_layout_get_iter_at_position (BtkTextLayout     *layout,
					   BtkTextIter       *target_iter,
					   gint              *trailing,
					   gint               x,
					   gint               y)
{
  BtkTextLine *line;
  gint byte_index;
  gint line_top;
  BtkTextLineDisplay *display;

  g_return_if_fail (BTK_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (target_iter != NULL);

  get_line_at_y (layout, y, &line, &line_top);

  display = btk_text_layout_get_line_display (layout, line, FALSE);

  x -= display->x_offset;
  y -= line_top + display->top_margin;

  /* If we are below the layout, position the cursor at the last character
   * of the line.
   */
  if (y > display->height - display->top_margin - display->bottom_margin)
    {
      byte_index = _btk_text_line_byte_count (line);
      if (trailing)
        *trailing = 0;
    }
  else
    {
       /* Ignore the "outside" return value from bango. Bango is doing
        * the right thing even if we are outside the layout in the
        * x-direction.
        */
      bango_layout_xy_to_index (display->layout, x * BANGO_SCALE, y * BANGO_SCALE,
                                &byte_index, trailing);
    }

  line_display_index_to_iter (layout, display, target_iter, byte_index, 0);

  btk_text_layout_free_line_display (layout, display);
}


/**
 * btk_text_layout_get_cursor_locations:
 * @layout: a #BtkTextLayout
 * @iter: a #BtkTextIter
 * @strong_pos: (allow-none): location to store the strong cursor position (may be %NULL)
 * @weak_pos: (allow-none): location to store the weak cursor position (may be %NULL)
 *
 * Given an iterator within a text layout, determine the positions of the
 * strong and weak cursors if the insertion point is at that
 * iterator. The position of each cursor is stored as a zero-width
 * rectangle. The strong cursor location is the location where
 * characters of the directionality equal to the base direction of the
 * paragraph are inserted.  The weak cursor location is the location
 * where characters of the directionality opposite to the base
 * direction of the paragraph are inserted.
 **/
void
btk_text_layout_get_cursor_locations (BtkTextLayout  *layout,
                                      BtkTextIter    *iter,
                                      BdkRectangle   *strong_pos,
                                      BdkRectangle   *weak_pos)
{
  BtkTextLine *line;
  BtkTextLineDisplay *display;
  gint line_top;
  gint index;
  BtkTextIter insert_iter;

  BangoRectangle bango_strong_pos;
  BangoRectangle bango_weak_pos;

  g_return_if_fail (layout != NULL);
  g_return_if_fail (iter != NULL);

  line = _btk_text_iter_get_text_line (iter);
  display = btk_text_layout_get_line_display (layout, line, FALSE);
  index = line_display_iter_to_index (layout, display, iter);
  
  line_top = _btk_text_btree_find_line_top (_btk_text_buffer_get_btree (layout->buffer),
                                           line, layout);
  
  btk_text_buffer_get_iter_at_mark (layout->buffer, &insert_iter,
                                    btk_text_buffer_get_insert (layout->buffer));

  if (btk_text_iter_equal (iter, &insert_iter))
    index += layout->preedit_cursor - layout->preedit_len;
  
  bango_layout_get_cursor_pos (display->layout, index,
			       strong_pos ? &bango_strong_pos : NULL,
			       weak_pos ? &bango_weak_pos : NULL);

  if (strong_pos)
    {
      strong_pos->x = display->x_offset + bango_strong_pos.x / BANGO_SCALE;
      strong_pos->y = line_top + display->top_margin + bango_strong_pos.y / BANGO_SCALE;
      strong_pos->width = 0;
      strong_pos->height = bango_strong_pos.height / BANGO_SCALE;
    }

  if (weak_pos)
    {
      weak_pos->x = display->x_offset + bango_weak_pos.x / BANGO_SCALE;
      weak_pos->y = line_top + display->top_margin + bango_weak_pos.y / BANGO_SCALE;
      weak_pos->width = 0;
      weak_pos->height = bango_weak_pos.height / BANGO_SCALE;
    }

  btk_text_layout_free_line_display (layout, display);
}

/**
 * _btk_text_layout_get_block_cursor:
 * @layout: a #BtkTextLayout
 * @pos: a #BdkRectangle to store block cursor position
 *
 * If layout is to display a block cursor, calculates its position
 * and returns %TRUE. Otherwise it returns %FALSE. In case when
 * cursor is visible, it simply returns the position stored in
 * the line display, otherwise it has to compute the position
 * (see get_block_cursor()).
 **/
gboolean
_btk_text_layout_get_block_cursor (BtkTextLayout *layout,
				   BdkRectangle  *pos)
{
  BtkTextLine *line;
  BtkTextLineDisplay *display;
  BtkTextIter iter;
  BdkRectangle rect;
  gboolean block = FALSE;

  g_return_val_if_fail (layout != NULL, FALSE);

  btk_text_buffer_get_iter_at_mark (layout->buffer, &iter,
                                    btk_text_buffer_get_insert (layout->buffer));
  line = _btk_text_iter_get_text_line (&iter);
  display = btk_text_layout_get_line_display (layout, line, FALSE);

  if (display->has_block_cursor)
    {
      block = TRUE;
      rect = display->block_cursor;
    }
  else
    {
      gint index = display->insert_index;

      if (index < 0)
        index = btk_text_iter_get_line_index (&iter);

      if (get_block_cursor (layout, display, &iter, index, &rect, NULL))
	block = TRUE;
    }

  if (block && pos)
    {
      gint line_top;

      line_top = _btk_text_btree_find_line_top (_btk_text_buffer_get_btree (layout->buffer),
						line, layout);

      *pos = rect;
      pos->x += display->x_offset;
      pos->y += line_top + display->top_margin;
    }

  btk_text_layout_free_line_display (layout, display);
  return block;
}

/**
 * btk_text_layout_get_line_yrange:
 * @layout: a #BtkTextLayout
 * @iter:   a #BtkTextIter
 * @y:      location to store the top of the paragraph in pixels,
 *          or %NULL.
 * @height  location to store the height of the paragraph in pixels,
 *          or %NULL.
 *
 * Find the range of y coordinates for the paragraph containing
 * the given iter.
 **/
void
btk_text_layout_get_line_yrange (BtkTextLayout     *layout,
                                 const BtkTextIter *iter,
                                 gint              *y,
                                 gint              *height)
{
  BtkTextLine *line;

  g_return_if_fail (BTK_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (_btk_text_iter_get_btree (iter) == _btk_text_buffer_get_btree (layout->buffer));

  line = _btk_text_iter_get_text_line (iter);

  if (y)
    *y = _btk_text_btree_find_line_top (_btk_text_buffer_get_btree (layout->buffer),
                                       line, layout);
  if (height)
    {
      BtkTextLineData *line_data = _btk_text_line_get_data (line, layout);
      if (line_data)
        *height = line_data->height;
      else
        *height = 0;
    }
}

/**
 * _btk_text_layout_get_line_xrange:
 * @layout: a #BtkTextLayout
 * @iter:   a #BtkTextIter
 * @x:      location to store the top of the paragraph in pixels,
 *          or %NULL.
 * @width  location to store the height of the paragraph in pixels,
 *          or %NULL.
 *
 * Find the range of X coordinates for the paragraph containing
 * the given iter. Private for 2.0 due to API freeze, could
 * be made public for 2.2.
 **/
void
_btk_text_layout_get_line_xrange (BtkTextLayout     *layout,
                                  const BtkTextIter *iter,
                                  gint              *x,
                                  gint              *width)
{
  BtkTextLine *line;

  g_return_if_fail (BTK_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (_btk_text_iter_get_btree (iter) == _btk_text_buffer_get_btree (layout->buffer));

  line = _btk_text_iter_get_text_line (iter);

  if (x)
    *x = 0; /* FIXME This is wrong; should represent the first available cursor position */
  
  if (width)
    {
      BtkTextLineData *line_data = _btk_text_line_get_data (line, layout);
      if (line_data)
        *width = line_data->width;
      else
        *width = 0;
    }
}

void
btk_text_layout_get_iter_location (BtkTextLayout     *layout,
                                   const BtkTextIter *iter,
                                   BdkRectangle      *rect)
{
  BangoRectangle bango_rect;
  BtkTextLine *line;
  BtkTextBTree *tree;
  BtkTextLineDisplay *display;
  gint byte_index;
  gint x_offset;
  
  g_return_if_fail (BTK_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (_btk_text_iter_get_btree (iter) == _btk_text_buffer_get_btree (layout->buffer));
  g_return_if_fail (rect != NULL);

  tree = _btk_text_iter_get_btree (iter);
  line = _btk_text_iter_get_text_line (iter);

  display = btk_text_layout_get_line_display (layout, line, FALSE);

  rect->y = _btk_text_btree_find_line_top (tree, line, layout);

  x_offset = display->x_offset * BANGO_SCALE;

  byte_index = btk_text_iter_get_line_index (iter);
  
  bango_layout_index_to_pos (display->layout, byte_index, &bango_rect);
  
  rect->x = BANGO_PIXELS (x_offset + bango_rect.x);
  rect->y += BANGO_PIXELS (bango_rect.y) + display->top_margin;
  rect->width = BANGO_PIXELS (bango_rect.width);
  rect->height = BANGO_PIXELS (bango_rect.height);

  btk_text_layout_free_line_display (layout, display);
}

/* FFIXX */

/* Find the iter for the logical beginning of the first display line whose
 * top y is >= y. If none exists, move the iter to the logical beginning
 * of the last line in the buffer.
 */
static void
find_display_line_below (BtkTextLayout *layout,
                         BtkTextIter   *iter,
                         gint           y)
{
  BtkTextLine *line, *next;
  BtkTextLine *found_line = NULL;
  gint line_top;
  gint found_byte = 0;

  line = _btk_text_btree_find_line_by_y (_btk_text_buffer_get_btree (layout->buffer),
                                        layout, y, &line_top);
  if (!line)
    {
      line =
        _btk_text_btree_get_end_iter_line (_btk_text_buffer_get_btree (layout->buffer));

      line_top =
        _btk_text_btree_find_line_top (_btk_text_buffer_get_btree (layout->buffer),
                                      line, layout);
    }

  while (line && !found_line)
    {
      BtkTextLineDisplay *display = btk_text_layout_get_line_display (layout, line, FALSE);
      BangoLayoutIter *layout_iter;

      layout_iter = bango_layout_get_iter (display->layout);

      line_top += display->top_margin;

      do
        {
          gint first_y, last_y;
          BangoLayoutLine *layout_line = bango_layout_iter_get_line_readonly (layout_iter);

          found_byte = layout_line->start_index;
          
          if (line_top >= y)
            {
              found_line = line;
              break;
            }

          bango_layout_iter_get_line_yrange (layout_iter, &first_y, &last_y);
          line_top += (last_y - first_y) / BANGO_SCALE;
        }
      while (bango_layout_iter_next_line (layout_iter));

      bango_layout_iter_free (layout_iter);
      
      line_top += display->bottom_margin;
      btk_text_layout_free_line_display (layout, display);

      next = _btk_text_line_next_excluding_last (line);
      if (!next)
        found_line = line;

      line = next;
    }

  _btk_text_btree_get_iter_at_line (_btk_text_buffer_get_btree (layout->buffer),
                                   iter, found_line, found_byte);
}

/* Find the iter for the logical beginning of the last display line whose
 * top y is >= y. If none exists, move the iter to the logical beginning
 * of the first line in the buffer.
 */
static void
find_display_line_above (BtkTextLayout *layout,
                         BtkTextIter   *iter,
                         gint           y)
{
  BtkTextLine *line;
  BtkTextLine *found_line = NULL;
  gint line_top;
  gint found_byte = 0;

  line = _btk_text_btree_find_line_by_y (_btk_text_buffer_get_btree (layout->buffer), layout, y, &line_top);
  if (!line)
    {
      line = _btk_text_btree_get_end_iter_line (_btk_text_buffer_get_btree (layout->buffer));
      
      line_top = _btk_text_btree_find_line_top (_btk_text_buffer_get_btree (layout->buffer), line, layout);
    }

  while (line && !found_line)
    {
      BtkTextLineDisplay *display = btk_text_layout_get_line_display (layout, line, FALSE);
      BangoRectangle logical_rect;
      BangoLayoutIter *layout_iter;
      gint tmp_top;

      layout_iter = bango_layout_get_iter (display->layout);
      
      line_top -= display->top_margin + display->bottom_margin;
      bango_layout_iter_get_layout_extents (layout_iter, NULL, &logical_rect);
      line_top -= logical_rect.height / BANGO_SCALE;

      tmp_top = line_top + display->top_margin;

      do
        {
          gint first_y, last_y;
          BangoLayoutLine *layout_line = bango_layout_iter_get_line_readonly (layout_iter);

          found_byte = layout_line->start_index;

          bango_layout_iter_get_line_yrange (layout_iter, &first_y, &last_y);
          
          tmp_top -= (last_y - first_y) / BANGO_SCALE;

          if (tmp_top < y)
            {
              found_line = line;
	      bango_layout_iter_free (layout_iter);
              goto done;
            }
        }
      while (bango_layout_iter_next_line (layout_iter));

      bango_layout_iter_free (layout_iter);
      
      btk_text_layout_free_line_display (layout, display);

      line = _btk_text_line_previous (line);
    }

 done:
  
  if (found_line)
    _btk_text_btree_get_iter_at_line (_btk_text_buffer_get_btree (layout->buffer),
                                     iter, found_line, found_byte);
  else
    btk_text_buffer_get_iter_at_offset (layout->buffer, iter, 0);
}

/**
 * btk_text_layout_clamp_iter_to_vrange:
 * @layout: a #BtkTextLayout
 * @iter:   a #BtkTextIter
 * @top:    the top of the range
 * @bottom: the bottom the range
 *
 * If the iterator is not fully in the range @top <= y < @bottom,
 * then, if possible, move it the minimum distance so that the
 * iterator in this range.
 *
 * Returns: %TRUE if the iterator was moved, otherwise %FALSE.
 **/
gboolean
btk_text_layout_clamp_iter_to_vrange (BtkTextLayout *layout,
                                      BtkTextIter   *iter,
                                      gint           top,
                                      gint           bottom)
{
  BdkRectangle iter_rect;

  btk_text_layout_get_iter_location (layout, iter, &iter_rect);

  /* If the iter is at least partially above the range, put the iter
   * at the first fully visible line after the range.
   */
  if (iter_rect.y < top)
    {
      find_display_line_below (layout, iter, top);

      return TRUE;
    }
  /* Otherwise, if the iter is at least partially below the screen, put the
   * iter on the last logical position of the last completely visible
   * line on screen
   */
  else if (iter_rect.y + iter_rect.height > bottom)
    {
      find_display_line_above (layout, iter, bottom);

      return TRUE;
    }
  else
    return FALSE;
}

/**
 * btk_text_layout_move_iter_to_previous_line:
 * @layout: a #BtkLayout
 * @iter:   a #BtkTextIter
 *
 * Move the iterator to the beginning of the previous line. The lines
 * of a wrapped paragraph are treated as distinct for this operation.
 **/
gboolean
btk_text_layout_move_iter_to_previous_line (BtkTextLayout *layout,
                                            BtkTextIter   *iter)
{
  BtkTextLine *line;
  BtkTextLineDisplay *display;
  gint line_byte;
  GSList *tmp_list;
  BangoLayoutLine *layout_line;
  BtkTextIter orig;
  gboolean update_byte = FALSE;
  
  g_return_val_if_fail (BTK_IS_TEXT_LAYOUT (layout), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  orig = *iter;


  line = _btk_text_iter_get_text_line (iter);
  display = btk_text_layout_get_line_display (layout, line, FALSE);
  line_byte = line_display_iter_to_index (layout, display, iter);

  /* If display->height == 0 then the line is invisible, so don't
   * move onto it.
   */
  while (display->height == 0)
    {
      BtkTextLine *prev_line;

      prev_line = _btk_text_line_previous (line);

      if (prev_line == NULL)
        {
          line_display_index_to_iter (layout, display, iter, 0, 0);
          goto out;
        }

      btk_text_layout_free_line_display (layout, display);

      line = prev_line;
      display = btk_text_layout_get_line_display (layout, prev_line, FALSE);
      update_byte = TRUE;
    }
  
  tmp_list = bango_layout_get_lines_readonly (display->layout);
  layout_line = tmp_list->data;

  if (update_byte)
    {
      line_byte = layout_line->start_index + layout_line->length;
    }

  if (line_byte < layout_line->length || !tmp_list->next) /* first line of paragraph */
    {
      BtkTextLine *prev_line;

      prev_line = _btk_text_line_previous (line);

      /* first line of the whole buffer, do not move the iter and return FALSE */
      if (prev_line == NULL)
        goto out;

      while (prev_line)
        {
          btk_text_layout_free_line_display (layout, display);

          display = btk_text_layout_get_line_display (layout, prev_line, FALSE);

          if (display->height > 0)
            {
              tmp_list = b_slist_last (bango_layout_get_lines_readonly (display->layout));
              layout_line = tmp_list->data;

              line_display_index_to_iter (layout, display, iter,
                                          layout_line->start_index + layout_line->length, 0);
              break;
            }

          prev_line = _btk_text_line_previous (prev_line);
        }
    }
  else
    {
      gint prev_offset = layout_line->start_index;

      tmp_list = tmp_list->next;
      while (tmp_list)
        {
          layout_line = tmp_list->data;

          if (line_byte < layout_line->start_index + layout_line->length ||
              !tmp_list->next)
            {
 	      line_display_index_to_iter (layout, display, iter, prev_offset, 0);
              break;
            }

          prev_offset = layout_line->start_index;
          tmp_list = tmp_list->next;
        }
    }

 out:
  
  btk_text_layout_free_line_display (layout, display);

  return
    !btk_text_iter_equal (iter, &orig) &&
    !btk_text_iter_is_end (iter);
}

/**
 * btk_text_layout_move_iter_to_next_line:
 * @layout: a #BtkLayout
 * @iter:   a #BtkTextIter
 *
 * Move the iterator to the beginning of the next line. The
 * lines of a wrapped paragraph are treated as distinct for
 * this operation.
 **/
gboolean
btk_text_layout_move_iter_to_next_line (BtkTextLayout *layout,
                                        BtkTextIter   *iter)
{
  BtkTextLine *line;
  BtkTextLineDisplay *display;
  gint line_byte;
  BtkTextIter orig;
  gboolean found = FALSE;
  gboolean found_after = FALSE;
  gboolean first = TRUE;

  g_return_val_if_fail (BTK_IS_TEXT_LAYOUT (layout), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  orig = *iter;
  
  line = _btk_text_iter_get_text_line (iter);

  while (line && !found_after)
    {
      GSList *tmp_list;

      display = btk_text_layout_get_line_display (layout, line, FALSE);

      if (display->height == 0)
        goto next;
      
      if (first)
	{
	  line_byte = line_display_iter_to_index (layout, display, iter);
	  first = FALSE;
	}
      else
	line_byte = 0;
	
      tmp_list = bango_layout_get_lines_readonly (display->layout);
      while (tmp_list && !found_after)
        {
          BangoLayoutLine *layout_line = tmp_list->data;

          if (found)
            {
	      line_display_index_to_iter (layout, display, iter,
                                          layout_line->start_index, 0);
              found_after = TRUE;
            }
          else if (line_byte < layout_line->start_index + layout_line->length || !tmp_list->next)
            found = TRUE;
          
          tmp_list = tmp_list->next;
        }

    next:
      
      btk_text_layout_free_line_display (layout, display);

      line = _btk_text_line_next_excluding_last (line);
    }

  if (!found_after)
    btk_text_buffer_get_end_iter (layout->buffer, iter);
  
  return
    !btk_text_iter_equal (iter, &orig) &&
    !btk_text_iter_is_end (iter);
}

/**
 * btk_text_layout_move_iter_to_line_end:
 * @layout: a #BtkTextLayout
 * @direction: if negative, move to beginning of line, otherwise
               move to end of line.
 *
 * Move to the beginning or end of a display line.
 **/
gboolean
btk_text_layout_move_iter_to_line_end (BtkTextLayout *layout,
                                       BtkTextIter   *iter,
                                       gint           direction)
{
  BtkTextLine *line;
  BtkTextLineDisplay *display;
  gint line_byte;
  GSList *tmp_list;
  BtkTextIter orig;
  
  g_return_val_if_fail (BTK_IS_TEXT_LAYOUT (layout), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  orig = *iter;
  
  line = _btk_text_iter_get_text_line (iter);
  display = btk_text_layout_get_line_display (layout, line, FALSE);
  line_byte = line_display_iter_to_index (layout, display, iter);

  tmp_list = bango_layout_get_lines_readonly (display->layout);
  while (tmp_list)
    {
      BangoLayoutLine *layout_line = tmp_list->data;

      if (line_byte < layout_line->start_index + layout_line->length || !tmp_list->next)
        {
 	  line_display_index_to_iter (layout, display, iter,
 				      direction < 0 ? layout_line->start_index : layout_line->start_index + layout_line->length,
 				      0);

          /* FIXME: As a bad hack, we move back one position when we
	   * are inside a paragraph to avoid going to next line on a
	   * forced break not at whitespace. Real fix is to keep track
	   * of whether marks are at leading or trailing edge?  */
          if (direction > 0 && layout_line->length > 0 && 
	      !btk_text_iter_ends_line (iter) && 
	      !_btk_text_btree_char_is_invisible (iter))
            btk_text_iter_backward_char (iter);
          break;
        }
      
      tmp_list = tmp_list->next;
    }

  btk_text_layout_free_line_display (layout, display);

  return
    !btk_text_iter_equal (iter, &orig) &&
    !btk_text_iter_is_end (iter);
}


/**
 * btk_text_layout_iter_starts_line:
 * @layout: a #BtkTextLayout
 * @iter: iterator to test
 *
 * Tests whether an iterator is at the start of a display line.
 **/
gboolean
btk_text_layout_iter_starts_line (BtkTextLayout       *layout,
                                  const BtkTextIter   *iter)
{
  BtkTextLine *line;
  BtkTextLineDisplay *display;
  gint line_byte;
  GSList *tmp_list;
  
  g_return_val_if_fail (BTK_IS_TEXT_LAYOUT (layout), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  line = _btk_text_iter_get_text_line (iter);
  display = btk_text_layout_get_line_display (layout, line, FALSE);
  line_byte = line_display_iter_to_index (layout, display, iter);

  tmp_list = bango_layout_get_lines_readonly (display->layout);
  while (tmp_list)
    {
      BangoLayoutLine *layout_line = tmp_list->data;

      if (line_byte < layout_line->start_index + layout_line->length ||
          !tmp_list->next)
        {
          /* We're located on this line or the para delimiters before
           * it
           */
          btk_text_layout_free_line_display (layout, display);
          
          if (line_byte == layout_line->start_index)
            return TRUE;
          else
            return FALSE;
        }
      
      tmp_list = tmp_list->next;
    }

  g_assert_not_reached ();
  return FALSE;
}

void
btk_text_layout_get_iter_at_line (BtkTextLayout  *layout,
                                  BtkTextIter    *iter,
                                  BtkTextLine    *line,
                                  gint            byte_offset)
{
  _btk_text_btree_get_iter_at_line (_btk_text_buffer_get_btree (layout->buffer),
                                    iter, line, byte_offset);
}


/**
 * btk_text_layout_move_iter_to_x:
 * @layout: a #BtkTextLayout
 * @iter:   a #BtkTextIter
 * @x:      X coordinate
 *
 * Keeping the iterator on the same line of the layout, move it to the
 * specified X coordinate. The lines of a wrapped paragraph are
 * treated as distinct for this operation.
 **/
void
btk_text_layout_move_iter_to_x (BtkTextLayout *layout,
                                BtkTextIter   *iter,
                                gint           x)
{
  BtkTextLine *line;
  BtkTextLineDisplay *display;
  gint line_byte;
  BangoLayoutIter *layout_iter;
  
  g_return_if_fail (BTK_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (iter != NULL);

  line = _btk_text_iter_get_text_line (iter);

  display = btk_text_layout_get_line_display (layout, line, FALSE);
  line_byte = line_display_iter_to_index (layout, display, iter);

  layout_iter = bango_layout_get_iter (display->layout);

  do
    {
      BangoLayoutLine *layout_line = bango_layout_iter_get_line_readonly (layout_iter);

      if (line_byte < layout_line->start_index + layout_line->length ||
          bango_layout_iter_at_last_line (layout_iter))
        {
          BangoRectangle logical_rect;
          gint byte_index, trailing;
          gint x_offset = display->x_offset * BANGO_SCALE;

          bango_layout_iter_get_line_extents (layout_iter, NULL, &logical_rect);

          bango_layout_line_x_to_index (layout_line,
                                        x * BANGO_SCALE - x_offset - logical_rect.x,
                                        &byte_index, &trailing);

 	  line_display_index_to_iter (layout, display, iter, byte_index, trailing);

          break;
        }
    }
  while (bango_layout_iter_next_line (layout_iter));

  bango_layout_iter_free (layout_iter);
  
  btk_text_layout_free_line_display (layout, display);
}

/**
 * btk_text_layout_move_iter_visually:
 * @layout:  a #BtkTextLayout
 * @iter:    a #BtkTextIter
 * @count:   number of characters to move (negative moves left, positive moves right)
 *
 * Move the iterator a given number of characters visually, treating
 * it as the strong cursor position. If @count is positive, then the
 * new strong cursor position will be @count positions to the right of
 * the old cursor position. If @count is negative then the new strong
 * cursor position will be @count positions to the left of the old
 * cursor position.
 *
 * In the presence of bidirection text, the correspondence
 * between logical and visual order will depend on the direction
 * of the current run, and there may be jumps when the cursor
 * is moved off of the end of a run.
 **/

gboolean
btk_text_layout_move_iter_visually (BtkTextLayout *layout,
                                    BtkTextIter   *iter,
                                    gint           count)
{
  BtkTextLineDisplay *display = NULL;
  BtkTextIter orig;
  BtkTextIter lineiter;
  
  g_return_val_if_fail (layout != NULL, FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  orig = *iter;
  
  while (count != 0)
    {
      BtkTextLine *line = _btk_text_iter_get_text_line (iter);
      gint line_byte;
      gint extra_back = 0;
      gboolean strong;

      int byte_count = _btk_text_line_byte_count (line);

      int new_index;
      int new_trailing;

      if (!display)
	display = btk_text_layout_get_line_display (layout, line, FALSE);

      if (layout->cursor_direction == BTK_TEXT_DIR_NONE)
	strong = TRUE;
      else
	strong = display->direction == layout->cursor_direction;

      line_byte = line_display_iter_to_index (layout, display, iter);

      if (count > 0)
        {
          bango_layout_move_cursor_visually (display->layout, strong, line_byte, 0, 1, &new_index, &new_trailing);
          count--;
        }
      else
        {
          bango_layout_move_cursor_visually (display->layout, strong, line_byte, 0, -1, &new_index, &new_trailing);
          count++;
        }

      /* We need to handle the preedit string specially. Well, we don't really need to
       * handle it specially, since hopefully calling btk_im_context_reset() will
       * remove the preedit string; but if we start off in front of the preedit
       * string (logically) and end up in or on the back edge of the preedit string,
       * we should move the iter one place farther.
       */
      if (layout->preedit_len > 0 && display->insert_index >= 0)
	{
	  if (line_byte == display->insert_index + layout->preedit_len &&
	      new_index < display->insert_index + layout->preedit_len)
	    {
	      line_byte = display->insert_index;
	      extra_back = 1;
	    }
	}
      
      if (new_index < 0 || (new_index == 0 && extra_back))
        {
          do
            {
              line = _btk_text_line_previous (line);

              if (!line)
                goto done;
              
              _btk_text_btree_get_iter_at_line (_btk_text_buffer_get_btree (layout->buffer),
                                                &lineiter, line, 0);
            }
          while (totally_invisible_line (layout, line, &lineiter));
          
 	  btk_text_layout_free_line_display (layout, display);
 	  display = btk_text_layout_get_line_display (layout, line, FALSE);
          btk_text_iter_forward_to_line_end (&lineiter);
          new_index = btk_text_iter_get_visible_line_index (&lineiter);
        }
      else if (new_index > byte_count)
        {
          do
            {
              line = _btk_text_line_next_excluding_last (line);
              if (!line)
                goto done;

              _btk_text_btree_get_iter_at_line (_btk_text_buffer_get_btree (layout->buffer),
                                                &lineiter, line, 0);
            }
          while (totally_invisible_line (layout, line, &lineiter));
  
 	  btk_text_layout_free_line_display (layout, display);
 	  display = btk_text_layout_get_line_display (layout, line, FALSE);
          new_index = 0;
        }
      
       line_display_index_to_iter (layout, display, iter, new_index, new_trailing);
       if (extra_back)
	 btk_text_iter_backward_char (iter);
    }

  btk_text_layout_free_line_display (layout, display);

 done:
  
  return
    !btk_text_iter_equal (iter, &orig) &&
    !btk_text_iter_is_end (iter);
}

void
btk_text_layout_spew (BtkTextLayout *layout)
{
#if 0
  BtkTextDisplayLine *iter;
  guint wrapped = 0;
  guint paragraphs = 0;
  BtkTextLine *last_line = NULL;

  iter = layout->line_list;
  while (iter != NULL)
    {
      if (iter->line != last_line)
        {
          printf ("%5u  paragraph (%p)\n", paragraphs, iter->line);
          ++paragraphs;
          last_line = iter->line;
        }

      printf ("  %5u  y: %d len: %d start: %d bytes: %d\n",
              wrapped, iter->y, iter->length, iter->byte_offset,
              iter->byte_count);

      ++wrapped;
      iter = iter->next;
    }

  printf ("Layout %s recompute\n",
          layout->need_recompute ? "needs" : "doesn't need");

  printf ("Layout pars: %u lines: %u size: %d x %d Screen width: %d\n",
          paragraphs, wrapped, layout->width,
          layout->height, layout->screen_width);
#endif
}

/* Catch all situations that move the insertion point.
 */
static void
btk_text_layout_mark_set_handler (BtkTextBuffer     *buffer,
                                  const BtkTextIter *location,
                                  BtkTextMark       *mark,
                                  gpointer           data)
{
  BtkTextLayout *layout = BTK_TEXT_LAYOUT (data);

  if (mark == btk_text_buffer_get_insert (buffer))
    btk_text_layout_update_cursor_line (layout);
}

static void
btk_text_layout_buffer_insert_text (BtkTextBuffer *textbuffer,
				    BtkTextIter   *iter,
				    gchar         *str,
				    gint           len,
				    gpointer       data)
{
  BtkTextLayout *layout = BTK_TEXT_LAYOUT (data);

  btk_text_layout_update_cursor_line (layout);
}

static void
btk_text_layout_buffer_delete_range (BtkTextBuffer *textbuffer,
				     BtkTextIter   *start,
				     BtkTextIter   *end,
				     gpointer       data)
{
  BtkTextLayout *layout = BTK_TEXT_LAYOUT (data);

  btk_text_layout_update_cursor_line (layout);
}

#define __BTK_TEXT_LAYOUT_C__
#include "btkaliasdef.c"
