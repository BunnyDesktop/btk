/* BTK - The GIMP Toolkit
 * btktextview.h Copyright (C) 2000 Red Hat, Inc.
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

#ifndef __BTK_TEXT_VIEW_H__
#define __BTK_TEXT_VIEW_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcontainer.h>
#include <btk/btkimcontext.h>
#include <btk/btktextbuffer.h>
#include <btk/btkmenu.h>

B_BEGIN_DECLS

#define BTK_TYPE_TEXT_VIEW             (btk_text_view_get_type ())
#define BTK_TEXT_VIEW(obj)             (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TEXT_VIEW, BtkTextView))
#define BTK_TEXT_VIEW_CLASS(klass)     (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TEXT_VIEW, BtkTextViewClass))
#define BTK_IS_TEXT_VIEW(obj)          (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TEXT_VIEW))
#define BTK_IS_TEXT_VIEW_CLASS(klass)  (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TEXT_VIEW))
#define BTK_TEXT_VIEW_GET_CLASS(obj)   (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TEXT_VIEW, BtkTextViewClass))

typedef enum
{
  BTK_TEXT_WINDOW_PRIVATE,
  BTK_TEXT_WINDOW_WIDGET,
  BTK_TEXT_WINDOW_TEXT,
  BTK_TEXT_WINDOW_LEFT,
  BTK_TEXT_WINDOW_RIGHT,
  BTK_TEXT_WINDOW_TOP,
  BTK_TEXT_WINDOW_BOTTOM
} BtkTextWindowType;

#define BTK_TEXT_VIEW_PRIORITY_VALIDATE (BDK_PRIORITY_REDRAW + 5)

typedef struct _BtkTextView BtkTextView;
typedef struct _BtkTextViewClass BtkTextViewClass;

/* Internal private types. */
typedef struct _BtkTextWindow BtkTextWindow;
typedef struct _BtkTextPendingScroll BtkTextPendingScroll;

struct _BtkTextView
{
  BtkContainer parent_instance;

  struct _BtkTextLayout *GSEAL (layout);
  BtkTextBuffer *GSEAL (buffer);

  guint GSEAL (selection_drag_handler);
  guint GSEAL (scroll_timeout);

  /* Default style settings */
  gint GSEAL (pixels_above_lines);
  gint GSEAL (pixels_below_lines);
  gint GSEAL (pixels_inside_wrap);
  BtkWrapMode GSEAL (wrap_mode);
  BtkJustification GSEAL (justify);
  gint GSEAL (left_margin);
  gint GSEAL (right_margin);
  gint GSEAL (indent);
  BangoTabArray *GSEAL (tabs);
  guint GSEAL (editable) : 1;

  guint GSEAL (overwrite_mode) : 1;
  guint GSEAL (cursor_visible) : 1;

  /* if we have reset the IM since the last character entered */  
  guint GSEAL (need_im_reset) : 1;

  guint GSEAL (accepts_tab) : 1;

  guint GSEAL (width_changed) : 1;

  /* debug flag - means that we've validated onscreen since the
   * last "invalidate" signal from the layout
   */
  guint GSEAL (onscreen_validated) : 1;

  guint GSEAL (mouse_cursor_obscured) : 1;

  BtkTextWindow *GSEAL (text_window);
  BtkTextWindow *GSEAL (left_window);
  BtkTextWindow *GSEAL (right_window);
  BtkTextWindow *GSEAL (top_window);
  BtkTextWindow *GSEAL (bottom_window);

  BtkAdjustment *GSEAL (hadjustment);
  BtkAdjustment *GSEAL (vadjustment);

  gint GSEAL (xoffset);         /* Offsets between widget coordinates and buffer coordinates */
  gint GSEAL (yoffset);
  gint GSEAL (width);           /* Width and height of the buffer */
  gint GSEAL (height);

  /* The virtual cursor position is normally the same as the
   * actual (strong) cursor position, except in two circumstances:
   *
   * a) When the cursor is moved vertically with the keyboard
   * b) When the text view is scrolled with the keyboard
   *
   * In case a), virtual_cursor_x is preserved, but not virtual_cursor_y
   * In case b), both virtual_cursor_x and virtual_cursor_y are preserved.
   */
  gint GSEAL (virtual_cursor_x);   /* -1 means use actual cursor position */
  gint GSEAL (virtual_cursor_y);   /* -1 means use actual cursor position */

  BtkTextMark *GSEAL (first_para_mark); /* Mark at the beginning of the first onscreen paragraph */
  gint GSEAL (first_para_pixels);       /* Offset of top of screen in the first onscreen paragraph */

  BtkTextMark *GSEAL (dnd_mark);
  guint GSEAL (blink_timeout);

  guint GSEAL (first_validate_idle);        /* Idle to revalidate onscreen portion, runs before resize */
  guint GSEAL (incremental_validate_idle);  /* Idle to revalidate offscreen portions, runs after redraw */

  BtkIMContext *GSEAL (im_context);
  BtkWidget *GSEAL (popup_menu);

  gint GSEAL (drag_start_x);
  gint GSEAL (drag_start_y);

  GSList *GSEAL (children);

