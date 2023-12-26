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
 * Modified by the BTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"

#include "btktextview.h"
#include "btktextutil.h"

#define BTK_TEXT_USE_INTERNAL_UNSUPPORTED_API

#include "btktextdisplay.h"
#include "btktextbuffer.h"
#include "btkmenuitem.h"
#include "btkintl.h"
#include "btkalias.h"

#define DRAG_ICON_MAX_WIDTH 250
#define DRAG_ICON_MAX_HEIGHT 250
#define DRAG_ICON_LAYOUT_BORDER 5
#define DRAG_ICON_MAX_LINES 7
#define ELLIPSIS_CHARACTER "\xe2\x80\xa6"

typedef struct _BtkUnicodeMenuEntry BtkUnicodeMenuEntry;
typedef struct _BtkTextUtilCallbackInfo BtkTextUtilCallbackInfo;

struct _BtkUnicodeMenuEntry {
  const char *label;
  gunichar ch;
};

struct _BtkTextUtilCallbackInfo
{
  BtkTextUtilCharChosenFunc func;
  gpointer data;
};

static const BtkUnicodeMenuEntry bidi_menu_entries[] = {
  { N_("LRM _Left-to-right mark"), 0x200E },
  { N_("RLM _Right-to-left mark"), 0x200F },
  { N_("LRE Left-to-right _embedding"), 0x202A },
  { N_("RLE Right-to-left e_mbedding"), 0x202B },
  { N_("LRO Left-to-right _override"), 0x202D },
  { N_("RLO Right-to-left o_verride"), 0x202E },
  { N_("PDF _Pop directional formatting"), 0x202C },
  { N_("ZWS _Zero width space"), 0x200B },
  { N_("ZWJ Zero width _joiner"), 0x200D },
  { N_("ZWNJ Zero width _non-joiner"), 0x200C }
};

static BtkTextUtilCallbackInfo *
callback_info_new (BtkTextUtilCharChosenFunc  func,
                   gpointer                   data)
{
  BtkTextUtilCallbackInfo *info;

  info = g_slice_new (BtkTextUtilCallbackInfo);

  info->func = func;
  info->data = data;

  return info;
}

static void
callback_info_free (BtkTextUtilCallbackInfo *info)
{
  g_slice_free (BtkTextUtilCallbackInfo, info);
}

static void
activate_cb (BtkWidget *menu_item,
             gpointer   data)
{
  BtkUnicodeMenuEntry *entry;
  BtkTextUtilCallbackInfo *info = data;
  char buf[7];
  
  entry = g_object_get_data (B_OBJECT (menu_item), "btk-unicode-menu-entry");

  buf[g_unichar_to_utf8 (entry->ch, buf)] = '\0';
  
  (* info->func) (buf, info->data);
}

/*
 * _btk_text_util_append_special_char_menuitems
 * @menushell: a #BtkMenuShell
 * @callback:  call this when an item is chosen
 * @data: data for callback
 * 
 * Add menuitems for various bidi control characters  to a menu;
 * the menuitems, when selected, will call the given function
 * with the chosen character.
 *
 * This function is private/internal in BTK 2.0, the functionality may
 * become public sometime, but it probably needs more thought first.
 * e.g. maybe there should be a way to just get the list of items,
 * instead of requiring the menu items to be created.
 */
void
_btk_text_util_append_special_char_menuitems (BtkMenuShell              *menushell,
                                              BtkTextUtilCharChosenFunc  func,
                                              gpointer                   data)
{
  int i;

  for (i = 0; i < G_N_ELEMENTS (bidi_menu_entries); i++)
    {
      BtkWidget *menuitem;
      BtkTextUtilCallbackInfo *info;

      info = callback_info_new (func, data);

      menuitem = btk_menu_item_new_with_mnemonic (_(bidi_menu_entries[i].label));
      g_object_set_data (B_OBJECT (menuitem), I_("btk-unicode-menu-entry"),
                         (gpointer)&bidi_menu_entries[i]);

      g_signal_connect_data (menuitem, "activate",
                             G_CALLBACK (activate_cb),
                             info, (GClosureNotify) callback_info_free, 0);

      btk_widget_show (menuitem);
      btk_menu_shell_append (menushell, menuitem);
    }
}

