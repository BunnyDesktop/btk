/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2001, 2002, 2003 Sun Microsystems Inc.
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

#include "config.h"

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <bunnylib-object.h>
#include <bunnylib/gstdio.h>
#include <btk/btk.h>
#include "bailtextview.h"
#include <libbail-util/bailmisc.h>

static void       bail_text_view_class_init            (BailTextViewClass *klass);
static void       bail_text_view_init                  (BailTextView      *text_view);

static void       bail_text_view_real_initialize       (BatkObject         *obj,
                                                        gpointer          data);
static void       bail_text_view_real_notify_btk       (BObject           *obj,
                                                        BParamSpec        *pspec);

static void       bail_text_view_finalize              (BObject           *object);

static void       batk_text_interface_init              (BatkTextIface     *iface);

/* batkobject.h */

static BatkStateSet* bail_text_view_ref_state_set       (BatkObject        *accessible);

/* batktext.h */

static gchar*     bail_text_view_get_text_after_offset (BatkText          *text,
                                                        gint             offset,
                                                        BatkTextBoundary  boundary_type,
                                                        gint             *start_offset,
                                                        gint             *end_offset);
static gchar*     bail_text_view_get_text_at_offset    (BatkText          *text,
                                                        gint             offset,
                                                        BatkTextBoundary  boundary_type,
                                                        gint             *start_offset,
                                                        gint             *end_offset);
static gchar*     bail_text_view_get_text_before_offset (BatkText         *text,
                                                        gint             offset,
                                                        BatkTextBoundary  boundary_type,
                                                        gint             *start_offset,
                                                        gint             *end_offset);
static gchar*     bail_text_view_get_text              (BatkText*text,
                                                        gint             start_offset,
                                                        gint             end_offset);
static gunichar   bail_text_view_get_character_at_offset (BatkText        *text,
                                                        gint             offset);
static gint       bail_text_view_get_character_count   (BatkText          *text);
static gint       bail_text_view_get_caret_offset      (BatkText          *text);
static gboolean   bail_text_view_set_caret_offset      (BatkText          *text,
                                                        gint             offset);
static gint       bail_text_view_get_offset_at_point   (BatkText          *text,
                                                        gint             x,
                                                        gint             y,
                                                        BatkCoordType     coords);
static gint       bail_text_view_get_n_selections      (BatkText          *text);
static gchar*     bail_text_view_get_selection         (BatkText          *text,
                                                        gint             selection_num,
                                                        gint             *start_offset,
                                                        gint             *end_offset);
static gboolean   bail_text_view_add_selection         (BatkText          *text,
                                                        gint             start_offset,
                                                        gint             end_offset);
static gboolean   bail_text_view_remove_selection      (BatkText          *text,
                                                        gint             selection_num);
static gboolean   bail_text_view_set_selection         (BatkText          *text,
                                                        gint             selection_num,
                                                        gint             start_offset,
                                                        gint             end_offset);
static void       bail_text_view_get_character_extents (BatkText          *text,
                                                        gint             offset,
                                                        gint             *x,
                                                        gint             *y,
                                                        gint             *width,
                                                        gint             *height,
                                                        BatkCoordType     coords);
static BatkAttributeSet *  bail_text_view_get_run_attributes 
                                                       (BatkText          *text,
                                                        gint             offset,
                                                        gint             *start_offset,
                                                        gint             *end_offset);
static BatkAttributeSet *  bail_text_view_get_default_attributes 
                                                       (BatkText          *text);
/* batkeditabletext.h */

static void       batk_editable_text_interface_init     (BatkEditableTextIface *iface);
static gboolean   bail_text_view_set_run_attributes    (BatkEditableText  *text,
                                                        BatkAttributeSet  *attrib_set,
                                                        gint             start_offset,
                                                        gint             end_offset);
static void       bail_text_view_set_text_contents     (BatkEditableText  *text,
                                                        const gchar      *string);
static void       bail_text_view_insert_text           (BatkEditableText  *text,
                                                        const gchar      *string,
                                                        gint             length,
                                                        gint             *position);
static void       bail_text_view_copy_text             (BatkEditableText  *text,
                                                        gint             start_pos,
                                                        gint             end_pos);
static void       bail_text_view_cut_text              (BatkEditableText  *text,
                                                        gint             start_pos,
                                                        gint             end_pos);
static void       bail_text_view_delete_text           (BatkEditableText  *text,
                                                        gint             start_pos,
                                                        gint             end_pos);
static void       bail_text_view_paste_text            (BatkEditableText  *text,
                                                        gint             position);
static void       bail_text_view_paste_received        (BtkClipboard     *clipboard,
                                                        const gchar      *text,
                                                        gpointer         data);
/* BatkStreamableContent */
static void       batk_streamable_content_interface_init    (BatkStreamableContentIface *iface);
static gint       bail_streamable_content_get_n_mime_types (BatkStreamableContent *streamable);
static const gchar* bail_streamable_content_get_mime_type (BatkStreamableContent *streamable,
								    gint i);
static BUNNYIOChannel* bail_streamable_content_get_stream       (BatkStreamableContent *streamable,
							     const gchar *mime_type);
/* getURI requires batk-1.12.0 or later
static void       bail_streamable_content_get_uri          (BatkStreamableContent *streamable);
*/

/* Callbacks */

static void       _bail_text_view_insert_text_cb       (BtkTextBuffer    *buffer,
                                                        BtkTextIter      *arg1,
                                                        gchar            *arg2,
                                                        gint             arg3,
                                                        gpointer         user_data);
static void       _bail_text_view_delete_range_cb      (BtkTextBuffer    *buffer,
                                                        BtkTextIter      *arg1,
                                                        BtkTextIter      *arg2,
                                                        gpointer         user_data);
static void       _bail_text_view_changed_cb           (BtkTextBuffer    *buffer,
                                                        gpointer         user_data);
static void       _bail_text_view_mark_set_cb          (BtkTextBuffer    *buffer,
                                                        BtkTextIter      *arg1,
                                                        BtkTextMark      *arg2,
                                                        gpointer         user_data);
static gchar*            get_text_near_offset          (BatkText          *text,
                                                        BailOffsetType   function,
                                                        BatkTextBoundary  boundary_type,
                                                        gint             offset,
                                                        gint             *start_offset,
                                                        gint             *end_offset);
static gint             get_insert_offset              (BtkTextBuffer    *buffer);
static gint             get_selection_bound            (BtkTextBuffer    *buffer);
static void             emit_text_caret_moved          (BailTextView     *bail_text_view,
                                                        gint             insert_offset);
static gint             insert_idle_handler            (gpointer         data);

typedef struct _BailTextViewPaste                       BailTextViewPaste;

struct _BailTextViewPaste
{
  BtkTextBuffer* buffer;
  gint position;
};