  BtkTextPendingScroll *GSEAL (pending_scroll);

  gint GSEAL (pending_place_cursor_button);
};

struct _BtkTextViewClass
{
  BtkContainerClass parent_class;

  void (* set_scroll_adjustments)   (BtkTextView    *text_view,
                                     BtkAdjustment  *hadjustment,
                                     BtkAdjustment  *vadjustment);

  void (* populate_popup)           (BtkTextView    *text_view,
                                     BtkMenu        *menu);

  /* These are all RUN_ACTION signals for keybindings */

  /* move insertion point */
  void (* move_cursor) (BtkTextView    *text_view,
                        BtkMovementStep step,
                        gint            count,
                        gboolean        extend_selection);

  /* FIXME should be deprecated in favor of adding BTK_MOVEMENT_HORIZONTAL_PAGES
   * or something in BTK 2.2, was put in to avoid adding enum values during
   * the freeze.
   */
  void (* page_horizontally) (BtkTextView *text_view,
                              gint         count,
                              gboolean     extend_selection);

  /* move the "anchor" (what Emacs calls the mark) to the cursor position */
  void (* set_anchor)  (BtkTextView    *text_view);

  /* Edits */
  void (* insert_at_cursor)      (BtkTextView *text_view,
                                  const gchar *str);
  void (* delete_from_cursor)    (BtkTextView  *text_view,
                                  BtkDeleteType type,
                                  gint          count);
  void (* backspace)             (BtkTextView *text_view);

  /* cut copy paste */
  void (* cut_clipboard)   (BtkTextView *text_view);
  void (* copy_clipboard)  (BtkTextView *text_view);
  void (* paste_clipboard) (BtkTextView *text_view);
  /* overwrite */
  void (* toggle_overwrite) (BtkTextView *text_view);

  /* as of BTK+ 2.12 the "move-focus" signal has been moved to BtkWidget,
   * so this is merley a virtual function now. Overriding it in subclasses
   * continues to work though.
   */
  void (* move_focus)       (BtkTextView     *text_view,
                             BtkDirectionType direction);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
  void (*_btk_reserved5) (void);
  void (*_btk_reserved6) (void);
  void (*_btk_reserved7) (void);
};

GType          btk_text_view_get_type              (void) B_GNUC_CONST;
BtkWidget *    btk_text_view_new                   (void);
BtkWidget *    btk_text_view_new_with_buffer       (BtkTextBuffer *buffer);
void           btk_text_view_set_buffer            (BtkTextView   *text_view,
                                                    BtkTextBuffer *buffer);
BtkTextBuffer *btk_text_view_get_buffer            (BtkTextView   *text_view);
gboolean       btk_text_view_scroll_to_iter        (BtkTextView   *text_view,
                                                    BtkTextIter   *iter,
                                                    gdouble        within_margin,
                                                    gboolean       use_align,
                                                    gdouble        xalign,
                                                    gdouble        yalign);
void           btk_text_view_scroll_to_mark        (BtkTextView   *text_view,
                                                    BtkTextMark   *mark,
                                                    gdouble        within_margin,
                                                    gboolean       use_align,
                                                    gdouble        xalign,
                                                    gdouble        yalign);
void           btk_text_view_scroll_mark_onscreen  (BtkTextView   *text_view,
                                                    BtkTextMark   *mark);
gboolean       btk_text_view_move_mark_onscreen    (BtkTextView   *text_view,
                                                    BtkTextMark   *mark);
gboolean       btk_text_view_place_cursor_onscreen (BtkTextView   *text_view);

void           btk_text_view_get_visible_rect      (BtkTextView   *text_view,
                                                    BdkRectangle  *visible_rect);
void           btk_text_view_set_cursor_visible    (BtkTextView   *text_view,
                                                    gboolean       setting);
gboolean       btk_text_view_get_cursor_visible    (BtkTextView   *text_view);

void           btk_text_view_get_iter_location     (BtkTextView   *text_view,
                                                    const BtkTextIter *iter,
                                                    BdkRectangle  *location);
void           btk_text_view_get_iter_at_location  (BtkTextView   *text_view,
                                                    BtkTextIter   *iter,
                                                    gint           x,
                                                    gint           y);
void           btk_text_view_get_iter_at_position  (BtkTextView   *text_view,
                                                    BtkTextIter   *iter,
						    gint          *trailing,
                                                    gint           x,
                                                    gint           y);
void           btk_text_view_get_line_yrange       (BtkTextView       *text_view,
                                                    const BtkTextIter *iter,
                                                    gint              *y,
                                                    gint              *height);

void           btk_text_view_get_line_at_y         (BtkTextView       *text_view,
                                                    BtkTextIter       *target_iter,
                                                    gint               y,
                                                    gint              *line_top);

void btk_text_view_buffer_to_window_coords (BtkTextView       *text_view,
                                            BtkTextWindowType  win,
                                            gint               buffer_x,
                                            gint               buffer_y,
                                            gint              *window_x,
                                            gint              *window_y);