static void
append_n_lines (GString *str, const gchar *text, GSList *lines, gint n_lines)
{
  BangoLayoutLine *line;
  gint i;

  for (i = 0; i < n_lines; i++)
    {
      line = lines->data;
      g_string_append_len (str, &text[line->start_index], line->length);
      lines = lines->next;
    }
}

static void
limit_layout_lines (BangoLayout *layout)
{
  const gchar *text;
  GString     *str;
  GSList      *lines, *elem;
  gint         n_lines;

  n_lines = bango_layout_get_line_count (layout);
  
  if (n_lines >= DRAG_ICON_MAX_LINES)
    {
      text  = bango_layout_get_text (layout);
      str   = g_string_new (NULL);
      lines = bango_layout_get_lines_readonly (layout);

      /* get first lines */
      elem = lines;
      append_n_lines (str, text, elem,
                      DRAG_ICON_MAX_LINES / 2);

      g_string_append (str, "\n" ELLIPSIS_CHARACTER "\n");

      /* get last lines */
      elem = b_slist_nth (lines, n_lines - DRAG_ICON_MAX_LINES / 2);
      append_n_lines (str, text, elem,
                      DRAG_ICON_MAX_LINES / 2);

      bango_layout_set_text (layout, str->str, -1);
      g_string_free (str, TRUE);
    }
}

/*
 * _btk_text_util_create_drag_icon
 * @widget: #BtkWidget to extract the bango context
 * @text: a #gchar to render the icon
 * @len: length of @text, or -1 for NUL-terminated text
 *
 * Creates a drag and drop icon from @text.
 *
 * Returns: a #BdkPixmap to use as DND icon
 */
BdkPixmap *
_btk_text_util_create_drag_icon (BtkWidget *widget, 
                                 gchar     *text,
                                 gsize      len)
{
  BdkDrawable  *drawable = NULL;
  BangoContext *context;
  BangoLayout  *layout;
  bairo_t      *cr;
  gint          pixmap_height, pixmap_width;
  gint          layout_width, layout_height;

  g_return_val_if_fail (widget != NULL, NULL);
  g_return_val_if_fail (text != NULL, NULL);

  context = btk_widget_get_bango_context (widget);
  layout  = bango_layout_new (context);

  bango_layout_set_text (layout, text, len);
  bango_layout_set_wrap (layout, BANGO_WRAP_WORD_CHAR);
  bango_layout_get_size (layout, &layout_width, &layout_height);

  layout_width = MIN (layout_width, DRAG_ICON_MAX_WIDTH * BANGO_SCALE);
  bango_layout_set_width (layout, layout_width);

  limit_layout_lines (layout);

  /* get again layout extents, they may have changed */
  bango_layout_get_size (layout, &layout_width, &layout_height);

  pixmap_width  = layout_width  / BANGO_SCALE + DRAG_ICON_LAYOUT_BORDER * 2;
  pixmap_height = layout_height / BANGO_SCALE + DRAG_ICON_LAYOUT_BORDER * 2;

  drawable = bdk_pixmap_new (widget->window,
                             pixmap_width  + 2,
                             pixmap_height + 2,
                             -1);
  cr = bdk_bairo_create (drawable);

  bdk_bairo_set_source_color (cr, &widget->style->base [btk_widget_get_state (widget)]);
  bairo_paint (cr);

  bdk_bairo_set_source_color (cr, &widget->style->text [btk_widget_get_state (widget)]);
  bairo_move_to (cr, 1 + DRAG_ICON_LAYOUT_BORDER, 1 + DRAG_ICON_LAYOUT_BORDER);
  bango_bairo_show_layout (cr, layout);

  bairo_set_source_rgb (cr, 0, 0, 0);
  bairo_rectangle (cr, 0.5, 0.5, pixmap_width + 1, pixmap_height + 1);
  bairo_set_line_width (cr, 1.0);
  bairo_stroke (cr);

  bairo_destroy (cr);
  g_object_unref (layout);

  return drawable;
}

static void
btk_text_view_set_attributes_from_style (BtkTextView        *text_view,
                                         BtkTextAttributes  *values,
                                         BtkStyle           *style)
{
  values->appearance.bg_color = style->base[BTK_STATE_NORMAL];
  values->appearance.fg_color = style->text[BTK_STATE_NORMAL];

  if (values->font)
    bango_font_description_free (values->font);

  values->font = bango_font_description_copy (style->font_desc);
}