G_DEFINE_TYPE_WITH_CODE (BailTextView, bail_text_view, BAIL_TYPE_CONTAINER,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_EDITABLE_TEXT, batk_editable_text_interface_init)
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_TEXT, batk_text_interface_init)
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_STREAMABLE_CONTENT, batk_streamable_content_interface_init))

static void
bail_text_view_class_init (BailTextViewClass *klass)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);
  BatkObjectClass  *class = BATK_OBJECT_CLASS (klass);
  BailWidgetClass *widget_class;

  widget_class = (BailWidgetClass*)klass;

  bobject_class->finalize = bail_text_view_finalize;

  class->ref_state_set = bail_text_view_ref_state_set;
  class->initialize = bail_text_view_real_initialize;

  widget_class->notify_btk = bail_text_view_real_notify_btk;
}

static void
bail_text_view_init (BailTextView      *text_view)
{
  text_view->textutil = NULL;
  text_view->signal_name = NULL;
  text_view->previous_insert_offset = -1;
  text_view->previous_selection_bound = -1;
  text_view->insert_notify_handler = 0;
}

static void
setup_buffer (BtkTextView  *view, 
              BailTextView *bail_view)
{
  BtkTextBuffer *buffer;

  buffer = view->buffer;
  if (buffer == NULL)
    return;

  if (bail_view->textutil)
    g_object_unref (bail_view->textutil);

  bail_view->textutil = bail_text_util_new ();
  bail_text_util_buffer_setup (bail_view->textutil, buffer);

  /* Set up signal callbacks */
  g_signal_connect_object (buffer, "insert-text",
                           (GCallback) _bail_text_view_insert_text_cb,
                           view, 0);
  g_signal_connect_object (buffer, "delete-range",
                           (GCallback) _bail_text_view_delete_range_cb,
                           view, 0);
  g_signal_connect_object (buffer, "mark-set",
                           (GCallback) _bail_text_view_mark_set_cb,
                           view, 0);
  g_signal_connect_object (buffer, "changed",
                           (GCallback) _bail_text_view_changed_cb,
                           view, 0);

}

static void
bail_text_view_real_initialize (BatkObject *obj,
                                gpointer  data)
{
  BtkTextView *view;
  BailTextView *bail_view;

  BATK_OBJECT_CLASS (bail_text_view_parent_class)->initialize (obj, data);

  view = BTK_TEXT_VIEW (data);

  bail_view = BAIL_TEXT_VIEW (obj);
  setup_buffer (view, bail_view);

  obj->role = BATK_ROLE_TEXT;

}

static void
bail_text_view_finalize (BObject            *object)
{
  BailTextView *text_view = BAIL_TEXT_VIEW (object);

  g_object_unref (text_view->textutil);
  if (text_view->insert_notify_handler)
    g_source_remove (text_view->insert_notify_handler);

  B_OBJECT_CLASS (bail_text_view_parent_class)->finalize (object);
}

static void
bail_text_view_real_notify_btk (BObject             *obj,
                                BParamSpec          *pspec)
{
  if (!strcmp (pspec->name, "editable"))
    {
      BatkObject *batk_obj;
      gboolean editable;

      batk_obj = btk_widget_get_accessible (BTK_WIDGET (obj));
      editable = btk_text_view_get_editable (BTK_TEXT_VIEW (obj));
      batk_object_notify_state_change (batk_obj, BATK_STATE_EDITABLE,
                                      editable);
    }
  else if (!strcmp (pspec->name, "buffer"))
    {
      BatkObject *batk_obj;

      batk_obj = btk_widget_get_accessible (BTK_WIDGET (obj));
      setup_buffer (BTK_TEXT_VIEW (obj), BAIL_TEXT_VIEW (batk_obj));
    }
  else
    BAIL_WIDGET_CLASS (bail_text_view_parent_class)->notify_btk (obj, pspec);
}

/* batkobject.h */

static BatkStateSet*
bail_text_view_ref_state_set (BatkObject *accessible)
{
  BatkStateSet *state_set;
  BtkTextView *text_view;
  BtkWidget *widget;

  state_set = BATK_OBJECT_CLASS (bail_text_view_parent_class)->ref_state_set (accessible);
  widget = BTK_ACCESSIBLE (accessible)->widget;

  if (widget == NULL)
    return state_set;

  text_view = BTK_TEXT_VIEW (widget);

  if (btk_text_view_get_editable (text_view))
    batk_state_set_add_state (state_set, BATK_STATE_EDITABLE);
  batk_state_set_add_state (state_set, BATK_STATE_MULTI_LINE);

  return state_set;
}

/* batktext.h */

static void
batk_text_interface_init (BatkTextIface *iface)
{
  iface->get_text = bail_text_view_get_text;
  iface->get_text_after_offset = bail_text_view_get_text_after_offset;
  iface->get_text_at_offset = bail_text_view_get_text_at_offset;
  iface->get_text_before_offset = bail_text_view_get_text_before_offset;
  iface->get_character_at_offset = bail_text_view_get_character_at_offset;
  iface->get_character_count = bail_text_view_get_character_count;
  iface->get_caret_offset = bail_text_view_get_caret_offset;
  iface->set_caret_offset = bail_text_view_set_caret_offset;
  iface->get_offset_at_point = bail_text_view_get_offset_at_point;
  iface->get_character_extents = bail_text_view_get_character_extents;
  iface->get_n_selections = bail_text_view_get_n_selections;
  iface->get_selection = bail_text_view_get_selection;
  iface->add_selection = bail_text_view_add_selection;
  iface->remove_selection = bail_text_view_remove_selection;
  iface->set_selection = bail_text_view_set_selection;
  iface->get_run_attributes = bail_text_view_get_run_attributes;
  iface->get_default_attributes = bail_text_view_get_default_attributes;
}

static gchar*
bail_text_view_get_text (BatkText *text,
                         gint    start_offset,
                         gint    end_offset)
{
  BtkTextView *view;
  BtkTextBuffer *buffer;
  BtkTextIter start, end;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  view = BTK_TEXT_VIEW (widget);
  buffer = view->buffer;
  btk_text_buffer_get_iter_at_offset (buffer, &start, start_offset);
  btk_text_buffer_get_iter_at_offset (buffer, &end, end_offset);

  return btk_text_buffer_get_text (buffer, &start, &end, FALSE);
}

static gchar*
bail_text_view_get_text_after_offset (BatkText         *text,
                                      gint            offset,
                                      BatkTextBoundary boundary_type,
                                      gint            *start_offset,
                                      gint            *end_offset)
{
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  return get_text_near_offset (text, BAIL_AFTER_OFFSET,
                               boundary_type, offset, 
                               start_offset, end_offset);
}

static gchar*
bail_text_view_get_text_at_offset (BatkText         *text,
                                   gint            offset,
                                   BatkTextBoundary boundary_type,
                                   gint            *start_offset,
                                   gint            *end_offset)
{
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  return get_text_near_offset (text, BAIL_AT_OFFSET,
                               boundary_type, offset, 
                               start_offset, end_offset);
}