void btk_text_view_window_to_buffer_coords (BtkTextView       *text_view,
                                            BtkTextWindowType  win,
                                            gint               window_x,
                                            gint               window_y,
                                            gint              *buffer_x,
                                            gint              *buffer_y);

BtkAdjustment* btk_text_view_get_hadjustment (BtkTextView *text_view);
BtkAdjustment* btk_text_view_get_vadjustment (BtkTextView *text_view);

BdkWindow*        btk_text_view_get_window      (BtkTextView       *text_view,
                                                 BtkTextWindowType  win);
BtkTextWindowType btk_text_view_get_window_type (BtkTextView       *text_view,
                                                 BdkWindow         *window);

void btk_text_view_set_border_window_size (BtkTextView       *text_view,
                                           BtkTextWindowType  type,
                                           gint               size);
gint btk_text_view_get_border_window_size (BtkTextView       *text_view,
					   BtkTextWindowType  type);

gboolean btk_text_view_forward_display_line           (BtkTextView       *text_view,
                                                       BtkTextIter       *iter);
gboolean btk_text_view_backward_display_line          (BtkTextView       *text_view,
                                                       BtkTextIter       *iter);
gboolean btk_text_view_forward_display_line_end       (BtkTextView       *text_view,
                                                       BtkTextIter       *iter);
gboolean btk_text_view_backward_display_line_start    (BtkTextView       *text_view,
                                                       BtkTextIter       *iter);
gboolean btk_text_view_starts_display_line            (BtkTextView       *text_view,
                                                       const BtkTextIter *iter);
gboolean btk_text_view_move_visually                  (BtkTextView       *text_view,
                                                       BtkTextIter       *iter,
                                                       gint               count);

gboolean        btk_text_view_im_context_filter_keypress        (BtkTextView       *text_view,
                                                                 BdkEventKey       *event);
void            btk_text_view_reset_im_context                  (BtkTextView       *text_view);

/* Adding child widgets */
void btk_text_view_add_child_at_anchor (BtkTextView          *text_view,
                                        BtkWidget            *child,
                                        BtkTextChildAnchor   *anchor);

void btk_text_view_add_child_in_window (BtkTextView          *text_view,
                                        BtkWidget            *child,
                                        BtkTextWindowType     which_window,
                                        /* window coordinates */
                                        gint                  xpos,
                                        gint                  ypos);

void btk_text_view_move_child          (BtkTextView          *text_view,
                                        BtkWidget            *child,
                                        /* window coordinates */
                                        gint                  xpos,
                                        gint                  ypos);

/* Default style settings (fallbacks if no tag affects the property) */

void             btk_text_view_set_wrap_mode          (BtkTextView      *text_view,
                                                       BtkWrapMode       wrap_mode);
BtkWrapMode      btk_text_view_get_wrap_mode          (BtkTextView      *text_view);
void             btk_text_view_set_editable           (BtkTextView      *text_view,
                                                       gboolean          setting);
gboolean         btk_text_view_get_editable           (BtkTextView      *text_view);
void             btk_text_view_set_overwrite          (BtkTextView      *text_view,
						       gboolean          overwrite);
gboolean         btk_text_view_get_overwrite          (BtkTextView      *text_view);
void		 btk_text_view_set_accepts_tab        (BtkTextView	*text_view,
						       gboolean		 accepts_tab);
gboolean	 btk_text_view_get_accepts_tab        (BtkTextView	*text_view);
void             btk_text_view_set_pixels_above_lines (BtkTextView      *text_view,
                                                       gint              pixels_above_lines);
gint             btk_text_view_get_pixels_above_lines (BtkTextView      *text_view);
void             btk_text_view_set_pixels_below_lines (BtkTextView      *text_view,
                                                       gint              pixels_below_lines);
gint             btk_text_view_get_pixels_below_lines (BtkTextView      *text_view);
void             btk_text_view_set_pixels_inside_wrap (BtkTextView      *text_view,
                                                       gint              pixels_inside_wrap);
gint             btk_text_view_get_pixels_inside_wrap (BtkTextView      *text_view);
void             btk_text_view_set_justification      (BtkTextView      *text_view,
                                                       BtkJustification  justification);
BtkJustification btk_text_view_get_justification      (BtkTextView      *text_view);
void             btk_text_view_set_left_margin        (BtkTextView      *text_view,
                                                       gint              left_margin);
gint             btk_text_view_get_left_margin        (BtkTextView      *text_view);
void             btk_text_view_set_right_margin       (BtkTextView      *text_view,
                                                       gint              right_margin);
gint             btk_text_view_get_right_margin       (BtkTextView      *text_view);
void             btk_text_view_set_indent             (BtkTextView      *text_view,
                                                       gint              indent);
gint             btk_text_view_get_indent             (BtkTextView      *text_view);
void             btk_text_view_set_tabs               (BtkTextView      *text_view,
                                                       BangoTabArray    *tabs);
BangoTabArray*   btk_text_view_get_tabs               (BtkTextView      *text_view);

/* note that the return value of this changes with the theme */
BtkTextAttributes* btk_text_view_get_default_attributes (BtkTextView    *text_view);

B_END_DECLS

#endif /* __BTK_TEXT_VIEW_H__ */