BdkPixmap *
_btk_text_util_create_rich_drag_icon (BtkWidget     *widget,
                                      BtkTextBuffer *buffer,
                                      BtkTextIter   *start,
                                      BtkTextIter   *end)
{
  BdkDrawable       *drawable = NULL;
  gint               pixmap_height, pixmap_width;
  gint               layout_width, layout_height;
  BtkTextBuffer     *new_buffer;
  BtkTextLayout     *layout;
  BtkTextAttributes *style;
  BangoContext      *ltr_context, *rtl_context;
  BtkTextIter        iter;
  bairo_t           *cr;

   g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);
   g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), NULL);
   g_return_val_if_fail (start != NULL, NULL);
   g_return_val_if_fail (end != NULL, NULL);

   new_buffer = btk_text_buffer_new (btk_text_buffer_get_tag_table (buffer));
   btk_text_buffer_get_start_iter (new_buffer, &iter);

   btk_text_buffer_insert_range (new_buffer, &iter, start, end);

   btk_text_buffer_get_start_iter (new_buffer, &iter);

   layout = btk_text_layout_new ();

   ltr_context = btk_widget_create_bango_context (widget);
   bango_context_set_base_dir (ltr_context, BANGO_DIRECTION_LTR);
   rtl_context = btk_widget_create_bango_context (widget);
   bango_context_set_base_dir (rtl_context, BANGO_DIRECTION_RTL);

   btk_text_layout_set_contexts (layout, ltr_context, rtl_context);

   g_object_unref (ltr_context);
   g_object_unref (rtl_context);

   style = btk_text_attributes_new ();

   layout_width = widget->allocation.width;

   if (BTK_IS_TEXT_VIEW (widget))
     {
       btk_widget_ensure_style (widget);
       btk_text_view_set_attributes_from_style (BTK_TEXT_VIEW (widget),
                                                style, widget->style);

       layout_width = layout_width
         - btk_text_view_get_border_window_size (BTK_TEXT_VIEW (widget), BTK_TEXT_WINDOW_LEFT)
         - btk_text_view_get_border_window_size (BTK_TEXT_VIEW (widget), BTK_TEXT_WINDOW_RIGHT);
     }

   style->direction = btk_widget_get_direction (widget);
   style->wrap_mode = BANGO_WRAP_WORD_CHAR;

   btk_text_layout_set_default_style (layout, style);
   btk_text_attributes_unref (style);

   btk_text_layout_set_buffer (layout, new_buffer);
   btk_text_layout_set_cursor_visible (layout, FALSE);
   btk_text_layout_set_screen_width (layout, layout_width);

   btk_text_layout_validate (layout, DRAG_ICON_MAX_HEIGHT);
   btk_text_layout_get_size (layout, &layout_width, &layout_height);

   layout_width = MIN (layout_width, DRAG_ICON_MAX_WIDTH);
   layout_height = MIN (layout_height, DRAG_ICON_MAX_HEIGHT);

   pixmap_width  = layout_width + DRAG_ICON_LAYOUT_BORDER * 2;
   pixmap_height = layout_height + DRAG_ICON_LAYOUT_BORDER * 2;

   drawable = bdk_pixmap_new (widget->window,
                              pixmap_width  + 2, pixmap_height + 2, -1);

   cr = bdk_bairo_create (drawable);

   bdk_bairo_set_source_color (cr, &widget->style->base [btk_widget_get_state (widget)]);
   bairo_paint (cr);

   btk_text_layout_draw (layout, widget, drawable,
                         widget->style->text_gc [btk_widget_get_state (widget)],
                         - (1 + DRAG_ICON_LAYOUT_BORDER),
                         - (1 + DRAG_ICON_LAYOUT_BORDER),
                         0, 0,
                         pixmap_width, pixmap_height, NULL);

   bairo_set_source_rgb (cr, 0, 0, 0);
   bairo_rectangle (cr, 0.5, 0.5, pixmap_width + 1, pixmap_height + 1);
   bairo_set_line_width (cr, 1.0);
   bairo_stroke (cr);

   bairo_destroy (cr);
   g_object_unref (layout);
   g_object_unref (new_buffer);

   return drawable;
}