static gchar*
bail_text_view_get_text_before_offset (BatkText         *text,
                                       gint            offset,
                                       BatkTextBoundary boundary_type,
                                       gint            *start_offset,
                                       gint            *end_offset)
{
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  return get_text_near_offset (text, BAIL_BEFORE_OFFSET,
                               boundary_type, offset, 
                               start_offset, end_offset);
}

static gunichar
bail_text_view_get_character_at_offset (BatkText *text,
                                        gint    offset)
{
  BtkWidget *widget;
  BtkTextIter start, end;
  BtkTextBuffer *buffer;
  gchar *string;
  gunichar unichar;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    return '\0';

  buffer = BAIL_TEXT_VIEW (text)->textutil->buffer;
  if (offset >= btk_text_buffer_get_char_count (buffer))
    return '\0';

  btk_text_buffer_get_iter_at_offset (buffer, &start, offset);
  end = start;
  btk_text_iter_forward_char (&end);
  string = btk_text_buffer_get_slice (buffer, &start, &end, FALSE);
  unichar = g_utf8_get_char (string);
  g_free(string);
  return unichar;
}

static gint
bail_text_view_get_character_count (BatkText *text)
{
  BtkTextView *view;
  BtkTextBuffer *buffer;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return 0;

  view = BTK_TEXT_VIEW (widget);
  buffer = view->buffer;
  return btk_text_buffer_get_char_count (buffer);
}

static gint
bail_text_view_get_caret_offset (BatkText *text)
{
  BtkTextView *view;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return 0;

  view = BTK_TEXT_VIEW (widget);
  return get_insert_offset (view->buffer);
}

static gboolean
bail_text_view_set_caret_offset (BatkText *text,
                                 gint    offset)
{
  BtkTextView *view;
  BtkWidget *widget;
  BtkTextBuffer *buffer;
  BtkTextIter pos_itr;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;

  view = BTK_TEXT_VIEW (widget);
  buffer = view->buffer;

  btk_text_buffer_get_iter_at_offset (buffer,  &pos_itr, offset);
  btk_text_buffer_place_cursor (buffer, &pos_itr);
  btk_text_view_scroll_to_iter (view, &pos_itr, 0, FALSE, 0, 0);
  return TRUE;
}

static gint
bail_text_view_get_offset_at_point (BatkText      *text,
                                    gint         x,
                                    gint         y,
                                    BatkCoordType coords)
{
  BtkTextView *view;
  BtkTextBuffer *buffer;
  BtkTextIter loc_itr;
  gint x_widget, y_widget, x_window, y_window, buff_x, buff_y;
  BtkWidget *widget;
  BdkWindow *window;
  BdkRectangle rect;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return -1;

  view = BTK_TEXT_VIEW (widget);
  buffer = view->buffer;

  window = btk_text_view_get_window (view, BTK_TEXT_WINDOW_WIDGET);
  bdk_window_get_origin (window, &x_widget, &y_widget);

  if (coords == BATK_XY_SCREEN)
    {
      x = x - x_widget;
      y = y - y_widget;
    }
  else if (coords == BATK_XY_WINDOW)
    {
      window = bdk_window_get_toplevel (window);
      bdk_window_get_origin (window, &x_window, &y_window);

      x = x - x_widget + x_window;
      y = y - y_widget + y_window;
    }
  else
    return -1;

  btk_text_view_window_to_buffer_coords (view, BTK_TEXT_WINDOW_WIDGET,
                                         x, y, &buff_x, &buff_y);
  btk_text_view_get_visible_rect (view, &rect);
  /*
   * Clamp point to visible rectangle
   */
  buff_x = CLAMP (buff_x, rect.x, rect.x + rect.width - 1);
  buff_y = CLAMP (buff_y, rect.y, rect.y + rect.height - 1);

  btk_text_view_get_iter_at_location (view, &loc_itr, buff_x, buff_y);
  /*
   * The iter at a location sometimes points to the next character.
   * See bug 111031. We work around that
   */
  btk_text_view_get_iter_location (view, &loc_itr, &rect);
  if (buff_x < rect.x)
    btk_text_iter_backward_char (&loc_itr);
  return btk_text_iter_get_offset (&loc_itr);
}

static void
bail_text_view_get_character_extents (BatkText      *text,
                                      gint         offset,
                                      gint         *x,
                                      gint         *y,
                                      gint         *width,
                                      gint         *height,
                                      BatkCoordType coords)
{
  BtkTextView *view;
  BtkTextBuffer *buffer;
  BtkTextIter iter;
  BtkWidget *widget;
  BdkRectangle rectangle;
  BdkWindow *window;
  gint x_widget, y_widget, x_window, y_window;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return;

  view = BTK_TEXT_VIEW (widget);
  buffer = view->buffer;
  btk_text_buffer_get_iter_at_offset (buffer, &iter, offset);
  btk_text_view_get_iter_location (view, &iter, &rectangle);

  window = btk_text_view_get_window (view, BTK_TEXT_WINDOW_WIDGET);
  bdk_window_get_origin (window, &x_widget, &y_widget);

  *height = rectangle.height;
  *width = rectangle.width;

  btk_text_view_buffer_to_window_coords (view, BTK_TEXT_WINDOW_WIDGET,
    rectangle.x, rectangle.y, x, y);
  if (coords == BATK_XY_WINDOW)
    {
      window = bdk_window_get_toplevel (window);
      bdk_window_get_origin (window, &x_window, &y_window);
      *x += x_widget - x_window;
        *y += y_widget - y_window;
    }
  else if (coords == BATK_XY_SCREEN)
    {
      *x += x_widget;
      *y += y_widget;
    }
  else
    {
      *x = 0;
      *y = 0;
      *height = 0;
      *width = 0;
    }
}

static BatkAttributeSet*
bail_text_view_get_run_attributes (BatkText *text,
                                   gint    offset,
                                   gint    *start_offset,
                                   gint    *end_offset)
{
  BtkTextView *view;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  view = BTK_TEXT_VIEW (widget);

  return bail_misc_buffer_get_run_attributes (view->buffer, offset, 
                                              start_offset, end_offset);
}

static BatkAttributeSet*
bail_text_view_get_default_attributes (BatkText *text)
{
  BtkTextView *view;
  BtkWidget *widget;
  BtkTextAttributes *text_attrs;
  BatkAttributeSet *attrib_set = NULL;
  BangoFontDescription *font;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  view = BTK_TEXT_VIEW (widget);
  text_attrs = btk_text_view_get_default_attributes (view);

  font = text_attrs->font;

  if (font)
    {
      attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                              BATK_TEXT_ATTR_STYLE);

      attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                              BATK_TEXT_ATTR_VARIANT);

      attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                              BATK_TEXT_ATTR_STRETCH);
    }

  attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                          BATK_TEXT_ATTR_JUSTIFICATION);

  attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                          BATK_TEXT_ATTR_DIRECTION);

  attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                          BATK_TEXT_ATTR_WRAP_MODE);

  attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                          BATK_TEXT_ATTR_FG_STIPPLE);

  attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                          BATK_TEXT_ATTR_BG_STIPPLE);

  attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                          BATK_TEXT_ATTR_FG_COLOR);

  attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                          BATK_TEXT_ATTR_BG_COLOR);

  if (font)
    {
      attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                              BATK_TEXT_ATTR_FAMILY_NAME);
    }

  attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                          BATK_TEXT_ATTR_LANGUAGE);

  if (font)
    {
      attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                              BATK_TEXT_ATTR_WEIGHT);
    }

  attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                          BATK_TEXT_ATTR_SCALE);

  if (font)
    {
      attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                              BATK_TEXT_ATTR_SIZE);
    }

  attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                          BATK_TEXT_ATTR_STRIKETHROUGH);

  attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                          BATK_TEXT_ATTR_UNDERLINE);

  attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                          BATK_TEXT_ATTR_RISE);

  attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                          BATK_TEXT_ATTR_BG_FULL_HEIGHT);

  attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                          BATK_TEXT_ATTR_PIXELS_INSIDE_WRAP);

  attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                         BATK_TEXT_ATTR_PIXELS_BELOW_LINES);

  attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                          BATK_TEXT_ATTR_PIXELS_ABOVE_LINES);

  attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                          BATK_TEXT_ATTR_EDITABLE);
    
  attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                          BATK_TEXT_ATTR_INVISIBLE);

  attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                          BATK_TEXT_ATTR_INDENT);

  attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                          BATK_TEXT_ATTR_RIGHT_MARGIN);

  attrib_set = bail_misc_add_to_attr_set (attrib_set, text_attrs, 
                                          BATK_TEXT_ATTR_LEFT_MARGIN);

  btk_text_attributes_unref (text_attrs);
  return attrib_set;
}

static gint
bail_text_view_get_n_selections (BatkText *text)
{
  BtkTextView *view;
  BtkWidget *widget;
  BtkTextBuffer *buffer;
  BtkTextIter start, end;
  gint select_start, select_end;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return -1;

  view = BTK_TEXT_VIEW (widget);
  buffer = view->buffer;

  btk_text_buffer_get_selection_bounds (buffer, &start, &end);
  select_start = btk_text_iter_get_offset (&start);
  select_end = btk_text_iter_get_offset (&end);

  if (select_start != select_end)
     return 1;
  else
     return 0;
}

static gchar*
bail_text_view_get_selection (BatkText *text,
                              gint    selection_num,
                              gint    *start_pos,
                              gint    *end_pos)
{
  BtkTextView *view;
  BtkWidget *widget;
  BtkTextBuffer *buffer;
  BtkTextIter start, end;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

 /* Only let the user get the selection if one is set, and if the
  * selection_num is 0.
  */
  if (selection_num != 0)
     return NULL;

  view = BTK_TEXT_VIEW (widget);
  buffer = view->buffer;

  btk_text_buffer_get_selection_bounds (buffer, &start, &end);
  *start_pos = btk_text_iter_get_offset (&start);
  *end_pos = btk_text_iter_get_offset (&end);

  if (*start_pos != *end_pos)
    return btk_text_buffer_get_text (buffer, &start, &end, FALSE);
  else
    return NULL;
}