static gint
layout_get_char_width (BangoLayout *layout)
{
  gint width;
  BangoFontMetrics *metrics;
  const BangoFontDescription *font_desc;
  BangoContext *context = bango_layout_get_context (layout);

  font_desc = bango_layout_get_font_description (layout);
  if (!font_desc)
    font_desc = bango_context_get_font_description (context);

  metrics = bango_context_get_metrics (context, font_desc, NULL);
  width = bango_font_metrics_get_approximate_char_width (metrics);
  bango_font_metrics_unref (metrics);

  return width;
}

/*
 * _btk_text_util_get_block_cursor_location
 * @layout: a #BangoLayout
 * @index: index at which cursor is located
 * @pos: cursor location
 * @at_line_end: whether cursor is drawn at line end, not over some
 * character
 *
 * Returns: whether cursor should actually be drawn as a rectangle.
 *     It may not be the case if character at index is invisible.
 */
gboolean
_btk_text_util_get_block_cursor_location (BangoLayout    *layout,
					  gint            index,
					  BangoRectangle *pos,
					  gboolean       *at_line_end)
{
  BangoRectangle strong_pos, weak_pos;
  BangoLayoutLine *layout_line;
  gboolean rtl;
  gint line_no;
  const gchar *text;

  g_return_val_if_fail (layout != NULL, FALSE);
  g_return_val_if_fail (index >= 0, FALSE);
  g_return_val_if_fail (pos != NULL, FALSE);

  bango_layout_index_to_pos (layout, index, pos);

  if (pos->width != 0)
    {
      /* cursor is at some visible character, good */
      if (at_line_end)
	*at_line_end = FALSE;
      if (pos->width < 0)
	{
	  pos->x += pos->width;
	  pos->width = -pos->width;
	}
      return TRUE;
    }

  bango_layout_index_to_line_x (layout, index, FALSE, &line_no, NULL);
  layout_line = bango_layout_get_line_readonly (layout, line_no);
  g_return_val_if_fail (layout_line != NULL, FALSE);

  text = bango_layout_get_text (layout);

  if (index < layout_line->start_index + layout_line->length)
    {
      /* this may be a zero-width character in the middle of the line,
       * or it could be a character where line is wrapped, we do want
       * block cursor in latter case */
      if (g_utf8_next_char (text + index) - text !=
	  layout_line->start_index + layout_line->length)
	{
	  /* zero-width character in the middle of the line, do not
	   * bother with block cursor */
	  return FALSE;
	}
    }

  /* Cursor is at the line end. It may be an empty line, or it could
   * be on the left or on the right depending on text direction, or it
   * even could be in the middle of visual layout in bidi text. */

  bango_layout_get_cursor_pos (layout, index, &strong_pos, &weak_pos);

  if (strong_pos.x != weak_pos.x)
    {
      /* do not show block cursor in this case, since the character typed
       * in may or may not appear at the cursor position */
      return FALSE;
    }

  /* In case when index points to the end of line, pos->x is always most right
   * pixel of the layout line, so we need to correct it for RTL text. */
  if (layout_line->length)
    {
      if (layout_line->resolved_dir == BANGO_DIRECTION_RTL)
	{
	  BangoLayoutIter *iter;
	  BangoRectangle line_rect;
	  gint i;
	  gint left, right;
	  const gchar *p;

	  p = g_utf8_prev_char (text + index);

	  bango_layout_line_index_to_x (layout_line, p - text, FALSE, &left);
	  bango_layout_line_index_to_x (layout_line, p - text, TRUE, &right);
	  pos->x = MIN (left, right);

	  iter = bango_layout_get_iter (layout);
	  for (i = 0; i < line_no; i++)
	    bango_layout_iter_next_line (iter);
	  bango_layout_iter_get_line_extents (iter, NULL, &line_rect);
	  bango_layout_iter_free (iter);

          rtl = TRUE;
	  pos->x += line_rect.x;
	}
      else
	rtl = FALSE;
    }
  else
    {
      BangoContext *context = bango_layout_get_context (layout);
      rtl = bango_context_get_base_dir (context) == BANGO_DIRECTION_RTL;
    }

  pos->width = layout_get_char_width (layout);

  if (rtl)
    pos->x -= pos->width - 1;

  if (at_line_end)
    *at_line_end = TRUE;

  return pos->width != 0;
}