static gboolean
bail_text_view_add_selection (BatkText *text,
                              gint    start_pos,
                              gint    end_pos)
{
  BtkTextView *view;
  BtkWidget *widget;
  BtkTextBuffer *buffer;
  BtkTextIter pos_itr;
  BtkTextIter start, end;
  gint select_start, select_end;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;

  view = BTK_TEXT_VIEW (widget);
  buffer = view->buffer;

  btk_text_buffer_get_selection_bounds (buffer, &start, &end);
  select_start = btk_text_iter_get_offset (&start);
  select_end = btk_text_iter_get_offset (&end);

 /* If there is already a selection, then don't allow another to be added,
  * since BtkTextView only supports one selected rebunnyion.
  */
  if (select_start == select_end)
    {
      btk_text_buffer_get_iter_at_offset (buffer,  &pos_itr, start_pos);
      btk_text_buffer_move_mark_by_name (buffer, "selection_bound", &pos_itr);
      btk_text_buffer_get_iter_at_offset (buffer,  &pos_itr, end_pos);
      btk_text_buffer_move_mark_by_name (buffer, "insert", &pos_itr);
      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
bail_text_view_remove_selection (BatkText *text,
                                 gint    selection_num)
{
  BtkTextView *view;
  BtkWidget *widget;
  BtkTextBuffer *buffer;
  BtkTextMark *cursor_mark;
  BtkTextIter cursor_itr;
  BtkTextIter start, end;
  gint select_start, select_end;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;

  if (selection_num != 0)
     return FALSE;

  view = BTK_TEXT_VIEW (widget);
  buffer = view->buffer;

  btk_text_buffer_get_selection_bounds(buffer, &start, &end);
  select_start = btk_text_iter_get_offset(&start);
  select_end = btk_text_iter_get_offset(&end);

  if (select_start != select_end)
    {
     /* Setting the start & end of the selected rebunnyion to the caret position
      * turns off the selection.
      */
      cursor_mark = btk_text_buffer_get_insert (buffer);
      btk_text_buffer_get_iter_at_mark (buffer, &cursor_itr, cursor_mark);
      btk_text_buffer_move_mark_by_name (buffer, "selection_bound", &cursor_itr);
      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
bail_text_view_set_selection (BatkText *text,
                              gint    selection_num,
                              gint    start_pos,
                              gint    end_pos)
{
  BtkTextView *view;
  BtkWidget *widget;
  BtkTextBuffer *buffer;
  BtkTextIter pos_itr;
  BtkTextIter start, end;
  gint select_start, select_end;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
  {
    /* State is defunct */
    return FALSE;
  }

 /* Only let the user move the selection if one is set, and if the
  * selection_num is 0
  */
  if (selection_num != 0)
     return FALSE;

  view = BTK_TEXT_VIEW (widget);
  buffer = view->buffer;

  btk_text_buffer_get_selection_bounds(buffer, &start, &end);
  select_start = btk_text_iter_get_offset(&start);
  select_end = btk_text_iter_get_offset(&end);

  if (select_start != select_end)
    {
      btk_text_buffer_get_iter_at_offset (buffer,  &pos_itr, start_pos);
      btk_text_buffer_move_mark_by_name (buffer, "selection_bound", &pos_itr);
      btk_text_buffer_get_iter_at_offset (buffer,  &pos_itr, end_pos);
      btk_text_buffer_move_mark_by_name (buffer, "insert", &pos_itr);
      return TRUE;
    }
  else
    return FALSE;
}

/* batkeditabletext.h */

static void
batk_editable_text_interface_init (BatkEditableTextIface *iface)
{
  iface->set_text_contents = bail_text_view_set_text_contents;
  iface->insert_text = bail_text_view_insert_text;
  iface->copy_text = bail_text_view_copy_text;
  iface->cut_text = bail_text_view_cut_text;
  iface->delete_text = bail_text_view_delete_text;
  iface->paste_text = bail_text_view_paste_text;
  iface->set_run_attributes = bail_text_view_set_run_attributes;
}

static gboolean
bail_text_view_set_run_attributes (BatkEditableText *text,
                                   BatkAttributeSet *attrib_set,
                                   gint            start_offset,
                                   gint            end_offset)
{
  BtkTextView *view;
  BtkTextBuffer *buffer;
  BtkWidget *widget;
  BtkTextTag *tag;
  BtkTextIter start;
  BtkTextIter end;
  gint j;
  BdkColor *color;
  gchar** RGB_vals;
  GSList *l;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;

  view = BTK_TEXT_VIEW (widget);
  if (!btk_text_view_get_editable (view))
    return FALSE;

  buffer = view->buffer;

  if (attrib_set == NULL)
    return FALSE;

  btk_text_buffer_get_iter_at_offset (buffer, &start, start_offset);
  btk_text_buffer_get_iter_at_offset (buffer, &end, end_offset);

  tag = btk_text_buffer_create_tag (buffer, NULL, NULL);

  for (l = attrib_set; l; l = l->next)
    {
      gchar *name;
      gchar *value;
      BatkAttribute *at;

      at = l->data;

      name = at->name;
      value = at->value;

      if (!strcmp (name, batk_text_attribute_get_name (BATK_TEXT_ATTR_LEFT_MARGIN)))
        g_object_set (B_OBJECT (tag), "left_margin", atoi (value), NULL);

      else if (!strcmp (name, batk_text_attribute_get_name (BATK_TEXT_ATTR_RIGHT_MARGIN)))
        g_object_set (B_OBJECT (tag), "right_margin", atoi (value), NULL);

      else if (!strcmp (name, batk_text_attribute_get_name (BATK_TEXT_ATTR_INDENT)))
        g_object_set (B_OBJECT (tag), "indent", atoi (value), NULL);

      else if (!strcmp (name, batk_text_attribute_get_name (BATK_TEXT_ATTR_PIXELS_ABOVE_LINES)))
        g_object_set (B_OBJECT (tag), "pixels_above_lines", atoi (value), NULL);

      else if (!strcmp(name, batk_text_attribute_get_name (BATK_TEXT_ATTR_PIXELS_BELOW_LINES)))
        g_object_set (B_OBJECT (tag), "pixels_below_lines", atoi (value), NULL);

      else if (!strcmp (name, batk_text_attribute_get_name (BATK_TEXT_ATTR_PIXELS_INSIDE_WRAP)))
        g_object_set (B_OBJECT (tag), "pixels_inside_wrap", atoi (value), NULL);

      else if (!strcmp (name, batk_text_attribute_get_name (BATK_TEXT_ATTR_SIZE)))
        g_object_set (B_OBJECT (tag), "size", atoi (value), NULL);

      else if (!strcmp (name, batk_text_attribute_get_name (BATK_TEXT_ATTR_RISE)))
        g_object_set (B_OBJECT (tag), "rise", atoi (value), NULL);

      else if (!strcmp (name, batk_text_attribute_get_name (BATK_TEXT_ATTR_WEIGHT)))
        g_object_set (B_OBJECT (tag), "weight", atoi (value), NULL);

      else if (!strcmp (name, batk_text_attribute_get_name (BATK_TEXT_ATTR_BG_FULL_HEIGHT)))
        {
          g_object_set (B_OBJECT (tag), "bg_full_height", 
                   (strcmp (value, batk_text_attribute_get_value (BATK_TEXT_ATTR_BG_FULL_HEIGHT, 0))),
                   NULL);
        }

      else if (!strcmp (name, batk_text_attribute_get_name (BATK_TEXT_ATTR_LANGUAGE)))
        g_object_set (B_OBJECT (tag), "language", value, NULL);

      else if (!strcmp (name, batk_text_attribute_get_name (BATK_TEXT_ATTR_FAMILY_NAME)))
        g_object_set (B_OBJECT (tag), "family", value, NULL);

      else if (!strcmp (name, batk_text_attribute_get_name (BATK_TEXT_ATTR_EDITABLE)))
        {
          g_object_set (B_OBJECT (tag), "editable", 
                   (strcmp (value, batk_text_attribute_get_value (BATK_TEXT_ATTR_EDITABLE, 0))),
                   NULL);
        }

      else if (!strcmp (name, batk_text_attribute_get_name (BATK_TEXT_ATTR_INVISIBLE)))
        {
          g_object_set (B_OBJECT (tag), "invisible", 
                   (strcmp (value, batk_text_attribute_get_value (BATK_TEXT_ATTR_EDITABLE, 0))),
                   NULL);
        }

      else if (!strcmp (name, batk_text_attribute_get_name (BATK_TEXT_ATTR_UNDERLINE)))
        {
          for (j = 0; j < 3; j++)
            {
              if (!strcmp (value, batk_text_attribute_get_value (BATK_TEXT_ATTR_UNDERLINE, j)))
                {
                  g_object_set (B_OBJECT (tag), "underline", j, NULL);
                  break;
                }
            } 
        }

      else if (!strcmp (name, batk_text_attribute_get_name (BATK_TEXT_ATTR_STRIKETHROUGH)))
        {
          g_object_set (B_OBJECT (tag), "strikethrough", 
                   (strcmp (value, batk_text_attribute_get_value (BATK_TEXT_ATTR_STRIKETHROUGH, 0))),
                   NULL);
        }

      else if (!strcmp (name, batk_text_attribute_get_name (BATK_TEXT_ATTR_BG_COLOR)))
        {
          RGB_vals = g_strsplit (value, ",", 3);
          color = g_malloc (sizeof (BdkColor));
          color->red = atoi (RGB_vals[0]);
          color->green = atoi (RGB_vals[1]);
          color->blue = atoi (RGB_vals[2]);
          g_object_set (B_OBJECT (tag), "background_bdk", color, NULL);
        }
  
      else if (!strcmp (name, batk_text_attribute_get_name (BATK_TEXT_ATTR_FG_COLOR)))
        {
          RGB_vals = g_strsplit (value, ",", 3);
          color = g_malloc (sizeof (BdkColor));
          color->red = atoi (RGB_vals[0]);
          color->green = atoi (RGB_vals[1]);
          color->blue = atoi (RGB_vals[2]);
          g_object_set (B_OBJECT (tag), "foreground_bdk", color, NULL);
        }

      else if (!strcmp (name, batk_text_attribute_get_name (BATK_TEXT_ATTR_STRETCH)))
        {
          for (j = 0; j < 9; j++)
            {
              if (!strcmp (value, batk_text_attribute_get_value (BATK_TEXT_ATTR_STRETCH, j)))
                {
                  g_object_set (B_OBJECT (tag), "stretch", j, NULL);
                  break;
                }
            }
        }

      else if (!strcmp (name, batk_text_attribute_get_name (BATK_TEXT_ATTR_JUSTIFICATION)))
        {
          for (j = 0; j < 4; j++)
            {
              if (!strcmp (value, batk_text_attribute_get_value (BATK_TEXT_ATTR_JUSTIFICATION, j)))
                {
                  g_object_set (B_OBJECT (tag), "justification", j, NULL);
                  break;
                }
            }
        }

      else if (!strcmp (name, batk_text_attribute_get_name (BATK_TEXT_ATTR_DIRECTION)))
        {
          for (j = 0; j < 3; j++)
            {
              if (!strcmp (value, batk_text_attribute_get_value (BATK_TEXT_ATTR_DIRECTION, j)))
                {
                  g_object_set (B_OBJECT (tag), "direction", j, NULL);
                  break;
                }
            }
        }

      else if (!strcmp (name, batk_text_attribute_get_name (BATK_TEXT_ATTR_VARIANT)))
        {
          for (j = 0; j < 2; j++)
            {
              if (!strcmp (value, batk_text_attribute_get_value (BATK_TEXT_ATTR_VARIANT, j)))
                {
                  g_object_set (B_OBJECT (tag), "variant", j, NULL);
                  break;
                }
            }
        }

      else if (!strcmp (name, batk_text_attribute_get_name (BATK_TEXT_ATTR_WRAP_MODE)))
        {
          for (j = 0; j < 3; j++)
            {
              if (!strcmp (value, batk_text_attribute_get_value (BATK_TEXT_ATTR_WRAP_MODE, j)))
                {
                  g_object_set (B_OBJECT (tag), "wrap_mode", j, NULL);
                  break;
                }
            }
        }

      else if (!strcmp (name, batk_text_attribute_get_name (BATK_TEXT_ATTR_STYLE)))
        {
          for (j = 0; j < 3; j++)
            {
              if (!strcmp (value, batk_text_attribute_get_value (BATK_TEXT_ATTR_STYLE, j)))
                {
                  g_object_set (B_OBJECT (tag), "style", j, NULL);
                  break;
              }
            }
        }

      else
        return FALSE;
    }

  btk_text_buffer_apply_tag (buffer, tag, &start, &end);

  return TRUE;
}

static void
bail_text_view_set_text_contents (BatkEditableText *text,
                                  const gchar     *string)
{
  BtkTextView *view;
  BtkWidget *widget;
  BtkTextBuffer *buffer;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return;

  view = BTK_TEXT_VIEW (widget);
  if (!btk_text_view_get_editable (view))
    return;
  buffer = view->buffer;

  /* The -1 indicates that the input string must be null-terminated */
  btk_text_buffer_set_text (buffer, string, -1);
}

static void
bail_text_view_insert_text (BatkEditableText *text,
                            const gchar     *string,
                            gint            length,
                            gint            *position)
{
  BtkTextView *view;
  BtkWidget *widget;
  BtkTextBuffer *buffer;
  BtkTextIter pos_itr;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return;

  view = BTK_TEXT_VIEW (widget);
  if (!btk_text_view_get_editable (view))
    return;
  buffer = view->buffer;

  btk_text_buffer_get_iter_at_offset (buffer, &pos_itr, *position);
  btk_text_buffer_insert (buffer, &pos_itr, string, length);
}

static void
bail_text_view_copy_text   (BatkEditableText *text,
                            gint            start_pos,
                            gint            end_pos)
{
  BtkTextView *view;
  BtkWidget *widget;
  BtkTextBuffer *buffer;
  BtkTextIter start, end;
  gchar *str;
  BtkClipboard *clipboard;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return;

  view = BTK_TEXT_VIEW (widget);
  buffer = view->buffer;

  btk_text_buffer_get_iter_at_offset (buffer, &start, start_pos);
  btk_text_buffer_get_iter_at_offset (buffer, &end, end_pos);
  str = btk_text_buffer_get_text (buffer, &start, &end, FALSE);
  clipboard = btk_clipboard_get_for_display (btk_widget_get_display (widget),
                                             BDK_SELECTION_CLIPBOARD);
  btk_clipboard_set_text (clipboard, str, -1);
}

static void
bail_text_view_cut_text (BatkEditableText *text,
                         gint            start_pos,
                         gint            end_pos)
{
  BtkTextView *view;
  BtkWidget *widget;
  BtkTextBuffer *buffer;
  BtkTextIter start, end;
  gchar *str;
  BtkClipboard *clipboard;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return;

  view = BTK_TEXT_VIEW (widget);
  if (!btk_text_view_get_editable (view))
    return;
  buffer = view->buffer;

  btk_text_buffer_get_iter_at_offset (buffer, &start, start_pos);
  btk_text_buffer_get_iter_at_offset (buffer, &end, end_pos);
  str = btk_text_buffer_get_text (buffer, &start, &end, FALSE);
  clipboard = btk_clipboard_get_for_display (btk_widget_get_display (widget),
                                             BDK_SELECTION_CLIPBOARD);
  btk_clipboard_set_text (clipboard, str, -1);
  btk_text_buffer_delete (buffer, &start, &end);
}

static void
bail_text_view_delete_text (BatkEditableText *text,
                            gint            start_pos,
                            gint            end_pos)
{
  BtkTextView *view;
  BtkWidget *widget;
  BtkTextBuffer *buffer;
  BtkTextIter start_itr;
  BtkTextIter end_itr;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return;

  view = BTK_TEXT_VIEW (widget);
  if (!btk_text_view_get_editable (view))
    return;
  buffer = view->buffer;

  btk_text_buffer_get_iter_at_offset (buffer, &start_itr, start_pos);
  btk_text_buffer_get_iter_at_offset (buffer, &end_itr, end_pos);
  btk_text_buffer_delete (buffer, &start_itr, &end_itr);
}

static void
bail_text_view_paste_text (BatkEditableText *text,
                           gint            position)
{
  BtkTextView *view;
  BtkWidget *widget;
  BtkTextBuffer *buffer;
  BailTextViewPaste paste_struct;
  BtkClipboard *clipboard;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return;

  view = BTK_TEXT_VIEW (widget);
  if (!btk_text_view_get_editable (view))
    return;
  buffer = view->buffer;

  paste_struct.buffer = buffer;
  paste_struct.position = position;

  g_object_ref (paste_struct.buffer);
  clipboard = btk_clipboard_get_for_display (btk_widget_get_display (widget),
                                             BDK_SELECTION_CLIPBOARD);
  btk_clipboard_request_text (clipboard,
    bail_text_view_paste_received, &paste_struct);
}

static void
bail_text_view_paste_received (BtkClipboard *clipboard,
                               const gchar  *text,
                               gpointer     data)
{
  BailTextViewPaste* paste_struct = (BailTextViewPaste *)data;
  BtkTextIter pos_itr;

  if (text)
    {
      btk_text_buffer_get_iter_at_offset (paste_struct->buffer, &pos_itr,
         paste_struct->position);
      btk_text_buffer_insert (paste_struct->buffer, &pos_itr, text, -1);
    }

  g_object_unref (paste_struct->buffer);
}

/* Callbacks */

/* Note arg1 returns the start of the insert range, arg3 returns the
 * end of the insert range if multiple characters are inserted.  If one
 * character is inserted they have the same value, which is the caret
 * location.  arg2 returns the begin location of the insert.
 */
static void 
_bail_text_view_insert_text_cb (BtkTextBuffer *buffer,
                                BtkTextIter   *arg1, 
                                gchar         *arg2, 
                                gint          arg3,
                                gpointer      user_data)
{
  BtkTextView *text = (BtkTextView *) user_data;
  BatkObject *accessible;
  BailTextView *bail_text_view;
  gint position;
  gint length;

  g_return_if_fail (arg3 > 0);

  accessible = btk_widget_get_accessible(BTK_WIDGET(text));
  bail_text_view = BAIL_TEXT_VIEW (accessible);

  bail_text_view->signal_name = "text_changed::insert";
  position = btk_text_iter_get_offset (arg1);
  length = g_utf8_strlen(arg2, arg3);
  
  if (bail_text_view->length == 0)
    {
      bail_text_view->position = position;
      bail_text_view->length = length;
    }
  else if (bail_text_view->position + bail_text_view->length == position)
    {
      bail_text_view->length += length;
    }
  else
    {
      /*
       * We have a non-contiguous insert so report what we have
       */
      if (bail_text_view->insert_notify_handler)
        {
          g_source_remove (bail_text_view->insert_notify_handler);
        }
      bail_text_view->insert_notify_handler = 0;
      insert_idle_handler (bail_text_view);
      bail_text_view->position = position;
      bail_text_view->length = length;
    }
    
  /*
   * The signal will be emitted when the changed signal is received
   */
}

/* Note arg1 returns the start of the delete range, arg2 returns the
 * end of the delete range if multiple characters are deleted.  If one
 * character is deleted they have the same value, which is the caret
 * location.
 */
static void 
_bail_text_view_delete_range_cb (BtkTextBuffer *buffer,
                                 BtkTextIter   *arg1, 
                                 BtkTextIter   *arg2,
                                 gpointer      user_data)
{
  BtkTextView *text = (BtkTextView *) user_data;
  BatkObject *accessible;
  BailTextView *bail_text_view;
  gint offset = btk_text_iter_get_offset (arg1);
  gint length = btk_text_iter_get_offset (arg2) - offset;

  accessible = btk_widget_get_accessible(BTK_WIDGET(text));
  bail_text_view = BAIL_TEXT_VIEW (accessible);
  if (bail_text_view->insert_notify_handler)
    {
      g_source_remove (bail_text_view->insert_notify_handler);
      bail_text_view->insert_notify_handler = 0;
      if (bail_text_view->position == offset && 
          bail_text_view->length == length)
        {
        /*
         * Do not bother with insert and delete notifications
         */
          bail_text_view->signal_name = NULL;
          bail_text_view->position = 0;
          bail_text_view->length = 0;
          return;
        }

      insert_idle_handler (bail_text_view);
    }
  g_signal_emit_by_name (accessible, "text_changed::delete",
                         offset, length);
}

/* Note arg1 and arg2 point to the same offset, which is the caret
 * position after the move
 */
static void 
_bail_text_view_mark_set_cb (BtkTextBuffer *buffer,
                             BtkTextIter   *arg1, 
                             BtkTextMark   *arg2,
                             gpointer      user_data)
{
  BtkTextView *text = (BtkTextView *) user_data;
  BatkObject *accessible;
  BailTextView *bail_text_view;
  const char *mark_name = btk_text_mark_get_name(arg2);

  accessible = btk_widget_get_accessible(BTK_WIDGET(text));
  bail_text_view = BAIL_TEXT_VIEW (accessible);

  /*
   * Only generate the signal for the "insert" mark, which
   * represents the cursor.
   */
  if (mark_name && !strcmp(mark_name, "insert"))
    {
      int insert_offset, selection_bound;
      gboolean selection_changed;

      insert_offset = btk_text_iter_get_offset (arg1);

      selection_bound = get_selection_bound (buffer);
      if (selection_bound != insert_offset)
        {
          if (selection_bound != bail_text_view->previous_selection_bound ||
              insert_offset != bail_text_view->previous_insert_offset)
            {
              selection_changed = TRUE;
            }
          else
            {
              selection_changed = FALSE;
            }
        }
      else if (bail_text_view->previous_selection_bound != bail_text_view->previous_insert_offset)
        {
          selection_changed = TRUE;
        }
      else
        {
          selection_changed = FALSE;
        }

      emit_text_caret_moved (bail_text_view, insert_offset);
      /*
       * insert and selection_bound marks are different to a selection
       * has changed
       */
      if (selection_changed)
        g_signal_emit_by_name (accessible, "text_selection_changed");
      bail_text_view->previous_selection_bound = selection_bound;
    }
}

static void 
_bail_text_view_changed_cb (BtkTextBuffer *buffer,
                            gpointer      user_data)
{
  BtkTextView *text = (BtkTextView *) user_data;
  BatkObject *accessible;
  BailTextView *bail_text_view;

  accessible = btk_widget_get_accessible (BTK_WIDGET (text));
  bail_text_view = BAIL_TEXT_VIEW (accessible);
  if (bail_text_view->signal_name)
    {
      if (!bail_text_view->insert_notify_handler)
        {
          bail_text_view->insert_notify_handler = bdk_threads_add_idle (insert_idle_handler, accessible);
        }
      return;
    }
  emit_text_caret_moved (bail_text_view, get_insert_offset (buffer));
  bail_text_view->previous_selection_bound = get_selection_bound (buffer);
}

static gchar*
get_text_near_offset (BatkText          *text,
                      BailOffsetType   function,
                      BatkTextBoundary  boundary_type,
                      gint             offset,
                      gint             *start_offset,
                      gint             *end_offset)
{
  BtkTextView *view;
  gpointer layout = NULL;

  view = BTK_TEXT_VIEW (BTK_ACCESSIBLE (text)->widget);

  /*
   * Pass the BtkTextView to the function bail_text_util_get_text() 
   * so it can find the start and end of the current line on the display.
   */
  if (boundary_type == BATK_TEXT_BOUNDARY_LINE_START ||
      boundary_type == BATK_TEXT_BOUNDARY_LINE_END)
    layout = view;

  return bail_text_util_get_text (BAIL_TEXT_VIEW (text)->textutil, layout,
                                  function, boundary_type, offset, 
                                    start_offset, end_offset);
}

static gint
get_insert_offset (BtkTextBuffer *buffer)
{
  BtkTextMark *cursor_mark;
  BtkTextIter cursor_itr;

  cursor_mark = btk_text_buffer_get_insert (buffer);
  btk_text_buffer_get_iter_at_mark (buffer, &cursor_itr, cursor_mark);
  return btk_text_iter_get_offset (&cursor_itr);
}

static gint
get_selection_bound (BtkTextBuffer *buffer)
{
  BtkTextMark *selection_mark;
  BtkTextIter selection_itr;

  selection_mark = btk_text_buffer_get_selection_bound (buffer);
  btk_text_buffer_get_iter_at_mark (buffer, &selection_itr, selection_mark);
  return btk_text_iter_get_offset (&selection_itr);
}

static void
emit_text_caret_moved (BailTextView *bail_text_view,
                       gint          insert_offset)
{
  /*
   * If we have text which has been inserted notify the user
   */
  if (bail_text_view->insert_notify_handler)
    {
      g_source_remove (bail_text_view->insert_notify_handler);
      bail_text_view->insert_notify_handler = 0;
      insert_idle_handler (bail_text_view);
    }

  if (insert_offset != bail_text_view->previous_insert_offset)
    {
      /*
       * If the caret position has not changed then don't bother notifying
       *
       * When mouse click is used to change caret position, notification
       * is received on button down and button up.
       */
      g_signal_emit_by_name (bail_text_view, "text_caret_moved", insert_offset);
      bail_text_view->previous_insert_offset = insert_offset;
    }
}

static gint
insert_idle_handler (gpointer data)
{
  BailTextView *bail_text_view;
  BtkTextBuffer *buffer;

  bail_text_view = BAIL_TEXT_VIEW (data);

  g_signal_emit_by_name (data,
                         bail_text_view->signal_name,
                         bail_text_view->position,
                         bail_text_view->length);
  bail_text_view->signal_name = NULL;
  bail_text_view->position = 0;
  bail_text_view->length = 0;

  buffer = bail_text_view->textutil->buffer;
  if (bail_text_view->insert_notify_handler)
    {
    /*
     * If called from idle handler notify caret moved
     */
      bail_text_view->insert_notify_handler = 0;
      emit_text_caret_moved (bail_text_view, get_insert_offset (buffer));
      bail_text_view->previous_selection_bound = get_selection_bound (buffer);
    }

  return FALSE;
}

static void       
batk_streamable_content_interface_init    (BatkStreamableContentIface *iface)
{
  iface->get_n_mime_types = bail_streamable_content_get_n_mime_types;
  iface->get_mime_type = bail_streamable_content_get_mime_type;
  iface->get_stream = bail_streamable_content_get_stream;
}

static gint       bail_streamable_content_get_n_mime_types (BatkStreamableContent *streamable)
{
    gint n_mime_types = 0;

    if (BAIL_IS_TEXT_VIEW (streamable) && BAIL_TEXT_VIEW (streamable)->textutil)
    {
	int i;
	gboolean advertises_plaintext = FALSE;
	BdkAtom *atoms =
	     btk_text_buffer_get_serialize_formats (
		BAIL_TEXT_VIEW (streamable)->textutil->buffer, 
		&n_mime_types);
	for (i = 0; i < n_mime_types-1; ++i)
	    if (!strcmp ("text/plain", bdk_atom_name (atoms[i])))
		advertises_plaintext = TRUE;
	if (!advertises_plaintext) ++n_mime_types; 
        /* we support text/plain even if the BtkTextBuffer doesn't */
    }
    return n_mime_types;
}

static const gchar*
bail_streamable_content_get_mime_type (BatkStreamableContent *streamable, gint i)
{
    if (BAIL_IS_TEXT_VIEW (streamable) && BAIL_TEXT_VIEW (streamable)->textutil)
    {
	gint n_mime_types = 0;
	BdkAtom *atoms;
	atoms = btk_text_buffer_get_serialize_formats (
	    BAIL_TEXT_VIEW (streamable)->textutil->buffer, 
	    &n_mime_types);
	if (i < n_mime_types)
	{
	    return bdk_atom_name (atoms [i]);
	}
	else if (i == n_mime_types)
	    return "text/plain";
    }
    return NULL;
}

static BUNNYIOChannel*       bail_streamable_content_get_stream       (BatkStreamableContent *streamable,
								   const gchar *mime_type)
{
    gint i, n_mime_types = 0;
    BdkAtom *atoms;
    if (!BAIL_IS_TEXT_VIEW (streamable) || !BAIL_TEXT_VIEW (streamable)->textutil)
	return NULL;
    atoms = btk_text_buffer_get_serialize_formats (
	BAIL_TEXT_VIEW (streamable)->textutil->buffer, 
	&n_mime_types);
    for (i = 0; i < n_mime_types; ++i) 
    {
	if (!strcmp ("text/plain", mime_type) ||
	    !strcmp (bdk_atom_name (atoms[i]), mime_type)) {
	    BtkTextBuffer *buffer;
	    guint8 *cbuf;
	    GError *err = NULL;
	    gsize len, written;
	    gchar tname[80];
	    BtkTextIter start, end;
	    BUNNYIOChannel *bunnyio = NULL;
	    int fd;
	    buffer = BAIL_TEXT_VIEW (streamable)->textutil->buffer;
	    btk_text_buffer_get_iter_at_offset (buffer, &start, 0);
	    btk_text_buffer_get_iter_at_offset (buffer, &end, -1);
	    if (!strcmp ("text/plain", mime_type)) 
	    {
		cbuf = (guint8*) btk_text_buffer_get_text (buffer, &start, &end, FALSE);
		len = strlen ((const char *) cbuf); 
	    }
	    else
	    {
		cbuf = btk_text_buffer_serialize (buffer, buffer, atoms[i], &start, &end, &len);
	    }
	    g_snprintf (tname, 20, "streamXXXXXX");
	    fd = g_mkstemp (tname);
	    bunnyio = g_io_channel_unix_new (fd);
	    g_io_channel_set_encoding (bunnyio, NULL, &err);
	    if (!err) g_io_channel_write_chars (bunnyio, (const char *) cbuf, (gssize) len, &written, &err);
	    else g_message ("%s", err->message);
	    if (!err) g_io_channel_seek_position (bunnyio, 0, G_SEEK_SET, &err);
	    else g_message ("%s", err->message);
	    if (!err) g_io_channel_flush (bunnyio, &err);
	    else g_message ("%s", err->message);
	    if (err) {
		g_message ("<error writing to stream [%s]>", tname);
		g_error_free (err);
	    }
	    /* make sure the file is removed on unref of the bunnyiochannel */
	    else {
		g_unlink (tname);
		return bunnyio; 
	    }
	}
    }
    return NULL;
}

