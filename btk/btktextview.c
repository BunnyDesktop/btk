/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* BTK - The GIMP Toolkit
 * btktextview.c Copyright (C) 2000 Red Hat, Inc.
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

#include "config.h"
#include <string.h>

#define BTK_TEXT_USE_INTERNAL_UNSUPPORTED_API
#include "btkbindings.h"
#include "btkdnd.h"
#include "btkimagemenuitem.h"
#include "btkintl.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkmenu.h"
#include "btkmenuitem.h"
#include "btkseparatormenuitem.h"
#include "btksettings.h"
#include "btkstock.h"
#include "btktextbufferrichtext.h"
#include "btktextdisplay.h"
#include "btktextview.h"
#include "btkimmulticontext.h"
#include "bdk/bdkkeysyms.h"
#include "btkprivate.h"
#include "btktextutil.h"
#include "btkwindow.h"
#include "btkalias.h"

/* How scrolling, validation, exposes, etc. work.
 *
 * The expose_event handler has the invariant that the onscreen lines
 * have been validated.
 *
 * There are two ways that onscreen lines can become invalid. The first
 * is to change which lines are onscreen. This happens when the value
 * of a scroll adjustment changes. So the code path begins in
 * btk_text_view_value_changed() and goes like this:
 *   - bdk_window_scroll() to reflect the new adjustment value
 *   - validate the lines that were moved onscreen
 *   - bdk_window_process_updates() to handle the exposes immediately
 *
 * The second way is that you get the "invalidated" signal from the layout,
 * indicating that lines have become invalid. This code path begins in
 * invalidated_handler() and goes like this:
 *   - install high-priority idle which does the rest of the steps
 *   - if a scroll is pending from scroll_to_mark(), do the scroll,
 *     jumping to the btk_text_view_value_changed() code path
 *   - otherwise, validate the onscreen lines
 *   - DO NOT process updates
 *
 * In both cases, validating the onscreen lines can trigger a scroll
 * due to maintaining the first_para on the top of the screen.
 * If validation triggers a scroll, we jump to the top of the code path
 * for value_changed, and bail out of the current code path.
 *
 * Also, in size_allocate, if we invalidate some lines from changing
 * the layout width, we need to go ahead and run the high-priority idle,
 * because BTK sends exposes right after doing the size allocates without
 * returning to the main loop. This is also why the high-priority idle
 * is at a higher priority than resizing.
 *
 */

#if 0
#define DEBUG_VALIDATION_AND_SCROLLING
#endif

#ifdef DEBUG_VALIDATION_AND_SCROLLING
#define DV(x) (x)
#else
#define DV(x)
#endif

#define SCREEN_WIDTH(widget) text_window_get_width (BTK_TEXT_VIEW (widget)->text_window)
#define SCREEN_HEIGHT(widget) text_window_get_height (BTK_TEXT_VIEW (widget)->text_window)

#define SPACE_FOR_CURSOR 1

typedef struct _BtkTextViewPrivate BtkTextViewPrivate;

#define BTK_TEXT_VIEW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_TEXT_VIEW, BtkTextViewPrivate))

struct _BtkTextViewPrivate 
{
  guint blink_time;  /* time in msec the cursor has blinked since last user event */
  guint im_spot_idle;
  gchar *im_module;
  guint scroll_after_paste : 1;
};


struct _BtkTextPendingScroll
{
  BtkTextMark   *mark;
  gdouble        within_margin;
  gboolean       use_align;
  gdouble        xalign;
  gdouble        yalign;
};
  
enum
{
  SET_SCROLL_ADJUSTMENTS,
  POPULATE_POPUP,
  MOVE_CURSOR,
  PAGE_HORIZONTALLY,
  SET_ANCHOR,
  INSERT_AT_CURSOR,
  DELETE_FROM_CURSOR,
  BACKSPACE,
  CUT_CLIPBOARD,
  COPY_CLIPBOARD,
  PASTE_CLIPBOARD,
  TOGGLE_OVERWRITE,
  MOVE_VIEWPORT,
  SELECT_ALL,
  TOGGLE_CURSOR_VISIBLE,
  PREEDIT_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_PIXELS_ABOVE_LINES,
  PROP_PIXELS_BELOW_LINES,
  PROP_PIXELS_INSIDE_WRAP,
  PROP_EDITABLE,
  PROP_WRAP_MODE,
  PROP_JUSTIFICATION,
  PROP_LEFT_MARGIN,
  PROP_RIGHT_MARGIN,
  PROP_INDENT,
  PROP_TABS,
  PROP_CURSOR_VISIBLE,
  PROP_BUFFER,
  PROP_OVERWRITE,
  PROP_ACCEPTS_TAB,
  PROP_IM_MODULE
};

static void btk_text_view_destroy              (BtkObject        *object);
static void btk_text_view_finalize             (GObject          *object);
static void btk_text_view_set_property         (GObject         *object,
						guint            prop_id,
						const GValue    *value,
						GParamSpec      *pspec);
static void btk_text_view_get_property         (GObject         *object,
						guint            prop_id,
						GValue          *value,
						GParamSpec      *pspec);
static void btk_text_view_size_request         (BtkWidget        *widget,
                                                BtkRequisition   *requisition);
static void btk_text_view_size_allocate        (BtkWidget        *widget,
                                                BtkAllocation    *allocation);
static void btk_text_view_realize              (BtkWidget        *widget);
static void btk_text_view_unrealize            (BtkWidget        *widget);
static void btk_text_view_style_set            (BtkWidget        *widget,
                                                BtkStyle         *previous_style);
static void btk_text_view_direction_changed    (BtkWidget        *widget,
                                                BtkTextDirection  previous_direction);
static void btk_text_view_grab_notify          (BtkWidget        *widget,
					        gboolean         was_grabbed);
static void btk_text_view_state_changed        (BtkWidget        *widget,
					        BtkStateType      previous_state);

static gint btk_text_view_event                (BtkWidget        *widget,
                                                BdkEvent         *event);
static gint btk_text_view_key_press_event      (BtkWidget        *widget,
                                                BdkEventKey      *event);
static gint btk_text_view_key_release_event    (BtkWidget        *widget,
                                                BdkEventKey      *event);
static gint btk_text_view_button_press_event   (BtkWidget        *widget,
                                                BdkEventButton   *event);
static gint btk_text_view_button_release_event (BtkWidget        *widget,
                                                BdkEventButton   *event);
static gint btk_text_view_focus_in_event       (BtkWidget        *widget,
                                                BdkEventFocus    *event);
static gint btk_text_view_focus_out_event      (BtkWidget        *widget,
                                                BdkEventFocus    *event);
static gint btk_text_view_motion_event         (BtkWidget        *widget,
                                                BdkEventMotion   *event);
static gint btk_text_view_expose_event         (BtkWidget        *widget,
                                                BdkEventExpose   *expose);
static void btk_text_view_draw_focus           (BtkWidget        *widget);
static gboolean btk_text_view_focus            (BtkWidget        *widget,
                                                BtkDirectionType  direction);
static void btk_text_view_move_focus           (BtkWidget        *widget,
                                                BtkDirectionType  direction_type);
static void btk_text_view_select_all           (BtkWidget        *widget,
                                                gboolean          select);


/* Source side drag signals */
static void btk_text_view_drag_begin       (BtkWidget        *widget,
                                            BdkDragContext   *context);
static void btk_text_view_drag_end         (BtkWidget        *widget,
                                            BdkDragContext   *context);
static void btk_text_view_drag_data_get    (BtkWidget        *widget,
                                            BdkDragContext   *context,
                                            BtkSelectionData *selection_data,
                                            guint             info,
                                            guint             time);
static void btk_text_view_drag_data_delete (BtkWidget        *widget,
                                            BdkDragContext   *context);

/* Target side drag signals */
static void     btk_text_view_drag_leave         (BtkWidget        *widget,
                                                  BdkDragContext   *context,
                                                  guint             time);
static gboolean btk_text_view_drag_motion        (BtkWidget        *widget,
                                                  BdkDragContext   *context,
                                                  gint              x,
                                                  gint              y,
                                                  guint             time);
static gboolean btk_text_view_drag_drop          (BtkWidget        *widget,
                                                  BdkDragContext   *context,
                                                  gint              x,
                                                  gint              y,
                                                  guint             time);
static void     btk_text_view_drag_data_received (BtkWidget        *widget,
                                                  BdkDragContext   *context,
                                                  gint              x,
                                                  gint              y,
                                                  BtkSelectionData *selection_data,
                                                  guint             info,
                                                  guint             time);

static void btk_text_view_set_scroll_adjustments (BtkTextView   *text_view,
                                                  BtkAdjustment *hadj,
                                                  BtkAdjustment *vadj);
static gboolean btk_text_view_popup_menu         (BtkWidget     *widget);

static void btk_text_view_move_cursor       (BtkTextView           *text_view,
                                             BtkMovementStep        step,
                                             gint                   count,
                                             gboolean               extend_selection);
static void btk_text_view_page_horizontally (BtkTextView          *text_view,
                                             gint                  count,
                                             gboolean              extend_selection);
static gboolean btk_text_view_move_viewport (BtkTextView           *text_view,
                                             BtkScrollStep          step,
                                             gint                   count);
static void btk_text_view_set_anchor       (BtkTextView           *text_view);
static gboolean btk_text_view_scroll_pages (BtkTextView           *text_view,
                                            gint                   count,
                                            gboolean               extend_selection);
static gboolean btk_text_view_scroll_hpages(BtkTextView           *text_view,
                                            gint                   count,
                                            gboolean               extend_selection);
static void btk_text_view_insert_at_cursor (BtkTextView           *text_view,
                                            const gchar           *str);
static void btk_text_view_delete_from_cursor (BtkTextView           *text_view,
                                              BtkDeleteType          type,
                                              gint                   count);
static void btk_text_view_backspace        (BtkTextView           *text_view);
static void btk_text_view_cut_clipboard    (BtkTextView           *text_view);
static void btk_text_view_copy_clipboard   (BtkTextView           *text_view);
static void btk_text_view_paste_clipboard  (BtkTextView           *text_view);
static void btk_text_view_toggle_overwrite (BtkTextView           *text_view);
static void btk_text_view_toggle_cursor_visible (BtkTextView      *text_view);
static void btk_text_view_compat_move_focus(BtkTextView           *text_view,
                                            BtkDirectionType       direction_type);
static void btk_text_view_unselect         (BtkTextView           *text_view);

static void     btk_text_view_validate_onscreen     (BtkTextView        *text_view);
static void     btk_text_view_get_first_para_iter   (BtkTextView        *text_view,
                                                     BtkTextIter        *iter);
static void     btk_text_view_update_layout_width       (BtkTextView        *text_view);
static void     btk_text_view_set_attributes_from_style (BtkTextView        *text_view,
                                                         BtkTextAttributes *values,
                                                         BtkStyle           *style);
static void     btk_text_view_ensure_layout          (BtkTextView        *text_view);
static void     btk_text_view_destroy_layout         (BtkTextView        *text_view);
static void     btk_text_view_check_keymap_direction (BtkTextView        *text_view);
static void     btk_text_view_start_selection_drag   (BtkTextView        *text_view,
                                                      const BtkTextIter  *iter,
                                                      BdkEventButton     *event);
static gboolean btk_text_view_end_selection_drag     (BtkTextView        *text_view);
static void     btk_text_view_start_selection_dnd    (BtkTextView        *text_view,
                                                      const BtkTextIter  *iter,
                                                      BdkEventMotion     *event);
static void     btk_text_view_check_cursor_blink     (BtkTextView        *text_view);
static void     btk_text_view_pend_cursor_blink      (BtkTextView        *text_view);
static void     btk_text_view_stop_cursor_blink      (BtkTextView        *text_view);
static void     btk_text_view_reset_blink_time       (BtkTextView        *text_view);

static void     btk_text_view_value_changed                (BtkAdjustment *adj,
							    BtkTextView   *view);
static void     btk_text_view_commit_handler               (BtkIMContext  *context,
							    const gchar   *str,
							    BtkTextView   *text_view);
static void     btk_text_view_commit_text                  (BtkTextView   *text_view,
                                                            const gchar   *text);
static void     btk_text_view_preedit_changed_handler      (BtkIMContext  *context,
							    BtkTextView   *text_view);
static gboolean btk_text_view_retrieve_surrounding_handler (BtkIMContext  *context,
							    BtkTextView   *text_view);
static gboolean btk_text_view_delete_surrounding_handler   (BtkIMContext  *context,
							    gint           offset,
							    gint           n_chars,
							    BtkTextView   *text_view);

static void btk_text_view_mark_set_handler       (BtkTextBuffer     *buffer,
                                                  const BtkTextIter *location,
                                                  BtkTextMark       *mark,
                                                  gpointer           data);
static void btk_text_view_target_list_notify     (BtkTextBuffer     *buffer,
                                                  const GParamSpec  *pspec,
                                                  gpointer           data);
static void btk_text_view_paste_done_handler     (BtkTextBuffer     *buffer,
                                                  BtkClipboard      *clipboard,
                                                  gpointer           data);
static void btk_text_view_get_cursor_location    (BtkTextView       *text_view,
						  BdkRectangle      *pos);
static void btk_text_view_get_virtual_cursor_pos (BtkTextView       *text_view,
                                                  BtkTextIter       *cursor,
                                                  gint              *x,
                                                  gint              *y);
static void btk_text_view_set_virtual_cursor_pos (BtkTextView       *text_view,
                                                  gint               x,
                                                  gint               y);

static BtkAdjustment* get_hadjustment            (BtkTextView       *text_view);
static BtkAdjustment* get_vadjustment            (BtkTextView       *text_view);

static void btk_text_view_do_popup               (BtkTextView       *text_view,
						  BdkEventButton    *event);

static void cancel_pending_scroll                (BtkTextView   *text_view);
static void btk_text_view_queue_scroll           (BtkTextView   *text_view,
                                                  BtkTextMark   *mark,
                                                  gdouble        within_margin,
                                                  gboolean       use_align,
                                                  gdouble        xalign,
                                                  gdouble        yalign);

static gboolean btk_text_view_flush_scroll         (BtkTextView *text_view);
static void     btk_text_view_update_adjustments   (BtkTextView *text_view);
static void     btk_text_view_invalidate           (BtkTextView *text_view);
static void     btk_text_view_flush_first_validate (BtkTextView *text_view);

static void btk_text_view_update_im_spot_location (BtkTextView *text_view);

/* Container methods */
static void btk_text_view_add    (BtkContainer *container,
                                  BtkWidget    *child);
static void btk_text_view_remove (BtkContainer *container,
                                  BtkWidget    *child);
static void btk_text_view_forall (BtkContainer *container,
                                  gboolean      include_internals,
                                  BtkCallback   callback,
                                  gpointer      callback_data);

/* FIXME probably need the focus methods. */

typedef struct _BtkTextViewChild BtkTextViewChild;

struct _BtkTextViewChild
{
  BtkWidget *widget;

  BtkTextChildAnchor *anchor;

  gint from_top_of_line;
  gint from_left_of_buffer;
  
  /* These are ignored if anchor != NULL */
  BtkTextWindowType type;
  gint x;
  gint y;
};

static BtkTextViewChild* text_view_child_new_anchored      (BtkWidget          *child,
							    BtkTextChildAnchor *anchor,
							    BtkTextLayout      *layout);
static BtkTextViewChild* text_view_child_new_window        (BtkWidget          *child,
							    BtkTextWindowType   type,
							    gint                x,
							    gint                y);
static void              text_view_child_free              (BtkTextViewChild   *child);
static void              text_view_child_set_parent_window (BtkTextView        *text_view,
							    BtkTextViewChild   *child);

struct _BtkTextWindow
{
  BtkTextWindowType type;
  BtkWidget *widget;
  BdkWindow *window;
  BdkWindow *bin_window;
  BtkRequisition requisition;
  BdkRectangle allocation;
};

static BtkTextWindow *text_window_new             (BtkTextWindowType  type,
                                                   BtkWidget         *widget,
                                                   gint               width_request,
                                                   gint               height_request);
static void           text_window_free            (BtkTextWindow     *win);
static void           text_window_realize         (BtkTextWindow     *win,
                                                   BtkWidget         *widget);
static void           text_window_unrealize       (BtkTextWindow     *win);
static void           text_window_size_allocate   (BtkTextWindow     *win,
                                                   BdkRectangle      *rect);
static void           text_window_scroll          (BtkTextWindow     *win,
                                                   gint               dx,
                                                   gint               dy);
static void           text_window_invalidate_rect (BtkTextWindow     *win,
                                                   BdkRectangle      *rect);
static void           text_window_invalidate_cursors (BtkTextWindow  *win);

static gint           text_window_get_width       (BtkTextWindow     *win);
static gint           text_window_get_height      (BtkTextWindow     *win);


static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BtkTextView, btk_text_view, BTK_TYPE_CONTAINER)

static void
add_move_binding (BtkBindingSet  *binding_set,
                  guint           keyval,
                  guint           modmask,
                  BtkMovementStep step,
                  gint            count)
{
  g_assert ((modmask & BDK_SHIFT_MASK) == 0);

  btk_binding_entry_add_signal (binding_set, keyval, modmask,
                                "move-cursor", 3,
                                G_TYPE_ENUM, step,
                                G_TYPE_INT, count,
                                G_TYPE_BOOLEAN, FALSE);

  /* Selection-extending version */
  btk_binding_entry_add_signal (binding_set, keyval, modmask | BDK_SHIFT_MASK,
                                "move-cursor", 3,
                                G_TYPE_ENUM, step,
                                G_TYPE_INT, count,
                                G_TYPE_BOOLEAN, TRUE);
}

static void
btk_text_view_class_init (BtkTextViewClass *klass)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (klass);
  BtkObjectClass *object_class = BTK_OBJECT_CLASS (klass);
  BtkWidgetClass *widget_class = BTK_WIDGET_CLASS (klass);
  BtkContainerClass *container_class = BTK_CONTAINER_CLASS (klass);
  BtkBindingSet *binding_set;

  /* Default handlers and virtual methods
   */
  bobject_class->set_property = btk_text_view_set_property;
  bobject_class->get_property = btk_text_view_get_property;

  object_class->destroy = btk_text_view_destroy;
  bobject_class->finalize = btk_text_view_finalize;

  widget_class->realize = btk_text_view_realize;
  widget_class->unrealize = btk_text_view_unrealize;
  widget_class->style_set = btk_text_view_style_set;
  widget_class->direction_changed = btk_text_view_direction_changed;
  widget_class->grab_notify = btk_text_view_grab_notify;
  widget_class->state_changed = btk_text_view_state_changed;
  widget_class->size_request = btk_text_view_size_request;
  widget_class->size_allocate = btk_text_view_size_allocate;
  widget_class->event = btk_text_view_event;
  widget_class->key_press_event = btk_text_view_key_press_event;
  widget_class->key_release_event = btk_text_view_key_release_event;
  widget_class->button_press_event = btk_text_view_button_press_event;
  widget_class->button_release_event = btk_text_view_button_release_event;
  widget_class->focus_in_event = btk_text_view_focus_in_event;
  widget_class->focus_out_event = btk_text_view_focus_out_event;
  widget_class->motion_notify_event = btk_text_view_motion_event;
  widget_class->expose_event = btk_text_view_expose_event;
  widget_class->focus = btk_text_view_focus;

  /* need to override the base class function via override_class_handler,
   * because the signal slot is not available in BtkWidgetCLass
   */
  g_signal_override_class_handler ("move-focus",
                                   BTK_TYPE_TEXT_VIEW,
                                   G_CALLBACK (btk_text_view_move_focus));

  widget_class->drag_begin = btk_text_view_drag_begin;
  widget_class->drag_end = btk_text_view_drag_end;
  widget_class->drag_data_get = btk_text_view_drag_data_get;
  widget_class->drag_data_delete = btk_text_view_drag_data_delete;

  widget_class->drag_leave = btk_text_view_drag_leave;
  widget_class->drag_motion = btk_text_view_drag_motion;
  widget_class->drag_drop = btk_text_view_drag_drop;
  widget_class->drag_data_received = btk_text_view_drag_data_received;

  widget_class->popup_menu = btk_text_view_popup_menu;
  
  container_class->add = btk_text_view_add;
  container_class->remove = btk_text_view_remove;
  container_class->forall = btk_text_view_forall;

  klass->move_cursor = btk_text_view_move_cursor;
  klass->page_horizontally = btk_text_view_page_horizontally;
  klass->set_anchor = btk_text_view_set_anchor;
  klass->insert_at_cursor = btk_text_view_insert_at_cursor;
  klass->delete_from_cursor = btk_text_view_delete_from_cursor;
  klass->backspace = btk_text_view_backspace;
  klass->cut_clipboard = btk_text_view_cut_clipboard;
  klass->copy_clipboard = btk_text_view_copy_clipboard;
  klass->paste_clipboard = btk_text_view_paste_clipboard;
  klass->toggle_overwrite = btk_text_view_toggle_overwrite;
  klass->move_focus = btk_text_view_compat_move_focus;
  klass->set_scroll_adjustments = btk_text_view_set_scroll_adjustments;

  /*
   * Properties
   */
 
  g_object_class_install_property (bobject_class,
                                   PROP_PIXELS_ABOVE_LINES,
                                   g_param_spec_int ("pixels-above-lines",
						     P_("Pixels Above Lines"),
						     P_("Pixels of blank space above paragraphs"),
						     0,
						     G_MAXINT,
						     0,
						     BTK_PARAM_READWRITE));
 
  g_object_class_install_property (bobject_class,
                                   PROP_PIXELS_BELOW_LINES,
                                   g_param_spec_int ("pixels-below-lines",
						     P_("Pixels Below Lines"),
						     P_("Pixels of blank space below paragraphs"),
						     0,
						     G_MAXINT,
						     0,
						     BTK_PARAM_READWRITE));
 
  g_object_class_install_property (bobject_class,
                                   PROP_PIXELS_INSIDE_WRAP,
                                   g_param_spec_int ("pixels-inside-wrap",
						     P_("Pixels Inside Wrap"),
						     P_("Pixels of blank space between wrapped lines in a paragraph"),
						     0,
						     G_MAXINT,
						     0,
						     BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_EDITABLE,
                                   g_param_spec_boolean ("editable",
							 P_("Editable"),
							 P_("Whether the text can be modified by the user"),
							 TRUE,
							 BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_WRAP_MODE,
                                   g_param_spec_enum ("wrap-mode",
						      P_("Wrap Mode"),
						      P_("Whether to wrap lines never, at word boundaries, or at character boundaries"),
						      BTK_TYPE_WRAP_MODE,
						      BTK_WRAP_NONE,
						      BTK_PARAM_READWRITE));
 
  g_object_class_install_property (bobject_class,
                                   PROP_JUSTIFICATION,
                                   g_param_spec_enum ("justification",
						      P_("Justification"),
						      P_("Left, right, or center justification"),
						      BTK_TYPE_JUSTIFICATION,
						      BTK_JUSTIFY_LEFT,
						      BTK_PARAM_READWRITE));
 
  g_object_class_install_property (bobject_class,
                                   PROP_LEFT_MARGIN,
                                   g_param_spec_int ("left-margin",
						     P_("Left Margin"),
						     P_("Width of the left margin in pixels"),
						     0,
						     G_MAXINT,
						     0,
						     BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_RIGHT_MARGIN,
                                   g_param_spec_int ("right-margin",
						     P_("Right Margin"),
						     P_("Width of the right margin in pixels"),
						     0,
						     G_MAXINT,
						     0,
						     BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_INDENT,
                                   g_param_spec_int ("indent",
						     P_("Indent"),
						     P_("Amount to indent the paragraph, in pixels"),
						     G_MININT,
						     G_MAXINT,
						     0,
						     BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_TABS,
                                   g_param_spec_boxed ("tabs",
                                                       P_("Tabs"),
                                                       P_("Custom tabs for this text"),
                                                       BANGO_TYPE_TAB_ARRAY,
						       BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_CURSOR_VISIBLE,
                                   g_param_spec_boolean ("cursor-visible",
							 P_("Cursor Visible"),
							 P_("If the insertion cursor is shown"),
							 TRUE,
							 BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_BUFFER,
                                   g_param_spec_object ("buffer",
							P_("Buffer"),
							P_("The buffer which is displayed"),
							BTK_TYPE_TEXT_BUFFER,
							BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_OVERWRITE,
                                   g_param_spec_boolean ("overwrite",
							 P_("Overwrite mode"),
							 P_("Whether entered text overwrites existing contents"),
							 FALSE,
							 BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_ACCEPTS_TAB,
                                   g_param_spec_boolean ("accepts-tab",
							 P_("Accepts tab"),
							 P_("Whether Tab will result in a tab character being entered"),
							 TRUE,
							 BTK_PARAM_READWRITE));

   /**
    * BtkTextView:im-module:
    *
    * Which IM (input method) module should be used for this entry. 
    * See #BtkIMContext.
    *
    * Setting this to a non-%NULL value overrides the
    * system-wide IM module setting. See the BtkSettings 
    * #BtkSettings:btk-im-module property.
    *
    * Since: 2.16
    */
   g_object_class_install_property (bobject_class,
                                    PROP_IM_MODULE,
                                    g_param_spec_string ("im-module",
                                                         P_("IM module"),
                                                         P_("Which IM module should be used"),
                                                         NULL,
                                                         BTK_PARAM_READWRITE));

  /*
   * Style properties
   */
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_boxed ("error-underline-color",
							       P_("Error underline color"),
							       P_("Color with which to draw error-indication underlines"),
							       BDK_TYPE_COLOR,
							       BTK_PARAM_READABLE));
  
  /*
   * Signals
   */

  /**
   * BtkTextView::move-cursor: 
   * @text_view: the object which received the signal
   * @step: the granularity of the move, as a #BtkMovementStep
   * @count: the number of @step units to move
   * @extend_selection: %TRUE if the move should extend the selection
   *  
   * The ::move-cursor signal is a 
   * <link linkend="keybinding-signals">keybinding signal</link> 
   * which gets emitted when the user initiates a cursor movement. 
   * If the cursor is not visible in @text_view, this signal causes
   * the viewport to be moved instead.
   *
   * Applications should not connect to it, but may emit it with 
   * g_signal_emit_by_name() if they need to control the cursor
   * programmatically.
   *
   * The default bindings for this signal come in two variants,
   * the variant with the Shift modifier extends the selection,
   * the variant without the Shift modifer does not.
   * There are too many key combinations to list them all here.
   * <itemizedlist>
   * <listitem>Arrow keys move by individual characters/lines</listitem>
   * <listitem>Ctrl-arrow key combinations move by words/paragraphs</listitem>
   * <listitem>Home/End keys move to the ends of the buffer</listitem>
   * <listitem>PageUp/PageDown keys move vertically by pages</listitem>
   * <listitem>Ctrl-PageUp/PageDown keys move horizontally by pages</listitem>
   * </itemizedlist>
   */
  signals[MOVE_CURSOR] = 
    g_signal_new (I_("move-cursor"),
		  G_OBJECT_CLASS_TYPE (bobject_class), 
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, 
		  G_STRUCT_OFFSET (BtkTextViewClass, move_cursor),
		  NULL, NULL, 
		  _btk_marshal_VOID__ENUM_INT_BOOLEAN, 
		  G_TYPE_NONE, 3,
		  BTK_TYPE_MOVEMENT_STEP, 
		  G_TYPE_INT, 
		  G_TYPE_BOOLEAN);

  /**
   * BtkTextView::page-horizontally:
   * @text_view: the object which received the signal
   * @count: the number of @step units to move
   * @extend_selection: %TRUE if the move should extend the selection
   *
   * The ::page-horizontally signal is a 
   * <link linkend="keybinding-signals">keybinding signal</link> 
   * which can be bound to key combinations to allow the user
   * to initiate horizontal cursor movement by pages.  
   * 
   * This signal should not be used anymore, instead use the
   * #BtkTextview::move-cursor signal with the #BTK_MOVEMENT_HORIZONTAL_PAGES
   * granularity.
   */
  signals[PAGE_HORIZONTALLY] =
    g_signal_new (I_("page-horizontally"),
		  G_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkTextViewClass, page_horizontally),
		  NULL, NULL,
		  _btk_marshal_VOID__INT_BOOLEAN,
		  G_TYPE_NONE, 2,
		  G_TYPE_INT,
		  G_TYPE_BOOLEAN);
  
  /**
   * BtkTextView::move-viewport:
   * @text_view: the object which received the signal
   * @step: the granularity of the move, as a #BtkMovementStep
   * @count: the number of @step units to move
   *
   * The ::move-viewport signal is a 
   * <link linkend="keybinding-signals">keybinding signal</link> 
   * which can be bound to key combinations to allow the user
   * to move the viewport, i.e. change what part of the text view
   * is visible in a containing scrolled window.
   *
   * There are no default bindings for this signal.
   */
  signals[MOVE_VIEWPORT] =
    g_signal_new_class_handler (I_("move-viewport"),
                                G_OBJECT_CLASS_TYPE (bobject_class),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (btk_text_view_move_viewport),
                                NULL, NULL,
                                _btk_marshal_VOID__ENUM_INT,
                                G_TYPE_NONE, 2,
                                BTK_TYPE_SCROLL_STEP,
                                G_TYPE_INT);

  /**
   * BtkTextView::set-anchor:
   * @text_view: the object which received the signal
   *
   * The ::set-anchor signal is a
   * <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted when the user initiates setting the "anchor" 
   * mark. The "anchor" mark gets placed at the same position as the
   * "insert" mark.
   *
   * This signal has no default bindings.
   */   
  signals[SET_ANCHOR] =
    g_signal_new (I_("set-anchor"),
		  G_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkTextViewClass, set_anchor),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * BtkTextView::insert-at-cursor:
   * @text_view: the object which received the signal
   * @string: the string to insert
   *
   * The ::insert-at-cursor signal is a
   * <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted when the user initiates the insertion of a 
   * fixed string at the cursor.
   *
   * This signal has no default bindings.
   */
  signals[INSERT_AT_CURSOR] =
    g_signal_new (I_("insert-at-cursor"),
		  G_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkTextViewClass, insert_at_cursor),
		  NULL, NULL,
		  _btk_marshal_VOID__STRING,
		  G_TYPE_NONE, 1,
		  G_TYPE_STRING);

  /**
   * BtkTextView::delete-from-cursor:
   * @text_view: the object which received the signal
   * @type: the granularity of the deletion, as a #BtkDeleteType
   * @count: the number of @type units to delete
   *
   * The ::delete-from-cursor signal is a 
   * <link linkend="keybinding-signals">keybinding signal</link> 
   * which gets emitted when the user initiates a text deletion.
   *
   * If the @type is %BTK_DELETE_CHARS, BTK+ deletes the selection
   * if there is one, otherwise it deletes the requested number
   * of characters.
   *
   * The default bindings for this signal are
   * Delete for deleting a character, Ctrl-Delete for 
   * deleting a word and Ctrl-Backspace for deleting a word 
   * backwords.
   */
  signals[DELETE_FROM_CURSOR] =
    g_signal_new (I_("delete-from-cursor"),
		  G_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkTextViewClass, delete_from_cursor),
		  NULL, NULL,
		  _btk_marshal_VOID__ENUM_INT,
		  G_TYPE_NONE, 2,
		  BTK_TYPE_DELETE_TYPE,
		  G_TYPE_INT);

  /**
   * BtkTextView::backspace:
   * @text_view: the object which received the signal
   *
   * The ::backspace signal is a 
   * <link linkend="keybinding-signals">keybinding signal</link> 
   * which gets emitted when the user asks for it.
   * 
   * The default bindings for this signal are
   * Backspace and Shift-Backspace.
   */
  signals[BACKSPACE] =
    g_signal_new (I_("backspace"),
		  G_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkTextViewClass, backspace),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * BtkTextView::cut-clipboard:
   * @text_view: the object which received the signal
   *
   * The ::cut-clipboard signal is a 
   * <link linkend="keybinding-signals">keybinding signal</link> 
   * which gets emitted to cut the selection to the clipboard.
   * 
   * The default bindings for this signal are
   * Ctrl-x and Shift-Delete.
   */
  signals[CUT_CLIPBOARD] =
    g_signal_new (I_("cut-clipboard"),
		  G_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkTextViewClass, cut_clipboard),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * BtkTextView::copy-clipboard:
   * @text_view: the object which received the signal
   *
   * The ::copy-clipboard signal is a 
   * <link linkend="keybinding-signals">keybinding signal</link> 
   * which gets emitted to copy the selection to the clipboard.
   * 
   * The default bindings for this signal are
   * Ctrl-c and Ctrl-Insert.
   */
  signals[COPY_CLIPBOARD] =
    g_signal_new (I_("copy-clipboard"),
		  G_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkTextViewClass, copy_clipboard),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * BtkTextView::paste-clipboard:
   * @text_view: the object which received the signal
   *
   * The ::paste-clipboard signal is a 
   * <link linkend="keybinding-signals">keybinding signal</link> 
   * which gets emitted to paste the contents of the clipboard 
   * into the text view.
   * 
   * The default bindings for this signal are
   * Ctrl-v and Shift-Insert.
   */
  signals[PASTE_CLIPBOARD] =
    g_signal_new (I_("paste-clipboard"),
		  G_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkTextViewClass, paste_clipboard),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * BtkTextView::toggle-overwrite:
   * @text_view: the object which received the signal
   *
   * The ::toggle-overwrite signal is a 
   * <link linkend="keybinding-signals">keybinding signal</link> 
   * which gets emitted to toggle the overwrite mode of the text view.
   * 
   * The default bindings for this signal is Insert.
   */ 
  signals[TOGGLE_OVERWRITE] =
    g_signal_new (I_("toggle-overwrite"),
		  G_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkTextViewClass, toggle_overwrite),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * BtkTextView::set-scroll-adjustments
   * @horizontal: the horizontal #BtkAdjustment
   * @vertical: the vertical #BtkAdjustment
   *
   * Set the scroll adjustments for the text view. Usually scrolled containers
   * like #BtkScrolledWindow will emit this signal to connect two instances
   * of #BtkScrollbar to the scroll directions of the #BtkTextView.
   */
  signals[SET_SCROLL_ADJUSTMENTS] =
    g_signal_new (I_("set-scroll-adjustments"),
		  G_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkTextViewClass, set_scroll_adjustments),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT_OBJECT,
		  G_TYPE_NONE, 2,
		  BTK_TYPE_ADJUSTMENT,
		  BTK_TYPE_ADJUSTMENT);
  widget_class->set_scroll_adjustments_signal = signals[SET_SCROLL_ADJUSTMENTS];

  /**
   * BtkTextView::populate-popup:
   * @entry: The text view on which the signal is emitted
   * @menu: the menu that is being populated
   *
   * The ::populate-popup signal gets emitted before showing the 
   * context menu of the text view.
   *
   * If you need to add items to the context menu, connect
   * to this signal and append your menuitems to the @menu.
   */
  signals[POPULATE_POPUP] =
    g_signal_new (I_("populate-popup"),
		  G_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkTextViewClass, populate_popup),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  BTK_TYPE_MENU);
  
  /**
   * BtkTextView::select-all:
   * @text_view: the object which received the signal
   * @select: %TRUE to select, %FALSE to unselect
   *
   * The ::select-all signal is a 
   * <link linkend="keybinding-signals">keybinding signal</link> 
   * which gets emitted to select or unselect the complete
   * contents of the text view.
   *
   * The default bindings for this signal are Ctrl-a and Ctrl-/ 
   * for selecting and Shift-Ctrl-a and Ctrl-\ for unselecting.
   */
  signals[SELECT_ALL] =
    g_signal_new_class_handler (I_("select-all"),
                                G_OBJECT_CLASS_TYPE (object_class),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (btk_text_view_select_all),
                                NULL, NULL,
                                _btk_marshal_VOID__BOOLEAN,
                                G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

  /**
   * BtkTextView::toggle-cursor-visible:
   * @text_view: the object which received the signal
   *
   * The ::toggle-cursor-visible signal is a 
   * <link linkend="keybinding-signals">keybinding signal</link> 
   * which gets emitted to toggle the visibility of the cursor.
   *
   * The default binding for this signal is F7.
   */ 
  signals[TOGGLE_CURSOR_VISIBLE] =
    g_signal_new_class_handler (I_("toggle-cursor-visible"),
                                G_OBJECT_CLASS_TYPE (object_class),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (btk_text_view_toggle_cursor_visible),
                                NULL, NULL,
                                _btk_marshal_VOID__VOID,
                                G_TYPE_NONE, 0);

  /**
   * BtkTextView::preedit-changed:
   * @text_view: the object which received the signal
   * @preedit: the current preedit string
   *
   * If an input method is used, the typed text will not immediately
   * be committed to the buffer. So if you are interested in the text,
   * connect to this signal.
   *
   * This signal is only emitted if the text at the given position
   * is actually editable.
   *
   * Since: 2.20
   */
  signals[PREEDIT_CHANGED] =
    g_signal_new_class_handler (I_("preedit-changed"),
                                G_OBJECT_CLASS_TYPE (object_class),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                NULL,
                                NULL, NULL,
                                _btk_marshal_VOID__STRING,
                                G_TYPE_NONE, 1,
                                G_TYPE_STRING);

  /*
   * Key bindings
   */

  binding_set = btk_binding_set_by_class (klass);
  
  /* Moving the insertion point */
  add_move_binding (binding_set, BDK_Right, 0,
                    BTK_MOVEMENT_VISUAL_POSITIONS, 1);

  add_move_binding (binding_set, BDK_KP_Right, 0,
                    BTK_MOVEMENT_VISUAL_POSITIONS, 1);
  
  add_move_binding (binding_set, BDK_Left, 0,
                    BTK_MOVEMENT_VISUAL_POSITIONS, -1);

  add_move_binding (binding_set, BDK_KP_Left, 0,
                    BTK_MOVEMENT_VISUAL_POSITIONS, -1);
  
  add_move_binding (binding_set, BDK_Right, BDK_CONTROL_MASK,
                    BTK_MOVEMENT_WORDS, 1);

  add_move_binding (binding_set, BDK_KP_Right, BDK_CONTROL_MASK,
                    BTK_MOVEMENT_WORDS, 1);
  
  add_move_binding (binding_set, BDK_Left, BDK_CONTROL_MASK,
                    BTK_MOVEMENT_WORDS, -1);

  add_move_binding (binding_set, BDK_KP_Left, BDK_CONTROL_MASK,
                    BTK_MOVEMENT_WORDS, -1);
  
  add_move_binding (binding_set, BDK_Up, 0,
                    BTK_MOVEMENT_DISPLAY_LINES, -1);

  add_move_binding (binding_set, BDK_KP_Up, 0,
                    BTK_MOVEMENT_DISPLAY_LINES, -1);
  
  add_move_binding (binding_set, BDK_Down, 0,
                    BTK_MOVEMENT_DISPLAY_LINES, 1);

  add_move_binding (binding_set, BDK_KP_Down, 0,
                    BTK_MOVEMENT_DISPLAY_LINES, 1);
  
  add_move_binding (binding_set, BDK_Up, BDK_CONTROL_MASK,
                    BTK_MOVEMENT_PARAGRAPHS, -1);

  add_move_binding (binding_set, BDK_KP_Up, BDK_CONTROL_MASK,
                    BTK_MOVEMENT_PARAGRAPHS, -1);
  
  add_move_binding (binding_set, BDK_Down, BDK_CONTROL_MASK,
                    BTK_MOVEMENT_PARAGRAPHS, 1);

  add_move_binding (binding_set, BDK_KP_Down, BDK_CONTROL_MASK,
                    BTK_MOVEMENT_PARAGRAPHS, 1);
  
  add_move_binding (binding_set, BDK_Home, 0,
                    BTK_MOVEMENT_DISPLAY_LINE_ENDS, -1);

  add_move_binding (binding_set, BDK_KP_Home, 0,
                    BTK_MOVEMENT_DISPLAY_LINE_ENDS, -1);
  
  add_move_binding (binding_set, BDK_End, 0,
                    BTK_MOVEMENT_DISPLAY_LINE_ENDS, 1);

  add_move_binding (binding_set, BDK_KP_End, 0,
                    BTK_MOVEMENT_DISPLAY_LINE_ENDS, 1);
  
  add_move_binding (binding_set, BDK_Home, BDK_CONTROL_MASK,
                    BTK_MOVEMENT_BUFFER_ENDS, -1);

  add_move_binding (binding_set, BDK_KP_Home, BDK_CONTROL_MASK,
                    BTK_MOVEMENT_BUFFER_ENDS, -1);
  
  add_move_binding (binding_set, BDK_End, BDK_CONTROL_MASK,
                    BTK_MOVEMENT_BUFFER_ENDS, 1);

  add_move_binding (binding_set, BDK_KP_End, BDK_CONTROL_MASK,
                    BTK_MOVEMENT_BUFFER_ENDS, 1);
  
  add_move_binding (binding_set, BDK_Page_Up, 0,
                    BTK_MOVEMENT_PAGES, -1);

  add_move_binding (binding_set, BDK_KP_Page_Up, 0,
                    BTK_MOVEMENT_PAGES, -1);
  
  add_move_binding (binding_set, BDK_Page_Down, 0,
                    BTK_MOVEMENT_PAGES, 1);

  add_move_binding (binding_set, BDK_KP_Page_Down, 0,
                    BTK_MOVEMENT_PAGES, 1);

  add_move_binding (binding_set, BDK_Page_Up, BDK_CONTROL_MASK,
                    BTK_MOVEMENT_HORIZONTAL_PAGES, -1);

  add_move_binding (binding_set, BDK_KP_Page_Up, BDK_CONTROL_MASK,
                    BTK_MOVEMENT_HORIZONTAL_PAGES, -1);
  
  add_move_binding (binding_set, BDK_Page_Down, BDK_CONTROL_MASK,
                    BTK_MOVEMENT_HORIZONTAL_PAGES, 1);

  add_move_binding (binding_set, BDK_KP_Page_Down, BDK_CONTROL_MASK,
                    BTK_MOVEMENT_HORIZONTAL_PAGES, 1);

  /* Select all */
  btk_binding_entry_add_signal (binding_set, BDK_a, BDK_CONTROL_MASK,
				"select-all", 1,
  				G_TYPE_BOOLEAN, TRUE);

  btk_binding_entry_add_signal (binding_set, BDK_slash, BDK_CONTROL_MASK,
				"select-all", 1,
  				G_TYPE_BOOLEAN, TRUE);
  
  /* Unselect all */
  btk_binding_entry_add_signal (binding_set, BDK_backslash, BDK_CONTROL_MASK,
				 "select-all", 1,
				 G_TYPE_BOOLEAN, FALSE);

  btk_binding_entry_add_signal (binding_set, BDK_a, BDK_SHIFT_MASK | BDK_CONTROL_MASK,
				 "select-all", 1,
				 G_TYPE_BOOLEAN, FALSE);

  /* Deleting text */
  btk_binding_entry_add_signal (binding_set, BDK_Delete, 0,
				"delete-from-cursor", 2,
				G_TYPE_ENUM, BTK_DELETE_CHARS,
				G_TYPE_INT, 1);

  btk_binding_entry_add_signal (binding_set, BDK_KP_Delete, 0,
				"delete-from-cursor", 2,
				G_TYPE_ENUM, BTK_DELETE_CHARS,
				G_TYPE_INT, 1);
  
  btk_binding_entry_add_signal (binding_set, BDK_BackSpace, 0,
				"backspace", 0);

  /* Make this do the same as Backspace, to help with mis-typing */
  btk_binding_entry_add_signal (binding_set, BDK_BackSpace, BDK_SHIFT_MASK,
				"backspace", 0);

  btk_binding_entry_add_signal (binding_set, BDK_Delete, BDK_CONTROL_MASK,
				"delete-from-cursor", 2,
				G_TYPE_ENUM, BTK_DELETE_WORD_ENDS,
				G_TYPE_INT, 1);

  btk_binding_entry_add_signal (binding_set, BDK_KP_Delete, BDK_CONTROL_MASK,
				"delete-from-cursor", 2,
				G_TYPE_ENUM, BTK_DELETE_WORD_ENDS,
				G_TYPE_INT, 1);
  
  btk_binding_entry_add_signal (binding_set, BDK_BackSpace, BDK_CONTROL_MASK,
				"delete-from-cursor", 2,
				G_TYPE_ENUM, BTK_DELETE_WORD_ENDS,
				G_TYPE_INT, -1);

  btk_binding_entry_add_signal (binding_set, BDK_Delete, BDK_SHIFT_MASK | BDK_CONTROL_MASK,
				"delete-from-cursor", 2,
				G_TYPE_ENUM, BTK_DELETE_PARAGRAPH_ENDS,
				G_TYPE_INT, 1);

  btk_binding_entry_add_signal (binding_set, BDK_KP_Delete, BDK_SHIFT_MASK | BDK_CONTROL_MASK,
				"delete-from-cursor", 2,
				G_TYPE_ENUM, BTK_DELETE_PARAGRAPH_ENDS,
				G_TYPE_INT, 1);

  btk_binding_entry_add_signal (binding_set, BDK_BackSpace, BDK_SHIFT_MASK | BDK_CONTROL_MASK,
				"delete-from-cursor", 2,
				G_TYPE_ENUM, BTK_DELETE_PARAGRAPH_ENDS,
				G_TYPE_INT, -1);

  /* Cut/copy/paste */

  btk_binding_entry_add_signal (binding_set, BDK_x, BDK_CONTROL_MASK,
				"cut-clipboard", 0);
  btk_binding_entry_add_signal (binding_set, BDK_c, BDK_CONTROL_MASK,
				"copy-clipboard", 0);
  btk_binding_entry_add_signal (binding_set, BDK_v, BDK_CONTROL_MASK,
				"paste-clipboard", 0);

  btk_binding_entry_add_signal (binding_set, BDK_Delete, BDK_SHIFT_MASK,
				"cut-clipboard", 0);
  btk_binding_entry_add_signal (binding_set, BDK_Insert, BDK_CONTROL_MASK,
				"copy-clipboard", 0);
  btk_binding_entry_add_signal (binding_set, BDK_Insert, BDK_SHIFT_MASK,
				"paste-clipboard", 0);

  /* Overwrite */
  btk_binding_entry_add_signal (binding_set, BDK_Insert, 0,
				"toggle-overwrite", 0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Insert, 0,
				"toggle-overwrite", 0);

  /* Caret mode */
  btk_binding_entry_add_signal (binding_set, BDK_F7, 0,
				"toggle-cursor-visible", 0);

  /* Control-tab focus motion */
  btk_binding_entry_add_signal (binding_set, BDK_Tab, BDK_CONTROL_MASK,
				"move-focus", 1,
				BTK_TYPE_DIRECTION_TYPE, BTK_DIR_TAB_FORWARD);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Tab, BDK_CONTROL_MASK,
				"move-focus", 1,
				BTK_TYPE_DIRECTION_TYPE, BTK_DIR_TAB_FORWARD);
  
  btk_binding_entry_add_signal (binding_set, BDK_Tab, BDK_SHIFT_MASK | BDK_CONTROL_MASK,
				"move-focus", 1,
				BTK_TYPE_DIRECTION_TYPE, BTK_DIR_TAB_BACKWARD);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Tab, BDK_SHIFT_MASK | BDK_CONTROL_MASK,
				"move-focus", 1,
				BTK_TYPE_DIRECTION_TYPE, BTK_DIR_TAB_BACKWARD);

  g_type_class_add_private (bobject_class, sizeof (BtkTextViewPrivate));
}

static void
btk_text_view_init (BtkTextView *text_view)
{
  BtkWidget *widget = BTK_WIDGET (text_view);
  BtkTargetList *target_list;
  BtkTextViewPrivate *priv;

  priv = BTK_TEXT_VIEW_GET_PRIVATE (text_view);

  btk_widget_set_can_focus (widget, TRUE);

  /* Set up default style */
  text_view->wrap_mode = BTK_WRAP_NONE;
  text_view->pixels_above_lines = 0;
  text_view->pixels_below_lines = 0;
  text_view->pixels_inside_wrap = 0;
  text_view->justify = BTK_JUSTIFY_LEFT;
  text_view->left_margin = 0;
  text_view->right_margin = 0;
  text_view->indent = 0;
  text_view->tabs = NULL;
  text_view->editable = TRUE;

  priv->scroll_after_paste = TRUE;

  btk_drag_dest_set (widget, 0, NULL, 0,
                     BDK_ACTION_COPY | BDK_ACTION_MOVE);

  target_list = btk_target_list_new (NULL, 0);
  btk_drag_dest_set_target_list (widget, target_list);
  btk_target_list_unref (target_list);

  text_view->virtual_cursor_x = -1;
  text_view->virtual_cursor_y = -1;

  /* This object is completely private. No external entity can gain a reference
   * to it; so we create it here and destroy it in finalize ().
   */
  text_view->im_context = btk_im_multicontext_new ();

  g_signal_connect (text_view->im_context, "commit",
                    G_CALLBACK (btk_text_view_commit_handler), text_view);
  g_signal_connect (text_view->im_context, "preedit-changed",
 		    G_CALLBACK (btk_text_view_preedit_changed_handler), text_view);
  g_signal_connect (text_view->im_context, "retrieve-surrounding",
 		    G_CALLBACK (btk_text_view_retrieve_surrounding_handler), text_view);
  g_signal_connect (text_view->im_context, "delete-surrounding",
 		    G_CALLBACK (btk_text_view_delete_surrounding_handler), text_view);

  text_view->cursor_visible = TRUE;

  text_view->accepts_tab = TRUE;

  text_view->text_window = text_window_new (BTK_TEXT_WINDOW_TEXT,
                                            widget, 200, 200);

  text_view->drag_start_x = -1;
  text_view->drag_start_y = -1;

  text_view->pending_place_cursor_button = 0;

  /* We handle all our own redrawing */
  btk_widget_set_redraw_on_allocate (widget, FALSE);
}

/**
 * btk_text_view_new:
 *
 * Creates a new #BtkTextView. If you don't call btk_text_view_set_buffer()
 * before using the text view, an empty default buffer will be created
 * for you. Get the buffer with btk_text_view_get_buffer(). If you want
 * to specify your own buffer, consider btk_text_view_new_with_buffer().
 *
 * Return value: a new #BtkTextView
 **/
BtkWidget*
btk_text_view_new (void)
{
  return g_object_new (BTK_TYPE_TEXT_VIEW, NULL);
}

/**
 * btk_text_view_new_with_buffer:
 * @buffer: a #BtkTextBuffer
 *
 * Creates a new #BtkTextView widget displaying the buffer
 * @buffer. One buffer can be shared among many widgets.
 * @buffer may be %NULL to create a default buffer, in which case
 * this function is equivalent to btk_text_view_new(). The
 * text view adds its own reference count to the buffer; it does not
 * take over an existing reference.
 *
 * Return value: a new #BtkTextView.
 **/
BtkWidget*
btk_text_view_new_with_buffer (BtkTextBuffer *buffer)
{
  BtkTextView *text_view;

  text_view = (BtkTextView*)btk_text_view_new ();

  btk_text_view_set_buffer (text_view, buffer);

  return BTK_WIDGET (text_view);
}

/**
 * btk_text_view_set_buffer:
 * @text_view: a #BtkTextView
 * @buffer: (allow-none): a #BtkTextBuffer
 *
 * Sets @buffer as the buffer being displayed by @text_view. The previous
 * buffer displayed by the text view is unreferenced, and a reference is
 * added to @buffer. If you owned a reference to @buffer before passing it
 * to this function, you must remove that reference yourself; #BtkTextView
 * will not "adopt" it.
 **/
void
btk_text_view_set_buffer (BtkTextView   *text_view,
                          BtkTextBuffer *buffer)
{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));
  g_return_if_fail (buffer == NULL || BTK_IS_TEXT_BUFFER (buffer));

  if (text_view->buffer == buffer)
    return;

  if (text_view->buffer != NULL)
    {
      /* Destroy all anchored children */
      GSList *tmp_list;
      GSList *copy;

      copy = g_slist_copy (text_view->children);
      tmp_list = copy;
      while (tmp_list != NULL)
        {
          BtkTextViewChild *vc = tmp_list->data;

          if (vc->anchor)
            {
              btk_widget_destroy (vc->widget);
              /* vc may now be invalid! */
            }

          tmp_list = g_slist_next (tmp_list);
        }

      g_slist_free (copy);

      g_signal_handlers_disconnect_by_func (text_view->buffer,
					    btk_text_view_mark_set_handler,
					    text_view);
      g_signal_handlers_disconnect_by_func (text_view->buffer,
                                            btk_text_view_target_list_notify,
                                            text_view);
      g_signal_handlers_disconnect_by_func (text_view->buffer,
                                            btk_text_view_paste_done_handler,
                                            text_view);

      if (btk_widget_get_realized (BTK_WIDGET (text_view)))
	{
	  BtkClipboard *clipboard = btk_widget_get_clipboard (BTK_WIDGET (text_view),
							      BDK_SELECTION_PRIMARY);
	  btk_text_buffer_remove_selection_clipboard (text_view->buffer, clipboard);
        }

      if (text_view->layout)
        btk_text_layout_set_buffer (text_view->layout, NULL);

      g_object_unref (text_view->buffer);
      text_view->dnd_mark = NULL;
      text_view->first_para_mark = NULL;
      cancel_pending_scroll (text_view);
    }

  text_view->buffer = buffer;

  if (text_view->layout)
    btk_text_layout_set_buffer (text_view->layout, buffer);

  if (buffer != NULL)
    {
      BtkTextIter start;

      g_object_ref (buffer);

      btk_text_buffer_get_iter_at_offset (text_view->buffer, &start, 0);

      text_view->dnd_mark = btk_text_buffer_create_mark (text_view->buffer,
                                                         "btk_drag_target",
                                                         &start, FALSE);

      text_view->first_para_mark = btk_text_buffer_create_mark (text_view->buffer,
                                                                NULL,
                                                                &start, TRUE);

      text_view->first_para_pixels = 0;

      g_signal_connect (text_view->buffer, "mark-set",
			G_CALLBACK (btk_text_view_mark_set_handler),
                        text_view);
      g_signal_connect (text_view->buffer, "notify::paste-target-list",
			G_CALLBACK (btk_text_view_target_list_notify),
                        text_view);
      g_signal_connect (text_view->buffer, "paste-done",
			G_CALLBACK (btk_text_view_paste_done_handler),
                        text_view);

      btk_text_view_target_list_notify (text_view->buffer, NULL, text_view);

      if (btk_widget_get_realized (BTK_WIDGET (text_view)))
	{
	  BtkClipboard *clipboard = btk_widget_get_clipboard (BTK_WIDGET (text_view),
							      BDK_SELECTION_PRIMARY);
	  btk_text_buffer_add_selection_clipboard (text_view->buffer, clipboard);
	}
    }

  g_object_notify (G_OBJECT (text_view), "buffer");
  
  if (btk_widget_get_visible (BTK_WIDGET (text_view)))
    btk_widget_queue_draw (BTK_WIDGET (text_view));

  DV(g_print ("Invalidating due to set_buffer\n"));
  btk_text_view_invalidate (text_view);
}

static BtkTextBuffer*
get_buffer (BtkTextView *text_view)
{
  if (text_view->buffer == NULL)
    {
      BtkTextBuffer *b;
      b = btk_text_buffer_new (NULL);
      btk_text_view_set_buffer (text_view, b);
      g_object_unref (b);
    }

  return text_view->buffer;
}

/**
 * btk_text_view_get_buffer:
 * @text_view: a #BtkTextView
 *
 * Returns the #BtkTextBuffer being displayed by this text view.
 * The reference count on the buffer is not incremented; the caller
 * of this function won't own a new reference.
 *
 * Return value: (transfer none): a #BtkTextBuffer
 **/
BtkTextBuffer*
btk_text_view_get_buffer (BtkTextView *text_view)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), NULL);

  return get_buffer (text_view);
}

/**
 * btk_text_view_get_iter_at_location:
 * @text_view: a #BtkTextView
 * @iter: (out): a #BtkTextIter
 * @x: x position, in buffer coordinates
 * @y: y position, in buffer coordinates
 *
 * Retrieves the iterator at buffer coordinates @x and @y. Buffer
 * coordinates are coordinates for the entire buffer, not just the
 * currently-displayed portion.  If you have coordinates from an
 * event, you have to convert those to buffer coordinates with
 * btk_text_view_window_to_buffer_coords().
 **/
void
btk_text_view_get_iter_at_location (BtkTextView *text_view,
                                    BtkTextIter *iter,
                                    gint         x,
                                    gint         y)
{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));
  g_return_if_fail (iter != NULL);

  btk_text_view_ensure_layout (text_view);
  
  btk_text_layout_get_iter_at_pixel (text_view->layout,
                                     iter, x, y);
}

/**
 * btk_text_view_get_iter_at_position:
 * @text_view: a #BtkTextView
 * @iter: (out): a #BtkTextIter
 * @trailing: (out) (allow-none): if non-%NULL, location to store an integer indicating where
 *    in the grapheme the user clicked. It will either be
 *    zero, or the number of characters in the grapheme. 
 *    0 represents the trailing edge of the grapheme.
 * @x: x position, in buffer coordinates
 * @y: y position, in buffer coordinates
 *
 * Retrieves the iterator pointing to the character at buffer 
 * coordinates @x and @y. Buffer coordinates are coordinates for 
 * the entire buffer, not just the currently-displayed portion.  
 * If you have coordinates from an event, you have to convert 
 * those to buffer coordinates with 
 * btk_text_view_window_to_buffer_coords().
 *
 * Note that this is different from btk_text_view_get_iter_at_location(),
 * which returns cursor locations, i.e. positions <emphasis>between</emphasis>
 * characters.
 *
 * Since: 2.6
 **/
void
btk_text_view_get_iter_at_position (BtkTextView *text_view,
				    BtkTextIter *iter,
				    gint        *trailing,
				    gint         x,
				    gint         y)
{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));
  g_return_if_fail (iter != NULL);

  btk_text_view_ensure_layout (text_view);
  
  btk_text_layout_get_iter_at_position (text_view->layout,
					iter, trailing, x, y);
}

/**
 * btk_text_view_get_iter_location:
 * @text_view: a #BtkTextView
 * @iter: a #BtkTextIter
 * @location: (out): bounds of the character at @iter
 *
 * Gets a rectangle which roughly contains the character at @iter.
 * The rectangle position is in buffer coordinates; use
 * btk_text_view_buffer_to_window_coords() to convert these
 * coordinates to coordinates for one of the windows in the text view.
 **/
void
btk_text_view_get_iter_location (BtkTextView       *text_view,
                                 const BtkTextIter *iter,
                                 BdkRectangle      *location)
{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));
  g_return_if_fail (btk_text_iter_get_buffer (iter) == get_buffer (text_view));

  btk_text_view_ensure_layout (text_view);
  
  btk_text_layout_get_iter_location (text_view->layout, iter, location);
}

/**
 * btk_text_view_get_line_yrange:
 * @text_view: a #BtkTextView
 * @iter: a #BtkTextIter
 * @y: (out): return location for a y coordinate
 * @height: (out): return location for a height
 *
 * Gets the y coordinate of the top of the line containing @iter,
 * and the height of the line. The coordinate is a buffer coordinate;
 * convert to window coordinates with btk_text_view_buffer_to_window_coords().
 **/
void
btk_text_view_get_line_yrange (BtkTextView       *text_view,
                               const BtkTextIter *iter,
                               gint              *y,
                               gint              *height)
{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));
  g_return_if_fail (btk_text_iter_get_buffer (iter) == get_buffer (text_view));

  btk_text_view_ensure_layout (text_view);
  
  btk_text_layout_get_line_yrange (text_view->layout,
                                   iter,
                                   y,
                                   height);
}

/**
 * btk_text_view_get_line_at_y:
 * @text_view: a #BtkTextView
 * @target_iter: (out): a #BtkTextIter
 * @y: a y coordinate
 * @line_top: (out): return location for top coordinate of the line
 *
 * Gets the #BtkTextIter at the start of the line containing
 * the coordinate @y. @y is in buffer coordinates, convert from
 * window coordinates with btk_text_view_window_to_buffer_coords().
 * If non-%NULL, @line_top will be filled with the coordinate of the top
 * edge of the line.
 **/
void
btk_text_view_get_line_at_y (BtkTextView *text_view,
                             BtkTextIter *target_iter,
                             gint         y,
                             gint        *line_top)
{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));

  btk_text_view_ensure_layout (text_view);
  
  btk_text_layout_get_line_at_y (text_view->layout,
                                 target_iter,
                                 y,
                                 line_top);
}

static gboolean
set_adjustment_clamped (BtkAdjustment *adj, gdouble val)
{
  DV (g_print ("  Setting adj to raw value %g\n", val));
  
  /* We don't really want to clamp to upper; we want to clamp to
     upper - page_size which is the highest value the scrollbar
     will let us reach. */
  if (val > (adj->upper - adj->page_size))
    val = adj->upper - adj->page_size;

  if (val < adj->lower)
    val = adj->lower;

  if (val != adj->value)
    {
      DV (g_print ("  Setting adj to clamped value %g\n", val));
      btk_adjustment_set_value (adj, val);
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * btk_text_view_scroll_to_iter:
 * @text_view: a #BtkTextView
 * @iter: a #BtkTextIter
 * @within_margin: margin as a [0.0,0.5) fraction of screen size
 * @use_align: whether to use alignment arguments (if %FALSE, 
 *    just get the mark onscreen)
 * @xalign: horizontal alignment of mark within visible area
 * @yalign: vertical alignment of mark within visible area
 *
 * Scrolls @text_view so that @iter is on the screen in the position
 * indicated by @xalign and @yalign. An alignment of 0.0 indicates
 * left or top, 1.0 indicates right or bottom, 0.5 means center. 
 * If @use_align is %FALSE, the text scrolls the minimal distance to 
 * get the mark onscreen, possibly not scrolling at all. The effective 
 * screen for purposes of this function is reduced by a margin of size 
 * @within_margin.
 *
 * Note that this function uses the currently-computed height of the
 * lines in the text buffer. Line heights are computed in an idle 
 * handler; so this function may not have the desired effect if it's 
 * called before the height computations. To avoid oddness, consider 
 * using btk_text_view_scroll_to_mark() which saves a point to be 
 * scrolled to after line validation.
 *
 * Return value: %TRUE if scrolling occurred
 **/
gboolean
btk_text_view_scroll_to_iter (BtkTextView   *text_view,
                              BtkTextIter   *iter,
                              gdouble        within_margin,
                              gboolean       use_align,
                              gdouble        xalign,
                              gdouble        yalign)
{
  BdkRectangle rect;
  BdkRectangle screen;
  gint screen_bottom;
  gint screen_right;
  gint scroll_dest;
  BtkWidget *widget;
  gboolean retval = FALSE;
  gint scroll_inc;
  gint screen_xoffset, screen_yoffset;
  gint current_x_scroll, current_y_scroll;

  /* FIXME why don't we do the validate-at-scroll-destination thing
   * from flush_scroll in this function? I think it wasn't done before
   * because changed_handler was screwed up, but I could be wrong.
   */
  
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (within_margin >= 0.0 && within_margin < 0.5, FALSE);
  g_return_val_if_fail (xalign >= 0.0 && xalign <= 1.0, FALSE);
  g_return_val_if_fail (yalign >= 0.0 && yalign <= 1.0, FALSE);
  
  widget = BTK_WIDGET (text_view);

  DV(g_print(B_STRLOC"\n"));
  
  btk_text_layout_get_iter_location (text_view->layout,
                                     iter,
                                     &rect);

  DV (g_print (" target rect %d,%d  %d x %d\n", rect.x, rect.y, rect.width, rect.height));
  
  current_x_scroll = text_view->xoffset;
  current_y_scroll = text_view->yoffset;

  screen.x = current_x_scroll;
  screen.y = current_y_scroll;
  screen.width = SCREEN_WIDTH (widget);
  screen.height = SCREEN_HEIGHT (widget);
  
  screen_xoffset = screen.width * within_margin;
  screen_yoffset = screen.height * within_margin;
  
  screen.x += screen_xoffset;
  screen.y += screen_yoffset;
  screen.width -= screen_xoffset * 2;
  screen.height -= screen_yoffset * 2;

  /* paranoia check */
  if (screen.width < 1)
    screen.width = 1;
  if (screen.height < 1)
    screen.height = 1;
  
  /* The -1 here ensures that we leave enough space to draw the cursor
   * when this function is used for horizontal scrolling. 
   */
  screen_right = screen.x + screen.width - 1;
  screen_bottom = screen.y + screen.height;
  
  /* The alignment affects the point in the target character that we
   * choose to align. If we're doing right/bottom alignment, we align
   * the right/bottom edge of the character the mark is at; if we're
   * doing left/top we align the left/top edge of the character; if
   * we're doing center alignment we align the center of the
   * character.
   */
  
  /* Vertical scroll */

  scroll_inc = 0;
  scroll_dest = current_y_scroll;
  
  if (use_align)
    {      
      scroll_dest = rect.y + (rect.height * yalign) - (screen.height * yalign);
      
      /* if scroll_dest < screen.y, we move a negative increment (up),
       * else a positive increment (down)
       */
      scroll_inc = scroll_dest - screen.y + screen_yoffset;
    }
  else
    {
      /* move minimum to get onscreen */
      if (rect.y < screen.y)
        {
          scroll_dest = rect.y;
          scroll_inc = scroll_dest - screen.y - screen_yoffset;
        }
      else if ((rect.y + rect.height) > screen_bottom)
        {
          scroll_dest = rect.y + rect.height;
          scroll_inc = scroll_dest - screen_bottom + screen_yoffset;
        }
    }  
  
  if (scroll_inc != 0)
    {
      retval = set_adjustment_clamped (get_vadjustment (text_view),
                                       current_y_scroll + scroll_inc);

      DV (g_print (" vert increment %d\n", scroll_inc));
    }

  /* Horizontal scroll */
  
  scroll_inc = 0;
  scroll_dest = current_x_scroll;
  
  if (use_align)
    {      
      scroll_dest = rect.x + (rect.width * xalign) - (screen.width * xalign);

      /* if scroll_dest < screen.y, we move a negative increment (left),
       * else a positive increment (right)
       */
      scroll_inc = scroll_dest - screen.x + screen_xoffset;
    }
  else
    {
      /* move minimum to get onscreen */
      if (rect.x < screen.x)
        {
          scroll_dest = rect.x;
          scroll_inc = scroll_dest - screen.x - screen_xoffset;
        }
      else if ((rect.x + rect.width) > screen_right)
        {
          scroll_dest = rect.x + rect.width;
          scroll_inc = scroll_dest - screen_right + screen_xoffset;
        }
    }
  
  if (scroll_inc != 0)
    {
      retval = set_adjustment_clamped (get_hadjustment (text_view),
                                       current_x_scroll + scroll_inc);

      DV (g_print (" horiz increment %d\n", scroll_inc));
    }
  
  if (retval)
    {
      DV(g_print (">Actually scrolled ("B_STRLOC")\n"));
    }
  else
    {
      DV(g_print (">Didn't end up scrolling ("B_STRLOC")\n"));
    }
  
  return retval;
}

static void
free_pending_scroll (BtkTextPendingScroll *scroll)
{
  if (!btk_text_mark_get_deleted (scroll->mark))
    btk_text_buffer_delete_mark (btk_text_mark_get_buffer (scroll->mark),
                                 scroll->mark);
  g_object_unref (scroll->mark);
  g_free (scroll);
}

static void
cancel_pending_scroll (BtkTextView *text_view)
{
  if (text_view->pending_scroll)
    {
      free_pending_scroll (text_view->pending_scroll);
      text_view->pending_scroll = NULL;
    }
}
    
static void
btk_text_view_queue_scroll (BtkTextView   *text_view,
                            BtkTextMark   *mark,
                            gdouble        within_margin,
                            gboolean       use_align,
                            gdouble        xalign,
                            gdouble        yalign)
{
  BtkTextIter iter;
  BtkTextPendingScroll *scroll;

  DV(g_print(B_STRLOC"\n"));
  
  scroll = g_new (BtkTextPendingScroll, 1);

  scroll->within_margin = within_margin;
  scroll->use_align = use_align;
  scroll->xalign = xalign;
  scroll->yalign = yalign;
  
  btk_text_buffer_get_iter_at_mark (get_buffer (text_view), &iter, mark);

  scroll->mark = btk_text_buffer_create_mark (get_buffer (text_view),
                                              NULL,
                                              &iter,
                                              btk_text_mark_get_left_gravity (mark));

  g_object_ref (scroll->mark);
  
  cancel_pending_scroll (text_view);

  text_view->pending_scroll = scroll;
}

static gboolean
btk_text_view_flush_scroll (BtkTextView *text_view)
{
  BtkTextIter iter;
  BtkTextPendingScroll *scroll;
  gboolean retval;
  BtkWidget *widget;

  widget = BTK_WIDGET (text_view);
  
  DV(g_print(B_STRLOC"\n"));
  
  if (text_view->pending_scroll == NULL)
    {
      DV (g_print ("in flush scroll, no pending scroll\n"));
      return FALSE;
    }

  scroll = text_view->pending_scroll;

  /* avoid recursion */
  text_view->pending_scroll = NULL;
  
  btk_text_buffer_get_iter_at_mark (get_buffer (text_view), &iter, scroll->mark);

  /* Validate area around the scroll destination, so the adjustment
   * can meaningfully point into that area. We must validate
   * enough area to be sure that after we scroll, everything onscreen
   * is valid; otherwise, validation will maintain the first para
   * in one place, but may push the target iter off the bottom of
   * the screen.
   */
  DV(g_print (">Validating scroll destination ("B_STRLOC")\n"));
  btk_text_layout_validate_yrange (text_view->layout, &iter,
                                   - (widget->allocation.height * 2),
                                   widget->allocation.height * 2);
  
  DV(g_print (">Done validating scroll destination ("B_STRLOC")\n"));

  /* Ensure we have updated width/height */
  btk_text_view_update_adjustments (text_view);
  
  retval = btk_text_view_scroll_to_iter (text_view,
                                         &iter,
                                         scroll->within_margin,
                                         scroll->use_align,
                                         scroll->xalign,
                                         scroll->yalign);

  free_pending_scroll (scroll);

  return retval;
}

static void
btk_text_view_set_adjustment_upper (BtkAdjustment *adj, gdouble upper)
{  
  if (upper != adj->upper)
    {
      gdouble min = MAX (0.0, upper - adj->page_size);
      gboolean value_changed = FALSE;

      adj->upper = upper;

      if (adj->value > min)
        {
          adj->value = min;
          value_changed = TRUE;
        }

      btk_adjustment_changed (adj);
      DV(g_print(">Changed adj upper to %g ("B_STRLOC")\n", upper));
      
      if (value_changed)
        {
          DV(g_print(">Changed adj value because upper decreased ("B_STRLOC")\n"));
	  btk_adjustment_value_changed (adj);
        }
    }
}

static void
btk_text_view_update_adjustments (BtkTextView *text_view)
{
  gint width = 0, height = 0;

  DV(g_print(">Updating adjustments ("B_STRLOC")\n"));

  if (text_view->layout)
    btk_text_layout_get_size (text_view->layout, &width, &height);

  /* Make room for the cursor after the last character in the widest line */
  width += SPACE_FOR_CURSOR;

  if (text_view->width != width || text_view->height != height)
    {
      if (text_view->width != width)
	text_view->width_changed = TRUE;

      text_view->width = width;
      text_view->height = height;

      btk_text_view_set_adjustment_upper (get_hadjustment (text_view),
                                          MAX (SCREEN_WIDTH (text_view), width));
      btk_text_view_set_adjustment_upper (get_vadjustment (text_view),
                                          MAX (SCREEN_HEIGHT (text_view), height));
      
      /* hadj/vadj exist since we called get_hadjustment/get_vadjustment above */

      /* Set up the step sizes; we'll say that a page is
         our allocation minus one step, and a step is
         1/10 of our allocation. */
      text_view->hadjustment->step_increment =
        SCREEN_WIDTH (text_view) / 10.0;
      text_view->hadjustment->page_increment =
        SCREEN_WIDTH (text_view) * 0.9;
      
      text_view->vadjustment->step_increment =
        SCREEN_HEIGHT (text_view) / 10.0;
      text_view->vadjustment->page_increment =
        SCREEN_HEIGHT (text_view) * 0.9;

      btk_adjustment_changed (get_hadjustment (text_view));
      btk_adjustment_changed (get_vadjustment (text_view));
    }
}

static void
btk_text_view_update_layout_width (BtkTextView *text_view)
{
  DV(g_print(">Updating layout width ("B_STRLOC")\n"));
  
  btk_text_view_ensure_layout (text_view);

  btk_text_layout_set_screen_width (text_view->layout,
                                    MAX (1, SCREEN_WIDTH (text_view) - SPACE_FOR_CURSOR));
}

static void
btk_text_view_update_im_spot_location (BtkTextView *text_view)
{
  BdkRectangle area;

  if (text_view->layout == NULL)
    return;
  
  btk_text_view_get_cursor_location (text_view, &area);

  area.x -= text_view->xoffset;
  area.y -= text_view->yoffset;
    
  /* Width returned by Bango indicates direction of cursor,
   * by it's sign more than the size of cursor.
   */
  area.width = 0;

  btk_im_context_set_cursor_location (text_view->im_context, &area);
}

static gboolean
do_update_im_spot_location (gpointer text_view)
{
  BtkTextViewPrivate *priv;

  priv = BTK_TEXT_VIEW_GET_PRIVATE (text_view);
  priv->im_spot_idle = 0;

  btk_text_view_update_im_spot_location (text_view);
  return FALSE;
}

static void
queue_update_im_spot_location (BtkTextView *text_view)
{
  BtkTextViewPrivate *priv;

  priv = BTK_TEXT_VIEW_GET_PRIVATE (text_view);

  /* Use priority a little higher than BTK_TEXT_VIEW_PRIORITY_VALIDATE,
   * so we don't wait until the entire buffer has been validated. */
  if (!priv->im_spot_idle)
    priv->im_spot_idle = bdk_threads_add_idle_full (BTK_TEXT_VIEW_PRIORITY_VALIDATE - 1,
						    do_update_im_spot_location,
						    text_view,
						    NULL);
}

static void
flush_update_im_spot_location (BtkTextView *text_view)
{
  BtkTextViewPrivate *priv;

  priv = BTK_TEXT_VIEW_GET_PRIVATE (text_view);

  if (priv->im_spot_idle)
    {
      g_source_remove (priv->im_spot_idle);
      priv->im_spot_idle = 0;
      btk_text_view_update_im_spot_location (text_view);
    }
}

/**
 * btk_text_view_scroll_to_mark:
 * @text_view: a #BtkTextView
 * @mark: a #BtkTextMark
 * @within_margin: margin as a [0.0,0.5) fraction of screen size
 * @use_align: whether to use alignment arguments (if %FALSE, just 
 *    get the mark onscreen)
 * @xalign: horizontal alignment of mark within visible area
 * @yalign: vertical alignment of mark within visible area
 *
 * Scrolls @text_view so that @mark is on the screen in the position
 * indicated by @xalign and @yalign. An alignment of 0.0 indicates
 * left or top, 1.0 indicates right or bottom, 0.5 means center. 
 * If @use_align is %FALSE, the text scrolls the minimal distance to 
 * get the mark onscreen, possibly not scrolling at all. The effective 
 * screen for purposes of this function is reduced by a margin of size 
 * @within_margin.
 **/
void
btk_text_view_scroll_to_mark (BtkTextView *text_view,
                              BtkTextMark *mark,
                              gdouble      within_margin,
                              gboolean     use_align,
                              gdouble      xalign,
                              gdouble      yalign)
{  
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));
  g_return_if_fail (BTK_IS_TEXT_MARK (mark));
  g_return_if_fail (within_margin >= 0.0 && within_margin < 0.5);
  g_return_if_fail (xalign >= 0.0 && xalign <= 1.0);
  g_return_if_fail (yalign >= 0.0 && yalign <= 1.0);

  /* We need to verify that the buffer contains the mark, otherwise this
   * can lead to data structure corruption later on.
   */
  g_return_if_fail (get_buffer (text_view) == btk_text_mark_get_buffer (mark));

  btk_text_view_queue_scroll (text_view, mark,
                              within_margin,
                              use_align,
                              xalign,
                              yalign);

  /* If no validation is pending, we need to go ahead and force an
   * immediate scroll.
   */
  if (text_view->layout &&
      btk_text_layout_is_valid (text_view->layout))
    btk_text_view_flush_scroll (text_view);
}

/**
 * btk_text_view_scroll_mark_onscreen:
 * @text_view: a #BtkTextView
 * @mark: a mark in the buffer for @text_view
 * 
 * Scrolls @text_view the minimum distance such that @mark is contained
 * within the visible area of the widget.
 **/
void
btk_text_view_scroll_mark_onscreen (BtkTextView *text_view,
                                    BtkTextMark *mark)
{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));
  g_return_if_fail (BTK_IS_TEXT_MARK (mark));

  /* We need to verify that the buffer contains the mark, otherwise this
   * can lead to data structure corruption later on.
   */
  g_return_if_fail (get_buffer (text_view) == btk_text_mark_get_buffer (mark));

  btk_text_view_scroll_to_mark (text_view, mark, 0.0, FALSE, 0.0, 0.0);
}

static gboolean
clamp_iter_onscreen (BtkTextView *text_view, BtkTextIter *iter)
{
  BdkRectangle visible_rect;
  btk_text_view_get_visible_rect (text_view, &visible_rect);

  return btk_text_layout_clamp_iter_to_vrange (text_view->layout, iter,
                                               visible_rect.y,
                                               visible_rect.y + visible_rect.height);
}

/**
 * btk_text_view_move_mark_onscreen:
 * @text_view: a #BtkTextView
 * @mark: a #BtkTextMark
 *
 * Moves a mark within the buffer so that it's
 * located within the currently-visible text area.
 *
 * Return value: %TRUE if the mark moved (wasn't already onscreen)
 **/
gboolean
btk_text_view_move_mark_onscreen (BtkTextView *text_view,
                                  BtkTextMark *mark)
{
  BtkTextIter iter;

  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), FALSE);
  g_return_val_if_fail (mark != NULL, FALSE);

  btk_text_buffer_get_iter_at_mark (get_buffer (text_view), &iter, mark);

  if (clamp_iter_onscreen (text_view, &iter))
    {
      btk_text_buffer_move_mark (get_buffer (text_view), mark, &iter);
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * btk_text_view_get_visible_rect:
 * @text_view: a #BtkTextView
 * @visible_rect: (out): rectangle to fill
 *
 * Fills @visible_rect with the currently-visible
 * rebunnyion of the buffer, in buffer coordinates. Convert to window coordinates
 * with btk_text_view_buffer_to_window_coords().
 **/
void
btk_text_view_get_visible_rect (BtkTextView  *text_view,
                                BdkRectangle *visible_rect)
{
  BtkWidget *widget;

  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));

  widget = BTK_WIDGET (text_view);

  if (visible_rect)
    {
      visible_rect->x = text_view->xoffset;
      visible_rect->y = text_view->yoffset;
      visible_rect->width = SCREEN_WIDTH (widget);
      visible_rect->height = SCREEN_HEIGHT (widget);

      DV(g_print(" visible rect: %d,%d %d x %d\n",
                 visible_rect->x,
                 visible_rect->y,
                 visible_rect->width,
                 visible_rect->height));
    }
}

/**
 * btk_text_view_set_wrap_mode:
 * @text_view: a #BtkTextView
 * @wrap_mode: a #BtkWrapMode
 *
 * Sets the line wrapping for the view.
 **/
void
btk_text_view_set_wrap_mode (BtkTextView *text_view,
                             BtkWrapMode  wrap_mode)
{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));

  if (text_view->wrap_mode != wrap_mode)
    {
      text_view->wrap_mode = wrap_mode;

      if (text_view->layout)
        {
          text_view->layout->default_style->wrap_mode = wrap_mode;
          btk_text_layout_default_style_changed (text_view->layout);
        }
    }

  g_object_notify (G_OBJECT (text_view), "wrap-mode");
}

/**
 * btk_text_view_get_wrap_mode:
 * @text_view: a #BtkTextView
 *
 * Gets the line wrapping for the view.
 *
 * Return value: the line wrap setting
 **/
BtkWrapMode
btk_text_view_get_wrap_mode (BtkTextView *text_view)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), BTK_WRAP_NONE);

  return text_view->wrap_mode;
}

/**
 * btk_text_view_set_editable:
 * @text_view: a #BtkTextView
 * @setting: whether it's editable
 *
 * Sets the default editability of the #BtkTextView. You can override
 * this default setting with tags in the buffer, using the "editable"
 * attribute of tags.
 **/
void
btk_text_view_set_editable (BtkTextView *text_view,
                            gboolean     setting)
{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));
  setting = setting != FALSE;

  if (text_view->editable != setting)
    {
      if (!setting)
	{
	  btk_text_view_reset_im_context(text_view);
	  if (btk_widget_has_focus (BTK_WIDGET (text_view)))
	    btk_im_context_focus_out (text_view->im_context);
	}

      text_view->editable = setting;

      if (setting && btk_widget_has_focus (BTK_WIDGET (text_view)))
	btk_im_context_focus_in (text_view->im_context);

      if (text_view->layout)
        {
	  btk_text_layout_set_overwrite_mode (text_view->layout,
					      text_view->overwrite_mode && text_view->editable);
          text_view->layout->default_style->editable = text_view->editable;
          btk_text_layout_default_style_changed (text_view->layout);
        }

      g_object_notify (G_OBJECT (text_view), "editable");
    }
}

/**
 * btk_text_view_get_editable:
 * @text_view: a #BtkTextView
 *
 * Returns the default editability of the #BtkTextView. Tags in the
 * buffer may override this setting for some ranges of text.
 *
 * Return value: whether text is editable by default
 **/
gboolean
btk_text_view_get_editable (BtkTextView *text_view)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), FALSE);

  return text_view->editable;
}

/**
 * btk_text_view_set_pixels_above_lines:
 * @text_view: a #BtkTextView
 * @pixels_above_lines: pixels above paragraphs
 * 
 * Sets the default number of blank pixels above paragraphs in @text_view.
 * Tags in the buffer for @text_view may override the defaults.
 **/
void
btk_text_view_set_pixels_above_lines (BtkTextView *text_view,
                                      gint         pixels_above_lines)
{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));

  if (text_view->pixels_above_lines != pixels_above_lines)
    {
      text_view->pixels_above_lines = pixels_above_lines;

      if (text_view->layout)
        {
          text_view->layout->default_style->pixels_above_lines = pixels_above_lines;
          btk_text_layout_default_style_changed (text_view->layout);
        }

      g_object_notify (G_OBJECT (text_view), "pixels-above-lines");
    }
}

/**
 * btk_text_view_get_pixels_above_lines:
 * @text_view: a #BtkTextView
 * 
 * Gets the default number of pixels to put above paragraphs.
 * 
 * Return value: default number of pixels above paragraphs
 **/
gint
btk_text_view_get_pixels_above_lines (BtkTextView *text_view)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), 0);

  return text_view->pixels_above_lines;
}

/**
 * btk_text_view_set_pixels_below_lines:
 * @text_view: a #BtkTextView
 * @pixels_below_lines: pixels below paragraphs 
 *
 * Sets the default number of pixels of blank space
 * to put below paragraphs in @text_view. May be overridden
 * by tags applied to @text_view's buffer. 
 **/
void
btk_text_view_set_pixels_below_lines (BtkTextView *text_view,
                                      gint         pixels_below_lines)
{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));

  if (text_view->pixels_below_lines != pixels_below_lines)
    {
      text_view->pixels_below_lines = pixels_below_lines;

      if (text_view->layout)
        {
          text_view->layout->default_style->pixels_below_lines = pixels_below_lines;
          btk_text_layout_default_style_changed (text_view->layout);
        }

      g_object_notify (G_OBJECT (text_view), "pixels-below-lines");
    }
}

/**
 * btk_text_view_get_pixels_below_lines:
 * @text_view: a #BtkTextView
 * 
 * Gets the value set by btk_text_view_set_pixels_below_lines().
 * 
 * Return value: default number of blank pixels below paragraphs
 **/
gint
btk_text_view_get_pixels_below_lines (BtkTextView *text_view)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), 0);

  return text_view->pixels_below_lines;
}

/**
 * btk_text_view_set_pixels_inside_wrap:
 * @text_view: a #BtkTextView
 * @pixels_inside_wrap: default number of pixels between wrapped lines
 *
 * Sets the default number of pixels of blank space to leave between
 * display/wrapped lines within a paragraph. May be overridden by
 * tags in @text_view's buffer.
 **/
void
btk_text_view_set_pixels_inside_wrap (BtkTextView *text_view,
                                      gint         pixels_inside_wrap)
{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));

  if (text_view->pixels_inside_wrap != pixels_inside_wrap)
    {
      text_view->pixels_inside_wrap = pixels_inside_wrap;

      if (text_view->layout)
        {
          text_view->layout->default_style->pixels_inside_wrap = pixels_inside_wrap;
          btk_text_layout_default_style_changed (text_view->layout);
        }

      g_object_notify (G_OBJECT (text_view), "pixels-inside-wrap");
    }
}

/**
 * btk_text_view_get_pixels_inside_wrap:
 * @text_view: a #BtkTextView
 * 
 * Gets the value set by btk_text_view_set_pixels_inside_wrap().
 * 
 * Return value: default number of pixels of blank space between wrapped lines
 **/
gint
btk_text_view_get_pixels_inside_wrap (BtkTextView *text_view)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), 0);

  return text_view->pixels_inside_wrap;
}

/**
 * btk_text_view_set_justification:
 * @text_view: a #BtkTextView
 * @justification: justification
 *
 * Sets the default justification of text in @text_view.
 * Tags in the view's buffer may override the default.
 * 
 **/
void
btk_text_view_set_justification (BtkTextView     *text_view,
                                 BtkJustification justification)
{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));

  if (text_view->justify != justification)
    {
      text_view->justify = justification;

      if (text_view->layout)
        {
          text_view->layout->default_style->justification = justification;
          btk_text_layout_default_style_changed (text_view->layout);
        }

      g_object_notify (G_OBJECT (text_view), "justification");
    }
}

/**
 * btk_text_view_get_justification:
 * @text_view: a #BtkTextView
 * 
 * Gets the default justification of paragraphs in @text_view.
 * Tags in the buffer may override the default.
 * 
 * Return value: default justification
 **/
BtkJustification
btk_text_view_get_justification (BtkTextView *text_view)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), BTK_JUSTIFY_LEFT);

  return text_view->justify;
}

/**
 * btk_text_view_set_left_margin:
 * @text_view: a #BtkTextView
 * @left_margin: left margin in pixels
 * 
 * Sets the default left margin for text in @text_view.
 * Tags in the buffer may override the default.
 **/
void
btk_text_view_set_left_margin (BtkTextView *text_view,
                               gint         left_margin)
{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));

  if (text_view->left_margin != left_margin)
    {
      text_view->left_margin = left_margin;

      if (text_view->layout)
        {
          text_view->layout->default_style->left_margin = left_margin;
          btk_text_layout_default_style_changed (text_view->layout);
        }

      g_object_notify (G_OBJECT (text_view), "left-margin");
    }
}

/**
 * btk_text_view_get_left_margin:
 * @text_view: a #BtkTextView
 * 
 * Gets the default left margin size of paragraphs in the @text_view.
 * Tags in the buffer may override the default.
 * 
 * Return value: left margin in pixels
 **/
gint
btk_text_view_get_left_margin (BtkTextView *text_view)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), 0);

  return text_view->left_margin;
}

/**
 * btk_text_view_set_right_margin:
 * @text_view: a #BtkTextView
 * @right_margin: right margin in pixels
 *
 * Sets the default right margin for text in the text view.
 * Tags in the buffer may override the default.
 **/
void
btk_text_view_set_right_margin (BtkTextView *text_view,
                                gint         right_margin)
{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));

  if (text_view->right_margin != right_margin)
    {
      text_view->right_margin = right_margin;

      if (text_view->layout)
        {
          text_view->layout->default_style->right_margin = right_margin;
          btk_text_layout_default_style_changed (text_view->layout);
        }

      g_object_notify (G_OBJECT (text_view), "right-margin");
    }
}

/**
 * btk_text_view_get_right_margin:
 * @text_view: a #BtkTextView
 * 
 * Gets the default right margin for text in @text_view. Tags
 * in the buffer may override the default.
 * 
 * Return value: right margin in pixels
 **/
gint
btk_text_view_get_right_margin (BtkTextView *text_view)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), 0);

  return text_view->right_margin;
}

/**
 * btk_text_view_set_indent:
 * @text_view: a #BtkTextView
 * @indent: indentation in pixels
 *
 * Sets the default indentation for paragraphs in @text_view.
 * Tags in the buffer may override the default.
 **/
void
btk_text_view_set_indent (BtkTextView *text_view,
                          gint         indent)
{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));

  if (text_view->indent != indent)
    {
      text_view->indent = indent;

      if (text_view->layout)
        {
          text_view->layout->default_style->indent = indent;
          btk_text_layout_default_style_changed (text_view->layout);
        }

      g_object_notify (G_OBJECT (text_view), "indent");
    }
}

/**
 * btk_text_view_get_indent:
 * @text_view: a #BtkTextView
 * 
 * Gets the default indentation of paragraphs in @text_view.
 * Tags in the view's buffer may override the default.
 * The indentation may be negative.
 * 
 * Return value: number of pixels of indentation
 **/
gint
btk_text_view_get_indent (BtkTextView *text_view)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), 0);

  return text_view->indent;
}

/**
 * btk_text_view_set_tabs:
 * @text_view: a #BtkTextView
 * @tabs: tabs as a #BangoTabArray
 *
 * Sets the default tab stops for paragraphs in @text_view.
 * Tags in the buffer may override the default.
 **/
void
btk_text_view_set_tabs (BtkTextView   *text_view,
                        BangoTabArray *tabs)
{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));

  if (text_view->tabs)
    bango_tab_array_free (text_view->tabs);

  text_view->tabs = tabs ? bango_tab_array_copy (tabs) : NULL;

  if (text_view->layout)
    {
      /* some unkosher futzing in internal struct details... */
      if (text_view->layout->default_style->tabs)
        bango_tab_array_free (text_view->layout->default_style->tabs);

      text_view->layout->default_style->tabs =
        text_view->tabs ? bango_tab_array_copy (text_view->tabs) : NULL;

      btk_text_layout_default_style_changed (text_view->layout);
    }

  g_object_notify (G_OBJECT (text_view), "tabs");
}

/**
 * btk_text_view_get_tabs:
 * @text_view: a #BtkTextView
 * 
 * Gets the default tabs for @text_view. Tags in the buffer may
 * override the defaults. The returned array will be %NULL if
 * "standard" (8-space) tabs are used. Free the return value
 * with bango_tab_array_free().
 * 
 * Return value: copy of default tab array, or %NULL if "standard" 
 *    tabs are used; must be freed with bango_tab_array_free().
 **/
BangoTabArray*
btk_text_view_get_tabs (BtkTextView *text_view)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), NULL);

  return text_view->tabs ? bango_tab_array_copy (text_view->tabs) : NULL;
}

static void
btk_text_view_toggle_cursor_visible (BtkTextView *text_view)
{
  btk_text_view_set_cursor_visible (text_view, !text_view->cursor_visible);
}

/**
 * btk_text_view_set_cursor_visible:
 * @text_view: a #BtkTextView
 * @setting: whether to show the insertion cursor
 *
 * Toggles whether the insertion point is displayed. A buffer with no editable
 * text probably shouldn't have a visible cursor, so you may want to turn
 * the cursor off.
 **/
void
btk_text_view_set_cursor_visible (BtkTextView *text_view,
				  gboolean     setting)
{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));

  setting = (setting != FALSE);

  if (text_view->cursor_visible != setting)
    {
      text_view->cursor_visible = setting;

      if (btk_widget_has_focus (BTK_WIDGET (text_view)))
        {
          if (text_view->layout)
            {
              btk_text_layout_set_cursor_visible (text_view->layout, setting);
	      btk_text_view_check_cursor_blink (text_view);
            }
        }

      g_object_notify (G_OBJECT (text_view), "cursor-visible");
    }
}

/**
 * btk_text_view_get_cursor_visible:
 * @text_view: a #BtkTextView
 *
 * Find out whether the cursor is being displayed.
 *
 * Return value: whether the insertion mark is visible
 **/
gboolean
btk_text_view_get_cursor_visible (BtkTextView *text_view)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), FALSE);

  return text_view->cursor_visible;
}


/**
 * btk_text_view_place_cursor_onscreen:
 * @text_view: a #BtkTextView
 *
 * Moves the cursor to the currently visible rebunnyion of the
 * buffer, it it isn't there already.
 *
 * Return value: %TRUE if the cursor had to be moved.
 **/
gboolean
btk_text_view_place_cursor_onscreen (BtkTextView *text_view)
{
  BtkTextIter insert;

  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), FALSE);

  btk_text_buffer_get_iter_at_mark (get_buffer (text_view), &insert,
                                    btk_text_buffer_get_insert (get_buffer (text_view)));

  if (clamp_iter_onscreen (text_view, &insert))
    {
      btk_text_buffer_place_cursor (get_buffer (text_view), &insert);
      return TRUE;
    }
  else
    return FALSE;
}

static void
btk_text_view_remove_validate_idles (BtkTextView *text_view)
{
  if (text_view->first_validate_idle != 0)
    {
      DV (g_print ("Removing first validate idle: %s\n", B_STRLOC));
      g_source_remove (text_view->first_validate_idle);
      text_view->first_validate_idle = 0;
    }

  if (text_view->incremental_validate_idle != 0)
    {
      g_source_remove (text_view->incremental_validate_idle);
      text_view->incremental_validate_idle = 0;
    }
}

static void
btk_text_view_destroy (BtkObject *object)
{
  BtkTextView *text_view;
  BtkTextViewPrivate *priv;

  text_view = BTK_TEXT_VIEW (object);
  priv = BTK_TEXT_VIEW_GET_PRIVATE (text_view);

  btk_text_view_remove_validate_idles (text_view);
  btk_text_view_set_buffer (text_view, NULL);
  btk_text_view_destroy_layout (text_view);

  if (text_view->scroll_timeout)
    {
      g_source_remove (text_view->scroll_timeout);
      text_view->scroll_timeout = 0;
    }

  if (priv->im_spot_idle)
    {
      g_source_remove (priv->im_spot_idle);
      priv->im_spot_idle = 0;
    }

  BTK_OBJECT_CLASS (btk_text_view_parent_class)->destroy (object);
}

static void
btk_text_view_finalize (GObject *object)
{
  BtkTextView *text_view;
  BtkTextViewPrivate *priv;

  text_view = BTK_TEXT_VIEW (object);
  priv = BTK_TEXT_VIEW_GET_PRIVATE (text_view);

  btk_text_view_destroy_layout (text_view);
  btk_text_view_set_buffer (text_view, NULL);

  /* at this point, no "notify::buffer" handler should recreate the buffer. */
  g_assert (text_view->buffer == NULL);
  
  cancel_pending_scroll (text_view);

  if (text_view->tabs)
    bango_tab_array_free (text_view->tabs);
  
  if (text_view->hadjustment)
    g_object_unref (text_view->hadjustment);
  if (text_view->vadjustment)
    g_object_unref (text_view->vadjustment);

  text_window_free (text_view->text_window);

  if (text_view->left_window)
    text_window_free (text_view->left_window);

  if (text_view->top_window)
    text_window_free (text_view->top_window);

  if (text_view->right_window)
    text_window_free (text_view->right_window);

  if (text_view->bottom_window)
    text_window_free (text_view->bottom_window);

  g_object_unref (text_view->im_context);

  g_free (priv->im_module);

  G_OBJECT_CLASS (btk_text_view_parent_class)->finalize (object);
}

static void
btk_text_view_set_property (GObject         *object,
			    guint            prop_id,
			    const GValue    *value,
			    GParamSpec      *pspec)
{
  BtkTextView *text_view;
  BtkTextViewPrivate *priv;

  text_view = BTK_TEXT_VIEW (object);
  priv = BTK_TEXT_VIEW_GET_PRIVATE (text_view);

  switch (prop_id)
    {
    case PROP_PIXELS_ABOVE_LINES:
      btk_text_view_set_pixels_above_lines (text_view, g_value_get_int (value));
      break;

    case PROP_PIXELS_BELOW_LINES:
      btk_text_view_set_pixels_below_lines (text_view, g_value_get_int (value));
      break;

    case PROP_PIXELS_INSIDE_WRAP:
      btk_text_view_set_pixels_inside_wrap (text_view, g_value_get_int (value));
      break;

    case PROP_EDITABLE:
      btk_text_view_set_editable (text_view, g_value_get_boolean (value));
      break;

    case PROP_WRAP_MODE:
      btk_text_view_set_wrap_mode (text_view, g_value_get_enum (value));
      break;
      
    case PROP_JUSTIFICATION:
      btk_text_view_set_justification (text_view, g_value_get_enum (value));
      break;

    case PROP_LEFT_MARGIN:
      btk_text_view_set_left_margin (text_view, g_value_get_int (value));
      break;

    case PROP_RIGHT_MARGIN:
      btk_text_view_set_right_margin (text_view, g_value_get_int (value));
      break;

    case PROP_INDENT:
      btk_text_view_set_indent (text_view, g_value_get_int (value));
      break;

    case PROP_TABS:
      btk_text_view_set_tabs (text_view, g_value_get_boxed (value));
      break;

    case PROP_CURSOR_VISIBLE:
      btk_text_view_set_cursor_visible (text_view, g_value_get_boolean (value));
      break;

    case PROP_OVERWRITE:
      btk_text_view_set_overwrite (text_view, g_value_get_boolean (value));
      break;

    case PROP_BUFFER:
      btk_text_view_set_buffer (text_view, BTK_TEXT_BUFFER (g_value_get_object (value)));
      break;

    case PROP_ACCEPTS_TAB:
      btk_text_view_set_accepts_tab (text_view, g_value_get_boolean (value));
      break;
      
    case PROP_IM_MODULE:
      g_free (priv->im_module);
      priv->im_module = g_value_dup_string (value);
      if (BTK_IS_IM_MULTICONTEXT (text_view->im_context))
        btk_im_multicontext_set_context_id (BTK_IM_MULTICONTEXT (text_view->im_context), priv->im_module);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_text_view_get_property (GObject         *object,
			    guint            prop_id,
			    GValue          *value,
			    GParamSpec      *pspec)
{
  BtkTextView *text_view;
  BtkTextViewPrivate *priv;

  text_view = BTK_TEXT_VIEW (object);
  priv = BTK_TEXT_VIEW_GET_PRIVATE (text_view);

  switch (prop_id)
    {
    case PROP_PIXELS_ABOVE_LINES:
      g_value_set_int (value, text_view->pixels_above_lines);
      break;

    case PROP_PIXELS_BELOW_LINES:
      g_value_set_int (value, text_view->pixels_below_lines);
      break;

    case PROP_PIXELS_INSIDE_WRAP:
      g_value_set_int (value, text_view->pixels_inside_wrap);
      break;

    case PROP_EDITABLE:
      g_value_set_boolean (value, text_view->editable);
      break;
      
    case PROP_WRAP_MODE:
      g_value_set_enum (value, text_view->wrap_mode);
      break;

    case PROP_JUSTIFICATION:
      g_value_set_enum (value, text_view->justify);
      break;

    case PROP_LEFT_MARGIN:
      g_value_set_int (value, text_view->left_margin);
      break;

    case PROP_RIGHT_MARGIN:
      g_value_set_int (value, text_view->right_margin);
      break;

    case PROP_INDENT:
      g_value_set_int (value, text_view->indent);
      break;

    case PROP_TABS:
      g_value_set_boxed (value, text_view->tabs);
      break;

    case PROP_CURSOR_VISIBLE:
      g_value_set_boolean (value, text_view->cursor_visible);
      break;

    case PROP_BUFFER:
      g_value_set_object (value, get_buffer (text_view));
      break;

    case PROP_OVERWRITE:
      g_value_set_boolean (value, text_view->overwrite_mode);
      break;

    case PROP_ACCEPTS_TAB:
      g_value_set_boolean (value, text_view->accepts_tab);
      break;
      
    case PROP_IM_MODULE:
      g_value_set_string (value, priv->im_module);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_text_view_size_request (BtkWidget      *widget,
                            BtkRequisition *requisition)
{
  BtkTextView *text_view;
  GSList *tmp_list;
  gint focus_edge_width;
  gint focus_width;
  gboolean interior_focus;
  
  text_view = BTK_TEXT_VIEW (widget);

  btk_widget_style_get (widget,
			"interior-focus", &interior_focus,
			"focus-line-width", &focus_width,
			NULL);

  if (interior_focus)
    focus_edge_width = 0;
  else
    focus_edge_width = focus_width;

  if (text_view->layout)
    {
      text_view->text_window->requisition.width = text_view->layout->width;
      text_view->text_window->requisition.height = text_view->layout->height;
    }
  else
    {
      text_view->text_window->requisition.width = 0;
      text_view->text_window->requisition.height = 0;
    }
  
  requisition->width = text_view->text_window->requisition.width + focus_edge_width * 2;
  requisition->height = text_view->text_window->requisition.height + focus_edge_width * 2;

  if (text_view->left_window)
    requisition->width += text_view->left_window->requisition.width;

  if (text_view->right_window)
    requisition->width += text_view->right_window->requisition.width;

  if (text_view->top_window)
    requisition->height += text_view->top_window->requisition.height;

  if (text_view->bottom_window)
    requisition->height += text_view->bottom_window->requisition.height;

  requisition->width += BTK_CONTAINER (text_view)->border_width * 2;
  requisition->height += BTK_CONTAINER (text_view)->border_width * 2;
  
  tmp_list = text_view->children;
  while (tmp_list != NULL)
    {
      BtkTextViewChild *child = tmp_list->data;

      if (child->anchor)
        {
          BtkRequisition child_req;
          BtkRequisition old_req;

          btk_widget_get_child_requisition (child->widget, &old_req);
          
          btk_widget_size_request (child->widget, &child_req);

          btk_widget_get_child_requisition (child->widget, &child_req);

          /* Invalidate layout lines if required */
          if (text_view->layout &&
              (old_req.width != child_req.width ||
               old_req.height != child_req.height))
            btk_text_child_anchor_queue_resize (child->anchor,
                                                text_view->layout);
        }
      else
        {
          BtkRequisition child_req;
          
          btk_widget_size_request (child->widget, &child_req);
        }

      tmp_list = g_slist_next (tmp_list);
    }
}

static void
btk_text_view_compute_child_allocation (BtkTextView      *text_view,
                                        BtkTextViewChild *vc,
                                        BtkAllocation    *allocation)
{
  gint buffer_y;
  BtkTextIter iter;
  BtkRequisition req;
  
  btk_text_buffer_get_iter_at_child_anchor (get_buffer (text_view),
                                            &iter,
                                            vc->anchor);

  btk_text_layout_get_line_yrange (text_view->layout, &iter,
                                   &buffer_y, NULL);

  buffer_y += vc->from_top_of_line;

  allocation->x = vc->from_left_of_buffer - text_view->xoffset;
  allocation->y = buffer_y - text_view->yoffset;

  btk_widget_get_child_requisition (vc->widget, &req);
  allocation->width = req.width;
  allocation->height = req.height;
}

static void
btk_text_view_update_child_allocation (BtkTextView      *text_view,
                                       BtkTextViewChild *vc)
{
  BtkAllocation allocation;

  btk_text_view_compute_child_allocation (text_view, vc, &allocation);
  
  btk_widget_size_allocate (vc->widget, &allocation);

#if 0
  g_print ("allocation for %p allocated to %d,%d yoffset = %d\n",
           vc->widget,
           vc->widget->allocation.x,
           vc->widget->allocation.y,
           text_view->yoffset);
#endif
}

static void
btk_text_view_child_allocated (BtkTextLayout *layout,
                               BtkWidget     *child,
                               gint           x,
                               gint           y,
                               gpointer       data)
{
  BtkTextViewChild *vc = NULL;
  BtkTextView *text_view = data;
  
  /* x,y is the position of the child from the top of the line, and
   * from the left of the buffer. We have to translate that into text
   * window coordinates, then size_allocate the child.
   */

  vc = g_object_get_data (G_OBJECT (child),
                          "btk-text-view-child");

  g_assert (vc != NULL);

  DV (g_print ("child allocated at %d,%d\n", x, y));
  
  vc->from_left_of_buffer = x;
  vc->from_top_of_line = y;

  btk_text_view_update_child_allocation (text_view, vc);
}

static void
btk_text_view_allocate_children (BtkTextView *text_view)
{
  GSList *tmp_list;

  DV(g_print(B_STRLOC"\n"));
  
  tmp_list = text_view->children;
  while (tmp_list != NULL)
    {
      BtkTextViewChild *child = tmp_list->data;

      g_assert (child != NULL);
          
      if (child->anchor)
        {
          /* We need to force-validate the rebunnyions containing
           * children.
           */
          BtkTextIter child_loc;
          btk_text_buffer_get_iter_at_child_anchor (get_buffer (text_view),
                                                    &child_loc,
                                                    child->anchor);

	  /* Since anchored children are only ever allocated from
           * btk_text_layout_get_line_display() we have to make sure
	   * that the display line caching in the layout doesn't 
           * get in the way. Invalidating the layout around the anchor
           * achieves this.
	   */ 
	  if (BTK_WIDGET_ALLOC_NEEDED (child->widget))
	    {
	      BtkTextIter end = child_loc;
	      btk_text_iter_forward_char (&end);
	      btk_text_layout_invalidate (text_view->layout, &child_loc, &end);
	    }

          btk_text_layout_validate_yrange (text_view->layout,
                                           &child_loc,
                                           0, 1);
        }
      else
        {
          BtkAllocation allocation;          
          BtkRequisition child_req;
             
          allocation.x = child->x;
          allocation.y = child->y;

          btk_widget_get_child_requisition (child->widget, &child_req);
          
          allocation.width = child_req.width;
          allocation.height = child_req.height;
          
          btk_widget_size_allocate (child->widget, &allocation);          
        }

      tmp_list = g_slist_next (tmp_list);
    }
}

static void
btk_text_view_size_allocate (BtkWidget *widget,
                             BtkAllocation *allocation)
{
  BtkTextView *text_view;
  BtkTextIter first_para;
  gint y;
  gint width, height;
  BdkRectangle text_rect;
  BdkRectangle left_rect;
  BdkRectangle right_rect;
  BdkRectangle top_rect;
  BdkRectangle bottom_rect;
  gint focus_edge_width;
  gint focus_width;
  gboolean interior_focus;
  gboolean size_changed;
  
  text_view = BTK_TEXT_VIEW (widget);

  DV(g_print(B_STRLOC"\n"));

  size_changed =
    widget->allocation.width != allocation->width ||
    widget->allocation.height != allocation->height;
  
  widget->allocation = *allocation;

  if (btk_widget_get_realized (widget))
    {
      bdk_window_move_resize (widget->window,
                              allocation->x, allocation->y,
                              allocation->width, allocation->height);
    }

  /* distribute width/height among child windows. Ensure all
   * windows get at least a 1x1 allocation.
   */

  btk_widget_style_get (widget,
			"interior-focus", &interior_focus,
			"focus-line-width", &focus_width,
			NULL);

  if (interior_focus)
    focus_edge_width = 0;
  else
    focus_edge_width = focus_width;
  
  width = allocation->width - focus_edge_width * 2 - BTK_CONTAINER (text_view)->border_width * 2;

  if (text_view->left_window)
    left_rect.width = text_view->left_window->requisition.width;
  else
    left_rect.width = 0;

  width -= left_rect.width;

  if (text_view->right_window)
    right_rect.width = text_view->right_window->requisition.width;
  else
    right_rect.width = 0;

  width -= right_rect.width;

  text_rect.width = MAX (1, width);

  top_rect.width = text_rect.width;
  bottom_rect.width = text_rect.width;


  height = allocation->height - focus_edge_width * 2 - BTK_CONTAINER (text_view)->border_width * 2;

  if (text_view->top_window)
    top_rect.height = text_view->top_window->requisition.height;
  else
    top_rect.height = 0;

  height -= top_rect.height;

  if (text_view->bottom_window)
    bottom_rect.height = text_view->bottom_window->requisition.height;
  else
    bottom_rect.height = 0;

  height -= bottom_rect.height;

  text_rect.height = MAX (1, height);

  left_rect.height = text_rect.height;
  right_rect.height = text_rect.height;

  /* Origins */
  left_rect.x = focus_edge_width + BTK_CONTAINER (text_view)->border_width;
  top_rect.y = focus_edge_width + BTK_CONTAINER (text_view)->border_width;

  text_rect.x = left_rect.x + left_rect.width;
  text_rect.y = top_rect.y + top_rect.height;

  left_rect.y = text_rect.y;
  right_rect.y = text_rect.y;

  top_rect.x = text_rect.x;
  bottom_rect.x = text_rect.x;

  right_rect.x = text_rect.x + text_rect.width;
  bottom_rect.y = text_rect.y + text_rect.height;

  text_window_size_allocate (text_view->text_window,
                             &text_rect);

  if (text_view->left_window)
    text_window_size_allocate (text_view->left_window,
                               &left_rect);

  if (text_view->right_window)
    text_window_size_allocate (text_view->right_window,
                               &right_rect);

  if (text_view->top_window)
    text_window_size_allocate (text_view->top_window,
                               &top_rect);

  if (text_view->bottom_window)
    text_window_size_allocate (text_view->bottom_window,
                               &bottom_rect);

  btk_text_view_update_layout_width (text_view);
  
  /* Note that this will do some layout validation */
  btk_text_view_allocate_children (text_view);

  /* Ensure h/v adj exist */
  get_hadjustment (text_view);
  get_vadjustment (text_view);

  text_view->hadjustment->page_size = SCREEN_WIDTH (text_view);
  text_view->hadjustment->page_increment = SCREEN_WIDTH (text_view) * 0.9;
  text_view->hadjustment->step_increment = SCREEN_WIDTH (text_view) * 0.1;
  text_view->hadjustment->lower = 0;
  text_view->hadjustment->upper = MAX (SCREEN_WIDTH (text_view),
                                       text_view->width);

  if (text_view->hadjustment->value > text_view->hadjustment->upper - text_view->hadjustment->page_size)
    btk_adjustment_set_value (text_view->hadjustment, MAX (0, text_view->hadjustment->upper - text_view->hadjustment->page_size));

  btk_adjustment_changed (text_view->hadjustment);

  text_view->vadjustment->page_size = SCREEN_HEIGHT (text_view);
  text_view->vadjustment->page_increment = SCREEN_HEIGHT (text_view) * 0.9;
  text_view->vadjustment->step_increment = SCREEN_HEIGHT (text_view) * 0.1;
  text_view->vadjustment->lower = 0;
  text_view->vadjustment->upper = MAX (SCREEN_HEIGHT (text_view),
                                       text_view->height);

  /* Now adjust the value of the adjustment to keep the cursor at the
   * same place in the buffer
   */
  btk_text_view_get_first_para_iter (text_view, &first_para);
  btk_text_layout_get_line_yrange (text_view->layout, &first_para, &y, NULL);

  y += text_view->first_para_pixels;

  if (y > text_view->vadjustment->upper - text_view->vadjustment->page_size)
    y = MAX (0, text_view->vadjustment->upper - text_view->vadjustment->page_size);

  if (y != text_view->yoffset)
    btk_adjustment_set_value (text_view->vadjustment, y);

  btk_adjustment_changed (text_view->vadjustment);

  /* The BTK resize loop processes all the pending exposes right
   * after doing the resize stuff, so the idle sizer won't have a
   * chance to run. So we do the work here. 
   */
  btk_text_view_flush_first_validate (text_view);

  /* widget->window doesn't get auto-redrawn as the layout is computed, so has to
   * be invalidated
   */
  if (size_changed && btk_widget_get_realized (widget))
    bdk_window_invalidate_rect (widget->window, NULL, FALSE);
}

static void
btk_text_view_get_first_para_iter (BtkTextView *text_view,
                                   BtkTextIter *iter)
{
  btk_text_buffer_get_iter_at_mark (get_buffer (text_view), iter,
                                    text_view->first_para_mark);
}

static void
btk_text_view_validate_onscreen (BtkTextView *text_view)
{
  BtkWidget *widget = BTK_WIDGET (text_view);
  
  DV(g_print(">Validating onscreen ("B_STRLOC")\n"));
  
  if (SCREEN_HEIGHT (widget) > 0)
    {
      BtkTextIter first_para;

      /* Be sure we've validated the stuff onscreen; if we
       * scrolled, these calls won't have any effect, because
       * they were called in the recursive validate_onscreen
       */
      btk_text_view_get_first_para_iter (text_view, &first_para);

      btk_text_layout_validate_yrange (text_view->layout,
                                       &first_para,
                                       0,
                                       text_view->first_para_pixels +
                                       SCREEN_HEIGHT (widget));
    }

  text_view->onscreen_validated = TRUE;

  DV(g_print(">Done validating onscreen, onscreen_validated = TRUE ("B_STRLOC")\n"));
  
  /* This can have the odd side effect of triggering a scroll, which should
   * flip "onscreen_validated" back to FALSE, but should also get us
   * back into this function to turn it on again.
   */
  btk_text_view_update_adjustments (text_view);

  g_assert (text_view->onscreen_validated);
}

static void
btk_text_view_flush_first_validate (BtkTextView *text_view)
{
  if (text_view->first_validate_idle == 0)
    return;

  /* Do this first, which means that if an "invalidate"
   * occurs during any of this process, a new first_validate_callback
   * will be installed, and we'll start again.
   */
  DV (g_print ("removing first validate in %s\n", B_STRLOC));
  g_source_remove (text_view->first_validate_idle);
  text_view->first_validate_idle = 0;
  
  /* be sure we have up-to-date screen size set on the
   * layout.
   */
  btk_text_view_update_layout_width (text_view);

  /* Bail out if we invalidated stuff; scrolling right away will just
   * confuse the issue.
   */
  if (text_view->first_validate_idle != 0)
    {
      DV(g_print(">Width change forced requeue ("B_STRLOC")\n"));
    }
  else
    {
      /* scroll to any marks, if that's pending. This can jump us to
       * the validation codepath used for scrolling onscreen, if so we
       * bail out.  It won't jump if already in that codepath since
       * value_changed is not recursive, so also validate if
       * necessary.
       */
      if (!btk_text_view_flush_scroll (text_view) ||
          !text_view->onscreen_validated)
	btk_text_view_validate_onscreen (text_view);
      
      DV(g_print(">Leaving first validate idle ("B_STRLOC")\n"));
      
      g_assert (text_view->onscreen_validated);
    }
}

static gboolean
first_validate_callback (gpointer data)
{
  BtkTextView *text_view = data;

  /* Note that some of this code is duplicated at the end of size_allocate,
   * keep in sync with that.
   */
  
  DV(g_print(B_STRLOC"\n"));

  btk_text_view_flush_first_validate (text_view);
  
  return FALSE;
}

static gboolean
incremental_validate_callback (gpointer data)
{
  BtkTextView *text_view = data;
  gboolean result = TRUE;

  DV(g_print(B_STRLOC"\n"));
  
  btk_text_layout_validate (text_view->layout, 2000);

  btk_text_view_update_adjustments (text_view);
  
  if (btk_text_layout_is_valid (text_view->layout))
    {
      text_view->incremental_validate_idle = 0;
      result = FALSE;
    }

  return result;
}

static void
btk_text_view_invalidate (BtkTextView *text_view)
{  
  DV (g_print (">Invalidate, onscreen_validated = %d now FALSE ("B_STRLOC")\n",
               text_view->onscreen_validated));

  text_view->onscreen_validated = FALSE;

  /* We'll invalidate when the layout is created */
  if (text_view->layout == NULL)
    return;
  
  if (!text_view->first_validate_idle)
    {
      text_view->first_validate_idle = bdk_threads_add_idle_full (BTK_PRIORITY_RESIZE - 2, first_validate_callback, text_view, NULL);
      DV (g_print (B_STRLOC": adding first validate idle %d\n",
                   text_view->first_validate_idle));
    }
      
  if (!text_view->incremental_validate_idle)
    {
      text_view->incremental_validate_idle = bdk_threads_add_idle_full (BTK_TEXT_VIEW_PRIORITY_VALIDATE, incremental_validate_callback, text_view, NULL);
      DV (g_print (B_STRLOC": adding incremental validate idle %d\n",
                   text_view->incremental_validate_idle));
    }
}

static void
invalidated_handler (BtkTextLayout *layout,
                     gpointer       data)
{
  BtkTextView *text_view;

  text_view = BTK_TEXT_VIEW (data);

  DV (g_print ("Invalidating due to layout invalidate signal\n"));
  btk_text_view_invalidate (text_view);
}

static void
changed_handler (BtkTextLayout     *layout,
                 gint               start_y,
                 gint               old_height,
                 gint               new_height,
                 gpointer           data)
{
  BtkTextView *text_view;
  BtkWidget *widget;
  BdkRectangle visible_rect;
  BdkRectangle redraw_rect;
  
  text_view = BTK_TEXT_VIEW (data);
  widget = BTK_WIDGET (data);
  
  DV(g_print(">Lines Validated ("B_STRLOC")\n"));

  if (btk_widget_get_realized (widget))
    {      
      btk_text_view_get_visible_rect (text_view, &visible_rect);

      redraw_rect.x = visible_rect.x;
      redraw_rect.width = visible_rect.width;
      redraw_rect.y = start_y;

      if (old_height == new_height)
        redraw_rect.height = old_height;
      else if (start_y + old_height > visible_rect.y)
        redraw_rect.height = MAX (0, visible_rect.y + visible_rect.height - start_y);
      else
        redraw_rect.height = 0;
	
      if (bdk_rectangle_intersect (&redraw_rect, &visible_rect, &redraw_rect))
        {
          /* text_window_invalidate_rect() takes buffer coordinates */
          text_window_invalidate_rect (text_view->text_window,
                                       &redraw_rect);

          DV(g_print(" invalidated rect: %d,%d %d x %d\n",
                     redraw_rect.x,
                     redraw_rect.y,
                     redraw_rect.width,
                     redraw_rect.height));
          
          if (text_view->left_window)
            text_window_invalidate_rect (text_view->left_window,
                                         &redraw_rect);
          if (text_view->right_window)
            text_window_invalidate_rect (text_view->right_window,
                                         &redraw_rect);
          if (text_view->top_window)
            text_window_invalidate_rect (text_view->top_window,
                                         &redraw_rect);
          if (text_view->bottom_window)
            text_window_invalidate_rect (text_view->bottom_window,
                                         &redraw_rect);

          queue_update_im_spot_location (text_view);
        }
    }
  
  if (old_height != new_height)
    {
      gboolean yoffset_changed = FALSE;
      GSList *tmp_list;
      int new_first_para_top;
      int old_first_para_top;
      BtkTextIter first;
      
      /* If the bottom of the old area was above the top of the
       * screen, we need to scroll to keep the current top of the
       * screen in place.  Remember that first_para_pixels is the
       * position of the top of the screen in coordinates relative to
       * the first paragraph onscreen.
       *
       * In short we are adding the height delta of the portion of the
       * changed rebunnyion above first_para_mark to text_view->yoffset.
       */
      btk_text_buffer_get_iter_at_mark (get_buffer (text_view), &first,
                                        text_view->first_para_mark);

      btk_text_layout_get_line_yrange (layout, &first, &new_first_para_top, NULL);

      old_first_para_top = text_view->yoffset - text_view->first_para_pixels;

      if (new_first_para_top != old_first_para_top)
        {
          text_view->yoffset += new_first_para_top - old_first_para_top;
          
          get_vadjustment (text_view)->value = text_view->yoffset;
          yoffset_changed = TRUE;
        }

      if (yoffset_changed)
        {
          DV(g_print ("Changing scroll position (%s)\n", B_STRLOC));
          btk_adjustment_value_changed (get_vadjustment (text_view));
        }

      /* FIXME be smarter about which anchored widgets we update */

      tmp_list = text_view->children;
      while (tmp_list != NULL)
        {
          BtkTextViewChild *child = tmp_list->data;

          if (child->anchor)
            btk_text_view_update_child_allocation (text_view, child);

          tmp_list = g_slist_next (tmp_list);
        }
    }

  {
    BtkRequisition old_req;
    BtkRequisition new_req;

    old_req = widget->requisition;

    /* Use this instead of btk_widget_size_request wrapper
     * to avoid the optimization which just returns widget->requisition
     * if a resize hasn't been queued.
     */
    BTK_WIDGET_GET_CLASS (widget)->size_request (widget, &new_req);

    if (old_req.width != new_req.width ||
        old_req.height != new_req.height)
      {
	btk_widget_queue_resize_no_redraw (widget);
      }
  }
}

static void
btk_text_view_realize (BtkWidget *widget)
{
  BtkTextView *text_view;
  BdkWindowAttr attributes;
  gint attributes_mask;
  GSList *tmp_list;
  
  text_view = BTK_TEXT_VIEW (widget);

  btk_widget_set_realized (widget, TRUE);

  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);
  attributes.event_mask = BDK_VISIBILITY_NOTIFY_MASK | BDK_EXPOSURE_MASK;

  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;

  widget->window = bdk_window_new (btk_widget_get_parent_window (widget),
                                   &attributes, attributes_mask);
  bdk_window_set_user_data (widget->window, widget);

  /* must come before text_window_realize calls */
  widget->style = btk_style_attach (widget->style, widget->window);

  bdk_window_set_background (widget->window,
                             &widget->style->bg[btk_widget_get_state (widget)]);

  text_window_realize (text_view->text_window, widget);

  if (text_view->left_window)
    text_window_realize (text_view->left_window, widget);

  if (text_view->top_window)
    text_window_realize (text_view->top_window, widget);

  if (text_view->right_window)
    text_window_realize (text_view->right_window, widget);

  if (text_view->bottom_window)
    text_window_realize (text_view->bottom_window, widget);

  btk_text_view_ensure_layout (text_view);

  if (text_view->buffer)
    {
      BtkClipboard *clipboard = btk_widget_get_clipboard (BTK_WIDGET (text_view),
							  BDK_SELECTION_PRIMARY);
      btk_text_buffer_add_selection_clipboard (text_view->buffer, clipboard);
    }

  tmp_list = text_view->children;
  while (tmp_list != NULL)
    {
      BtkTextViewChild *vc = tmp_list->data;
      
      text_view_child_set_parent_window (text_view, vc);
      
      tmp_list = tmp_list->next;
    }

  /* Ensure updating the spot location. */
  btk_text_view_update_im_spot_location (text_view);
}

static void
btk_text_view_unrealize (BtkWidget *widget)
{
  BtkTextView *text_view;
  
  text_view = BTK_TEXT_VIEW (widget);

  if (text_view->buffer)
    {
      BtkClipboard *clipboard = btk_widget_get_clipboard (BTK_WIDGET (text_view),
							  BDK_SELECTION_PRIMARY);
      btk_text_buffer_remove_selection_clipboard (text_view->buffer, clipboard);
    }

  btk_text_view_remove_validate_idles (text_view);

  if (text_view->popup_menu)
    {
      btk_widget_destroy (text_view->popup_menu);
      text_view->popup_menu = NULL;
    }

  text_window_unrealize (text_view->text_window);

  if (text_view->left_window)
    text_window_unrealize (text_view->left_window);

  if (text_view->top_window)
    text_window_unrealize (text_view->top_window);

  if (text_view->right_window)
    text_window_unrealize (text_view->right_window);

  if (text_view->bottom_window)
    text_window_unrealize (text_view->bottom_window);

  btk_text_view_destroy_layout (text_view);

  BTK_WIDGET_CLASS (btk_text_view_parent_class)->unrealize (widget);
}

static void
btk_text_view_set_background (BtkTextView *text_view)
{
  BtkWidget *widget = BTK_WIDGET (text_view);

  bdk_window_set_background (widget->window,
			     &widget->style->bg[btk_widget_get_state (widget)]);
  
  bdk_window_set_background (text_view->text_window->bin_window,
			     &widget->style->base[btk_widget_get_state (widget)]);
  
  if (text_view->left_window)
    bdk_window_set_background (text_view->left_window->bin_window,
			       &widget->style->bg[btk_widget_get_state (widget)]);
  if (text_view->right_window)
    bdk_window_set_background (text_view->right_window->bin_window,
			       &widget->style->bg[btk_widget_get_state (widget)]);
  
  if (text_view->top_window)
    bdk_window_set_background (text_view->top_window->bin_window,
			       &widget->style->bg[btk_widget_get_state (widget)]);
  
  if (text_view->bottom_window)
    bdk_window_set_background (text_view->bottom_window->bin_window,
			       &widget->style->bg[btk_widget_get_state (widget)]);
}

static void
btk_text_view_style_set (BtkWidget *widget,
                         BtkStyle  *previous_style)
{
  BtkTextView *text_view = BTK_TEXT_VIEW (widget);
  BangoContext *ltr_context, *rtl_context;

  if (btk_widget_get_realized (widget))
    {
      btk_text_view_set_background (text_view);
    }

  if (text_view->layout && previous_style)
    {
      btk_text_view_set_attributes_from_style (text_view,
                                               text_view->layout->default_style,
                                               widget->style);
      
      
      ltr_context = btk_widget_create_bango_context (widget);
      bango_context_set_base_dir (ltr_context, BANGO_DIRECTION_LTR);
      rtl_context = btk_widget_create_bango_context (widget);
      bango_context_set_base_dir (rtl_context, BANGO_DIRECTION_RTL);

      btk_text_layout_set_contexts (text_view->layout, ltr_context, rtl_context);

      g_object_unref (ltr_context);
      g_object_unref (rtl_context);
    }
}

static void
btk_text_view_direction_changed (BtkWidget        *widget,
                                 BtkTextDirection  previous_direction)
{
  BtkTextView *text_view = BTK_TEXT_VIEW (widget);

  if (text_view->layout)
    {
      text_view->layout->default_style->direction = btk_widget_get_direction (widget);

      btk_text_layout_default_style_changed (text_view->layout);
    }
}

static void
btk_text_view_state_changed (BtkWidget      *widget,
		 	     BtkStateType    previous_state)
{
  BtkTextView *text_view = BTK_TEXT_VIEW (widget);
  BdkCursor *cursor;

  if (btk_widget_get_realized (widget))
    {
      btk_text_view_set_background (text_view);

      if (btk_widget_is_sensitive (widget))
        cursor = bdk_cursor_new_for_display (btk_widget_get_display (widget), BDK_XTERM);
      else
        cursor = NULL;

      bdk_window_set_cursor (text_view->text_window->bin_window, cursor);

      if (cursor)
      bdk_cursor_unref (cursor);

      text_view->mouse_cursor_obscured = FALSE;
    }

  if (!btk_widget_is_sensitive (widget))
    {
      /* Clear any selection */
      btk_text_view_unselect (text_view);
    }
  
  btk_widget_queue_draw (widget);
}

static void
set_invisible_cursor (BdkWindow *window)
{
  BdkDisplay *display;
  BdkCursor *cursor;

  display = bdk_window_get_display (window);
  cursor = bdk_cursor_new_for_display (display, BDK_BLANK_CURSOR);
 
  bdk_window_set_cursor (window, cursor);
  
  bdk_cursor_unref (cursor);
}

static void
btk_text_view_obscure_mouse_cursor (BtkTextView *text_view)
{
  if (text_view->mouse_cursor_obscured)
    return;

  set_invisible_cursor (text_view->text_window->bin_window);
  
  text_view->mouse_cursor_obscured = TRUE;  
}

static void
btk_text_view_unobscure_mouse_cursor (BtkTextView *text_view)
{
  if (text_view->mouse_cursor_obscured)
    {
      BdkCursor *cursor;
      
      cursor = bdk_cursor_new_for_display (btk_widget_get_display (BTK_WIDGET (text_view)),
					   BDK_XTERM);
      bdk_window_set_cursor (text_view->text_window->bin_window, cursor);
      bdk_cursor_unref (cursor);
      text_view->mouse_cursor_obscured = FALSE;
    }
}

static void
btk_text_view_grab_notify (BtkWidget *widget,
		 	   gboolean   was_grabbed)
{
  if (!was_grabbed)
    {
      btk_text_view_end_selection_drag (BTK_TEXT_VIEW (widget));
      btk_text_view_unobscure_mouse_cursor (BTK_TEXT_VIEW (widget));
    }
}


/*
 * Events
 */

static gboolean
get_event_coordinates (BdkEvent *event, gint *x, gint *y)
{
  if (event)
    switch (event->type)
      {
      case BDK_MOTION_NOTIFY:
        *x = event->motion.x;
        *y = event->motion.y;
        return TRUE;
        break;

      case BDK_BUTTON_PRESS:
      case BDK_2BUTTON_PRESS:
      case BDK_3BUTTON_PRESS:
      case BDK_BUTTON_RELEASE:
        *x = event->button.x;
        *y = event->button.y;
        return TRUE;
        break;

      case BDK_KEY_PRESS:
      case BDK_KEY_RELEASE:
      case BDK_ENTER_NOTIFY:
      case BDK_LEAVE_NOTIFY:
      case BDK_PROPERTY_NOTIFY:
      case BDK_SELECTION_CLEAR:
      case BDK_SELECTION_REQUEST:
      case BDK_SELECTION_NOTIFY:
      case BDK_PROXIMITY_IN:
      case BDK_PROXIMITY_OUT:
      case BDK_DRAG_ENTER:
      case BDK_DRAG_LEAVE:
      case BDK_DRAG_MOTION:
      case BDK_DRAG_STATUS:
      case BDK_DROP_START:
      case BDK_DROP_FINISHED:
      default:
        return FALSE;
        break;
      }

  return FALSE;
}

static gint
emit_event_on_tags (BtkWidget   *widget,
                    BdkEvent    *event,
                    BtkTextIter *iter)
{
  GSList *tags;
  GSList *tmp;
  gboolean retval = FALSE;

  tags = btk_text_iter_get_tags (iter);

  tmp = tags;
  while (tmp != NULL)
    {
      BtkTextTag *tag = tmp->data;

      if (btk_text_tag_event (tag, G_OBJECT (widget), event, iter))
        {
          retval = TRUE;
          break;
        }

      tmp = g_slist_next (tmp);
    }

  g_slist_free (tags);

  return retval;
}

static gint
btk_text_view_event (BtkWidget *widget, BdkEvent *event)
{
  BtkTextView *text_view;
  gint x = 0, y = 0;

  text_view = BTK_TEXT_VIEW (widget);

  if (text_view->layout == NULL ||
      get_buffer (text_view) == NULL)
    return FALSE;

  if (event->any.window != text_view->text_window->bin_window)
    return FALSE;

  if (get_event_coordinates (event, &x, &y))
    {
      BtkTextIter iter;

      x += text_view->xoffset;
      y += text_view->yoffset;

      /* FIXME this is slow and we do it twice per event.
       * My favorite solution is to have BtkTextLayout cache
       * the last couple lookups.
       */
      btk_text_layout_get_iter_at_pixel (text_view->layout,
                                         &iter,
                                         x, y);

      return emit_event_on_tags (widget, event, &iter);
    }
  else if (event->type == BDK_KEY_PRESS ||
           event->type == BDK_KEY_RELEASE)
    {
      BtkTextIter iter;

      btk_text_buffer_get_iter_at_mark (get_buffer (text_view), &iter,
                                        btk_text_buffer_get_insert (get_buffer (text_view)));

      return emit_event_on_tags (widget, event, &iter);
    }
  else
    return FALSE;
}

static gint
btk_text_view_key_press_event (BtkWidget *widget, BdkEventKey *event)
{
  gboolean retval = FALSE;
  gboolean obscure = FALSE;
  BtkTextView *text_view = BTK_TEXT_VIEW (widget);
  BtkTextMark *insert;
  BtkTextIter iter;
  gboolean can_insert;
  
  if (text_view->layout == NULL ||
      get_buffer (text_view) == NULL)
    return FALSE;

  /* Make sure input method knows where it is */
  flush_update_im_spot_location (text_view);

  insert = btk_text_buffer_get_insert (get_buffer (text_view));
  btk_text_buffer_get_iter_at_mark (get_buffer (text_view), &iter, insert);
  can_insert = btk_text_iter_can_insert (&iter, text_view->editable);
  if (btk_im_context_filter_keypress (text_view->im_context, event))
    {
      text_view->need_im_reset = TRUE;
      if (!can_insert)
        btk_text_view_reset_im_context (text_view);
      obscure = can_insert;
      retval = TRUE;
    }
  /* Binding set */
  else if (BTK_WIDGET_CLASS (btk_text_view_parent_class)->key_press_event (widget, event))
    {
      retval = TRUE;
    }
  /* use overall editability not can_insert, more predictable for users */
  else if (text_view->editable &&
           (event->keyval == BDK_Return ||
            event->keyval == BDK_ISO_Enter ||
            event->keyval == BDK_KP_Enter))
    {
      /* this won't actually insert the newline if the cursor isn't
       * editable
       */
      btk_text_view_reset_im_context (text_view);
      btk_text_view_commit_text (text_view, "\n");

      obscure = TRUE;
      retval = TRUE;
    }
  /* Pass through Tab as literal tab, unless Control is held down */
  else if ((event->keyval == BDK_Tab ||
            event->keyval == BDK_KP_Tab ||
            event->keyval == BDK_ISO_Left_Tab) &&
           !(event->state & BDK_CONTROL_MASK))
    {
      /* If the text widget isn't editable overall, or if the application
       * has turned off "accepts_tab", move the focus instead
       */
      if (text_view->accepts_tab && text_view->editable)
	{
	  btk_text_view_reset_im_context (text_view);
	  btk_text_view_commit_text (text_view, "\t");
	  obscure = TRUE;
	}
      else
	g_signal_emit_by_name (text_view, "move-focus",
                               (event->state & BDK_SHIFT_MASK) ?
                               BTK_DIR_TAB_BACKWARD : BTK_DIR_TAB_FORWARD);

      retval = TRUE;
    }
  else
    retval = FALSE;

  if (obscure)
    btk_text_view_obscure_mouse_cursor (text_view);
  
  btk_text_view_reset_blink_time (text_view);
  btk_text_view_pend_cursor_blink (text_view);

  return retval;
}

static gint
btk_text_view_key_release_event (BtkWidget *widget, BdkEventKey *event)
{
  BtkTextView *text_view = BTK_TEXT_VIEW (widget);
  BtkTextMark *insert;
  BtkTextIter iter;

  if (text_view->layout == NULL || get_buffer (text_view) == NULL)
    return FALSE;
      
  insert = btk_text_buffer_get_insert (get_buffer (text_view));
  btk_text_buffer_get_iter_at_mark (get_buffer (text_view), &iter, insert);
  if (btk_text_iter_can_insert (&iter, text_view->editable) &&
      btk_im_context_filter_keypress (text_view->im_context, event))
    {
      text_view->need_im_reset = TRUE;
      return TRUE;
    }
  else
    return BTK_WIDGET_CLASS (btk_text_view_parent_class)->key_release_event (widget, event);
}

static gint
btk_text_view_button_press_event (BtkWidget *widget, BdkEventButton *event)
{
  BtkTextView *text_view;

  text_view = BTK_TEXT_VIEW (widget);

  btk_widget_grab_focus (widget);

  if (event->window != text_view->text_window->bin_window)
    {
      /* Remove selection if any. */
      btk_text_view_unselect (text_view);
      return FALSE;
    }

  btk_text_view_reset_blink_time (text_view);

#if 0
  /* debug hack */
  if (event->button == 3 && (event->state & BDK_CONTROL_MASK) != 0)
    _btk_text_buffer_spew (BTK_TEXT_VIEW (widget)->buffer);
  else if (event->button == 3)
    btk_text_layout_spew (BTK_TEXT_VIEW (widget)->layout);
#endif

  if (event->type == BDK_BUTTON_PRESS)
    {
      btk_text_view_reset_im_context (text_view);

      if (_btk_button_event_triggers_context_menu (event))
        {
	  btk_text_view_do_popup (text_view, event);
	  return TRUE;
        }
      else if (event->button == 1)
        {
          /* If we're in the selection, start a drag copy/move of the
           * selection; otherwise, start creating a new selection.
           */
          BtkTextIter iter;
          BtkTextIter start, end;

          btk_text_layout_get_iter_at_pixel (text_view->layout,
                                             &iter,
                                             event->x + text_view->xoffset,
                                             event->y + text_view->yoffset);

          if (btk_text_buffer_get_selection_bounds (get_buffer (text_view),
                                                    &start, &end) &&
              btk_text_iter_in_range (&iter, &start, &end) &&
              !(event->state & BTK_EXTEND_SELECTION_MOD_MASK))
            {
              text_view->drag_start_x = event->x;
              text_view->drag_start_y = event->y;
              text_view->pending_place_cursor_button = event->button;
            }
          else
            {
              btk_text_view_start_selection_drag (text_view, &iter, event);
            }

          return TRUE;
        }
      else if (event->button == 2)
        {
          BtkTextIter iter;
          BtkTextViewPrivate *priv;

          /* We do not want to scroll back to the insert iter when we paste
             with the middle button */
          priv = BTK_TEXT_VIEW_GET_PRIVATE (text_view);
          priv->scroll_after_paste = FALSE;

          btk_text_layout_get_iter_at_pixel (text_view->layout,
                                             &iter,
                                             event->x + text_view->xoffset,
                                             event->y + text_view->yoffset);

          btk_text_buffer_paste_clipboard (get_buffer (text_view),
					   btk_widget_get_clipboard (widget, BDK_SELECTION_PRIMARY),
					   &iter,
					   text_view->editable);
          return TRUE;
        }
    }
  else if ((event->type == BDK_2BUTTON_PRESS ||
	    event->type == BDK_3BUTTON_PRESS) &&
	   event->button == 1) 
    {
      BtkTextIter iter;

      btk_text_view_end_selection_drag (text_view);

      btk_text_layout_get_iter_at_pixel (text_view->layout,
					 &iter,
					 event->x + text_view->xoffset,
					 event->y + text_view->yoffset);
      
      btk_text_view_start_selection_drag (text_view, &iter, event);
      return TRUE;
    }
  
  return FALSE;
}

static gint
btk_text_view_button_release_event (BtkWidget *widget, BdkEventButton *event)
{
  BtkTextView *text_view;

  text_view = BTK_TEXT_VIEW (widget);

  if (event->window != text_view->text_window->bin_window)
    return FALSE;

  if (event->button == 1)
    {
      if (text_view->drag_start_x >= 0)
        {
          text_view->drag_start_x = -1;
          text_view->drag_start_y = -1;
        }

      if (btk_text_view_end_selection_drag (BTK_TEXT_VIEW (widget)))
        return TRUE;
      else if (text_view->pending_place_cursor_button == event->button)
        {
	  BtkTextIter iter;

          /* Unselect everything; we clicked inside selection, but
           * didn't move by the drag threshold, so just clear selection
           * and place cursor.
           */
	  btk_text_layout_get_iter_at_pixel (text_view->layout,
					     &iter,
					     event->x + text_view->xoffset,
					     event->y + text_view->yoffset);

	  btk_text_buffer_place_cursor (get_buffer (text_view), &iter);
	  btk_text_view_check_cursor_blink (text_view);
	  
          text_view->pending_place_cursor_button = 0;
          
          return FALSE;
        }
    }

  return FALSE;
}

static void
keymap_direction_changed (BdkKeymap   *keymap,
			  BtkTextView *text_view)
{
  btk_text_view_check_keymap_direction (text_view);
}

static gint
btk_text_view_focus_in_event (BtkWidget *widget, BdkEventFocus *event)
{
  BtkTextView *text_view = BTK_TEXT_VIEW (widget);

  btk_widget_queue_draw (widget);

  DV(g_print (B_STRLOC": focus_in_event\n"));

  btk_text_view_reset_blink_time (text_view);

  if (text_view->cursor_visible && text_view->layout)
    {
      btk_text_layout_set_cursor_visible (text_view->layout, TRUE);
      btk_text_view_check_cursor_blink (text_view);
    }

  g_signal_connect (bdk_keymap_get_for_display (btk_widget_get_display (widget)),
		    "direction-changed",
		    G_CALLBACK (keymap_direction_changed), text_view);
  btk_text_view_check_keymap_direction (text_view);

  if (text_view->editable)
    {
      text_view->need_im_reset = TRUE;
      btk_im_context_focus_in (BTK_TEXT_VIEW (widget)->im_context);
    }

  return FALSE;
}

static gint
btk_text_view_focus_out_event (BtkWidget *widget, BdkEventFocus *event)
{
  BtkTextView *text_view = BTK_TEXT_VIEW (widget);

  btk_text_view_end_selection_drag (text_view);

  btk_widget_queue_draw (widget);

  DV(g_print (B_STRLOC": focus_out_event\n"));
  
  if (text_view->cursor_visible && text_view->layout)
    {
      btk_text_view_check_cursor_blink (text_view);
      btk_text_layout_set_cursor_visible (text_view->layout, FALSE);
    }

  g_signal_handlers_disconnect_by_func (bdk_keymap_get_for_display (btk_widget_get_display (widget)),
					keymap_direction_changed,
					text_view);

  if (text_view->editable)
    {
      text_view->need_im_reset = TRUE;
      btk_im_context_focus_out (BTK_TEXT_VIEW (widget)->im_context);
    }

  return FALSE;
}

static gint
btk_text_view_motion_event (BtkWidget *widget, BdkEventMotion *event)
{
  BtkTextView *text_view = BTK_TEXT_VIEW (widget);

  btk_text_view_unobscure_mouse_cursor (text_view);

  if (event->window == text_view->text_window->bin_window &&
      text_view->drag_start_x >= 0)
    {
      gint x = event->x;
      gint y = event->y;

      bdk_event_request_motions (event);

      if (btk_drag_check_threshold (widget,
				    text_view->drag_start_x, 
				    text_view->drag_start_y,
				    x, y))
        {
          BtkTextIter iter;
          gint buffer_x, buffer_y;

          btk_text_view_window_to_buffer_coords (text_view,
                                                 BTK_TEXT_WINDOW_TEXT,
                                                 text_view->drag_start_x,
                                                 text_view->drag_start_y,
                                                 &buffer_x,
                                                 &buffer_y);

          btk_text_layout_get_iter_at_pixel (text_view->layout,
                                             &iter,
                                             buffer_x, buffer_y);

          btk_text_view_start_selection_dnd (text_view, &iter, event);
          return TRUE;
        }
    }

  return FALSE;
}

static void
btk_text_view_paint (BtkWidget      *widget,
                     BdkRectangle   *area,
                     BdkEventExpose *event)
{
  BtkTextView *text_view;
  GList *child_exposes;
  GList *tmp_list;
  
  text_view = BTK_TEXT_VIEW (widget);

  g_return_if_fail (text_view->layout != NULL);
  g_return_if_fail (text_view->xoffset >= 0);
  g_return_if_fail (text_view->yoffset >= 0);

  while (text_view->first_validate_idle != 0)
    {
      DV (g_print (B_STRLOC": first_validate_idle: %d\n",
                   text_view->first_validate_idle));
      btk_text_view_flush_first_validate (text_view);
    }

  if (!text_view->onscreen_validated)
    {
      g_warning (B_STRLOC ": somehow some text lines were modified or scrolling occurred since the last validation of lines on the screen - may be a text widget bug.");
      g_assert_not_reached ();
    }
  
#if 0
  printf ("painting %d,%d  %d x %d\n",
          area->x, area->y,
          area->width, area->height);
#endif

  child_exposes = NULL;
  btk_text_layout_draw (text_view->layout,
                        widget,
                        text_view->text_window->bin_window,
			NULL,
                        text_view->xoffset,
                        text_view->yoffset,
                        area->x, area->y,
                        area->width, area->height,
                        &child_exposes);

  tmp_list = child_exposes;
  while (tmp_list != NULL)
    {
      BtkWidget *child = tmp_list->data;
  
      btk_container_propagate_expose (BTK_CONTAINER (text_view),
                                      child,
                                      event);

      g_object_unref (child);
      
      tmp_list = tmp_list->next;
    }

  g_list_free (child_exposes);
}

static gint
btk_text_view_expose_event (BtkWidget *widget, BdkEventExpose *event)
{
  GSList *tmp_list;
  
  if (event->window == btk_text_view_get_window (BTK_TEXT_VIEW (widget),
                                                 BTK_TEXT_WINDOW_TEXT))
    {
      DV(g_print (">Exposed ("B_STRLOC")\n"));
      btk_text_view_paint (widget, &event->area, event);
    }

  if (event->window == widget->window)
    btk_text_view_draw_focus (widget);

  /* Propagate exposes to all unanchored children. 
   * Anchored children are handled in btk_text_view_paint(). 
   */
  tmp_list = BTK_TEXT_VIEW (widget)->children;
  while (tmp_list != NULL)
    {
      BtkTextViewChild *vc = tmp_list->data;

      /* propagate_expose checks that event->window matches
       * child->window
       */
      if (!vc->anchor)
        btk_container_propagate_expose (BTK_CONTAINER (widget),
                                        vc->widget,
                                        event);
      
      tmp_list = tmp_list->next;
    }
  
  return FALSE;
}

static void
btk_text_view_draw_focus (BtkWidget *widget)
{
  gboolean interior_focus;

  /* We clear the focus if we are in interior focus mode. */
  btk_widget_style_get (widget,
			"interior-focus", &interior_focus,
			NULL);
  
  if (btk_widget_is_drawable (widget))
    {
      if (btk_widget_has_focus (widget) && !interior_focus)
        {          
          btk_paint_focus (widget->style, widget->window, btk_widget_get_state (widget),
                           NULL, widget, "textview",
                           0, 0,
                           widget->allocation.width,
                           widget->allocation.height);
        }
      else
        {
          bdk_window_clear (widget->window);
        }
    }
}

static gboolean
btk_text_view_focus (BtkWidget        *widget,
                     BtkDirectionType  direction)
{
  BtkContainer *container;
  gboolean result;
  
  container = BTK_CONTAINER (widget);  

  if (!btk_widget_is_focus (widget) &&
      container->focus_child == NULL)
    {
      btk_widget_grab_focus (widget);
      return TRUE;
    }
  else
    {
      /*
       * Unset CAN_FOCUS flag so that btk_container_focus() allows
       * children to get the focus
       */
      btk_widget_set_can_focus (widget, FALSE);
      result = BTK_WIDGET_CLASS (btk_text_view_parent_class)->focus (widget, direction);
      btk_widget_set_can_focus (widget, TRUE);

      return result;
    }
}

static void
btk_text_view_move_focus (BtkWidget        *widget,
                          BtkDirectionType  direction_type)
{
  BtkTextView *text_view = BTK_TEXT_VIEW (widget);

  if (BTK_TEXT_VIEW_GET_CLASS (text_view)->move_focus)
    BTK_TEXT_VIEW_GET_CLASS (text_view)->move_focus (text_view,
                                                     direction_type);
}

/*
 * Container
 */

static void
btk_text_view_add (BtkContainer *container,
                   BtkWidget    *child)
{
  /* This is pretty random. */
  btk_text_view_add_child_in_window (BTK_TEXT_VIEW (container),
                                     child,
                                     BTK_TEXT_WINDOW_WIDGET,
                                     0, 0);
}

static void
btk_text_view_remove (BtkContainer *container,
                      BtkWidget    *child)
{
  GSList *iter;
  BtkTextView *text_view;
  BtkTextViewChild *vc;

  text_view = BTK_TEXT_VIEW (container);

  vc = NULL;
  iter = text_view->children;

  while (iter != NULL)
    {
      vc = iter->data;

      if (vc->widget == child)
        break;

      iter = g_slist_next (iter);
    }

  g_assert (iter != NULL); /* be sure we had the child in the list */

  text_view->children = g_slist_remove (text_view->children, vc);

  btk_widget_unparent (vc->widget);

  text_view_child_free (vc);
}

static void
btk_text_view_forall (BtkContainer *container,
                      gboolean      include_internals,
                      BtkCallback   callback,
                      gpointer      callback_data)
{
  GSList *iter;
  BtkTextView *text_view;
  GSList *copy;

  g_return_if_fail (BTK_IS_TEXT_VIEW (container));
  g_return_if_fail (callback != NULL);

  text_view = BTK_TEXT_VIEW (container);

  copy = g_slist_copy (text_view->children);
  iter = copy;

  while (iter != NULL)
    {
      BtkTextViewChild *vc = iter->data;

      (* callback) (vc->widget, callback_data);

      iter = g_slist_next (iter);
    }

  g_slist_free (copy);
}

#define CURSOR_ON_MULTIPLIER 2
#define CURSOR_OFF_MULTIPLIER 1
#define CURSOR_PEND_MULTIPLIER 3
#define CURSOR_DIVIDER 3

static gboolean
cursor_blinks (BtkTextView *text_view)
{
  BtkSettings *settings = btk_widget_get_settings (BTK_WIDGET (text_view));
  gboolean blink;

#ifdef DEBUG_VALIDATION_AND_SCROLLING
  return FALSE;
#endif
  if (btk_debug_flags & BTK_DEBUG_UPDATES)
    return FALSE;

  g_object_get (settings, "btk-cursor-blink", &blink, NULL);

  if (!blink)
    return FALSE;

  if (text_view->editable)
    {
      BtkTextMark *insert;
      BtkTextIter iter;
      
      insert = btk_text_buffer_get_insert (get_buffer (text_view));
      btk_text_buffer_get_iter_at_mark (get_buffer (text_view), &iter, insert);
      
      if (btk_text_iter_editable (&iter, text_view->editable))
	return blink;
    }

  return FALSE;
}

static gint
get_cursor_time (BtkTextView *text_view)
{
  BtkSettings *settings = btk_widget_get_settings (BTK_WIDGET (text_view));
  gint time;

  g_object_get (settings, "btk-cursor-blink-time", &time, NULL);

  return time;
}

static gint
get_cursor_blink_timeout (BtkTextView *text_view)
{
  BtkSettings *settings = btk_widget_get_settings (BTK_WIDGET (text_view));
  gint time;

  g_object_get (settings, "btk-cursor-blink-timeout", &time, NULL);

  return time;
}


/*
 * Blink!
 */

static gint
blink_cb (gpointer data)
{
  BtkTextView *text_view;
  BtkTextViewPrivate *priv;
  gboolean visible;
  gint blink_timeout;

  text_view = BTK_TEXT_VIEW (data);
  priv = BTK_TEXT_VIEW_GET_PRIVATE (text_view);

  if (!btk_widget_has_focus (BTK_WIDGET (text_view)))
    {
      g_warning ("BtkTextView - did not receive focus-out-event. If you\n"
                 "connect a handler to this signal, it must return\n"
                 "FALSE so the text view gets the event as well");

      btk_text_view_check_cursor_blink (text_view);

      return FALSE;
    }

  g_assert (text_view->layout);
  g_assert (text_view->cursor_visible);

  visible = btk_text_layout_get_cursor_visible (text_view->layout);

  blink_timeout = get_cursor_blink_timeout (text_view);
  if (priv->blink_time > 1000 * blink_timeout &&
      blink_timeout < G_MAXINT/1000) 
    {
      /* we've blinked enough without the user doing anything, stop blinking */
      visible = 0;
      text_view->blink_timeout = 0;
    } 
  else if (visible)
    text_view->blink_timeout = bdk_threads_add_timeout (get_cursor_time (text_view) * CURSOR_OFF_MULTIPLIER / CURSOR_DIVIDER,
					      blink_cb,
					      text_view);
  else 
    {
      text_view->blink_timeout = bdk_threads_add_timeout (get_cursor_time (text_view) * CURSOR_ON_MULTIPLIER / CURSOR_DIVIDER,
						blink_cb,
						text_view);
      priv->blink_time += get_cursor_time (text_view);
    }

  /* Block changed_handler while changing the layout's cursor visibility
   * because it would expose the whole paragraph. Instead, we expose
   * the cursor's area(s) manually below.
   */
  g_signal_handlers_block_by_func (text_view->layout,
                                   changed_handler,
                                   text_view);
  btk_text_layout_set_cursor_visible (text_view->layout, !visible);
  g_signal_handlers_unblock_by_func (text_view->layout,
                                     changed_handler,
                                     text_view);

  text_window_invalidate_cursors (text_view->text_window);

  /* Remove ourselves */
  return FALSE;
}


static void
btk_text_view_stop_cursor_blink (BtkTextView *text_view)
{
  if (text_view->blink_timeout)  
    { 
      g_source_remove (text_view->blink_timeout);
      text_view->blink_timeout = 0;
    }
}

static void
btk_text_view_check_cursor_blink (BtkTextView *text_view)
{
  if (text_view->layout != NULL &&
      text_view->cursor_visible &&
      btk_widget_has_focus (BTK_WIDGET (text_view)))
    {
      if (cursor_blinks (text_view))
	{
	  if (text_view->blink_timeout == 0)
	    {
	      btk_text_layout_set_cursor_visible (text_view->layout, TRUE);
	      
	      text_view->blink_timeout = bdk_threads_add_timeout (get_cursor_time (text_view) * CURSOR_OFF_MULTIPLIER / CURSOR_DIVIDER,
							blink_cb,
							text_view);
	    }
	}
      else
	{
	  btk_text_view_stop_cursor_blink (text_view);
	  btk_text_layout_set_cursor_visible (text_view->layout, TRUE);
	}
    }
  else
    {
      btk_text_view_stop_cursor_blink (text_view);
      btk_text_layout_set_cursor_visible (text_view->layout, FALSE);
    }
}

static void
btk_text_view_pend_cursor_blink (BtkTextView *text_view)
{
  if (text_view->layout != NULL &&
      text_view->cursor_visible &&
      btk_widget_has_focus (BTK_WIDGET (text_view)) &&
      cursor_blinks (text_view))
    {
      btk_text_view_stop_cursor_blink (text_view);
      btk_text_layout_set_cursor_visible (text_view->layout, TRUE);
      
      text_view->blink_timeout = bdk_threads_add_timeout (get_cursor_time (text_view) * CURSOR_PEND_MULTIPLIER / CURSOR_DIVIDER,
						blink_cb,
						text_view);
    }
}

static void
btk_text_view_reset_blink_time (BtkTextView *text_view)
{
  BtkTextViewPrivate *priv;

  priv = BTK_TEXT_VIEW_GET_PRIVATE (text_view);

  priv->blink_time = 0;
}


/*
 * Key binding handlers
 */

static gboolean
btk_text_view_move_iter_by_lines (BtkTextView *text_view,
                                  BtkTextIter *newplace,
                                  gint         count)
{
  gboolean ret = TRUE;

  while (count < 0)
    {
      ret = btk_text_layout_move_iter_to_previous_line (text_view->layout, newplace);
      count++;
    }

  while (count > 0)
    {
      ret = btk_text_layout_move_iter_to_next_line (text_view->layout, newplace);
      count--;
    }

  return ret;
}

static void
move_cursor (BtkTextView       *text_view,
             const BtkTextIter *new_location,
             gboolean           extend_selection)
{
  if (extend_selection)
    btk_text_buffer_move_mark_by_name (get_buffer (text_view),
                                       "insert",
                                       new_location);
  else
      btk_text_buffer_place_cursor (get_buffer (text_view),
				    new_location);
  btk_text_view_check_cursor_blink (text_view);
}

static gboolean
iter_line_is_rtl (const BtkTextIter *iter)
{
  BtkTextIter start, end;
  char *text;
  BangoDirection direction;

  start = end = *iter;
  btk_text_iter_set_line_offset (&start, 0);
  btk_text_iter_forward_line (&end);
  text = btk_text_iter_get_visible_text (&start, &end);
  direction = bango_find_base_dir (text, -1);

  g_free (text);

  return direction == BANGO_DIRECTION_RTL;
}

static void
btk_text_view_move_cursor_internal (BtkTextView     *text_view,
                                    BtkMovementStep  step,
                                    gint             count,
                                    gboolean         extend_selection)
{
  BtkTextIter insert;
  BtkTextIter newplace;
  gboolean cancel_selection = FALSE;
  gint cursor_x_pos = 0;
  BtkDirectionType leave_direction = -1;

  if (!text_view->cursor_visible) 
    {
      BtkScrollStep scroll_step;

      switch (step) 
	{
        case BTK_MOVEMENT_VISUAL_POSITIONS:
          leave_direction = count > 0 ? BTK_DIR_RIGHT : BTK_DIR_LEFT;
          /* fall through */
        case BTK_MOVEMENT_LOGICAL_POSITIONS:
        case BTK_MOVEMENT_WORDS:
	  scroll_step = BTK_SCROLL_HORIZONTAL_STEPS;
	  break;
        case BTK_MOVEMENT_DISPLAY_LINE_ENDS:
	  scroll_step = BTK_SCROLL_HORIZONTAL_ENDS;
	  break;	  
        case BTK_MOVEMENT_DISPLAY_LINES:
          leave_direction = count > 0 ? BTK_DIR_DOWN : BTK_DIR_UP;
          /* fall through */
        case BTK_MOVEMENT_PARAGRAPHS:
        case BTK_MOVEMENT_PARAGRAPH_ENDS:
	  scroll_step = BTK_SCROLL_STEPS;
	  break;
	case BTK_MOVEMENT_PAGES:
	  scroll_step = BTK_SCROLL_PAGES;
	  break;
	case BTK_MOVEMENT_HORIZONTAL_PAGES:
	  scroll_step = BTK_SCROLL_HORIZONTAL_PAGES;
	  break;
	case BTK_MOVEMENT_BUFFER_ENDS:
	  scroll_step = BTK_SCROLL_ENDS;
	  break;
	default:
          scroll_step = BTK_SCROLL_PAGES;
          break;
	}

      if (!btk_text_view_move_viewport (text_view, scroll_step, count))
        {
          if (leave_direction != -1 &&
              !btk_widget_keynav_failed (BTK_WIDGET (text_view),
                                         leave_direction))
            {
              g_signal_emit_by_name (text_view, "move-focus", leave_direction);
            }
        }

      return;
    }

  btk_text_view_reset_im_context (text_view);

  if (step == BTK_MOVEMENT_PAGES)
    {
      if (!btk_text_view_scroll_pages (text_view, count, extend_selection))
        btk_widget_error_bell (BTK_WIDGET (text_view));

      btk_text_view_check_cursor_blink (text_view);
      btk_text_view_pend_cursor_blink (text_view);
      return;
    }
  else if (step == BTK_MOVEMENT_HORIZONTAL_PAGES)
    {
      if (!btk_text_view_scroll_hpages (text_view, count, extend_selection))
        btk_widget_error_bell (BTK_WIDGET (text_view));

      btk_text_view_check_cursor_blink (text_view);
      btk_text_view_pend_cursor_blink (text_view);
      return;
    }

  btk_text_buffer_get_iter_at_mark (get_buffer (text_view), &insert,
                                    btk_text_buffer_get_insert (get_buffer (text_view)));

  if (! extend_selection)
    {
      gboolean move_forward = count > 0;
      BtkTextIter sel_bound;

      btk_text_buffer_get_iter_at_mark (get_buffer (text_view), &sel_bound,
                                        btk_text_buffer_get_selection_bound (get_buffer (text_view)));

      if (iter_line_is_rtl (&insert))
        move_forward = !move_forward;

      /* if we move forward, assume the cursor is at the end of the selection;
       * if we move backward, assume the cursor is at the start
       */
      if (move_forward)
        btk_text_iter_order (&sel_bound, &insert);
      else
        btk_text_iter_order (&insert, &sel_bound);

      /* if we actually have a selection, just move *to* the beginning/end
       * of the selection and not *from* there on LOGICAL_POSITIONS
       * and VISUAL_POSITIONS movement
       */
      if (! btk_text_iter_equal (&sel_bound, &insert))
        cancel_selection = TRUE;
    }

  newplace = insert;

  if (step == BTK_MOVEMENT_DISPLAY_LINES)
    btk_text_view_get_virtual_cursor_pos (text_view, &insert, &cursor_x_pos, NULL);

  switch (step)
    {
    case BTK_MOVEMENT_LOGICAL_POSITIONS:
      if (! cancel_selection)
        btk_text_iter_forward_visible_cursor_positions (&newplace, count);
      break;

    case BTK_MOVEMENT_VISUAL_POSITIONS:
      if (! cancel_selection)
        btk_text_layout_move_iter_visually (text_view->layout,
                                            &newplace, count);
      break;

    case BTK_MOVEMENT_WORDS:
      if (iter_line_is_rtl (&newplace))
        count *= -1;

      if (count < 0)
        btk_text_iter_backward_visible_word_starts (&newplace, -count);
      else if (count > 0) 
	{
	  if (!btk_text_iter_forward_visible_word_ends (&newplace, count))
	    btk_text_iter_forward_to_line_end (&newplace);
	}
      break;

    case BTK_MOVEMENT_DISPLAY_LINES:
      if (count < 0)
        {
          leave_direction = BTK_DIR_UP;

          if (btk_text_view_move_iter_by_lines (text_view, &newplace, count))
            btk_text_layout_move_iter_to_x (text_view->layout, &newplace, cursor_x_pos);
          else
            btk_text_iter_set_line_offset (&newplace, 0);
        }
      if (count > 0)
        {
          leave_direction = BTK_DIR_DOWN;

          if (btk_text_view_move_iter_by_lines (text_view, &newplace, count))
            btk_text_layout_move_iter_to_x (text_view->layout, &newplace, cursor_x_pos);
          else
            btk_text_iter_forward_to_line_end (&newplace);
        }
      break;

    case BTK_MOVEMENT_DISPLAY_LINE_ENDS:
      if (count > 1)
        btk_text_view_move_iter_by_lines (text_view, &newplace, --count);
      else if (count < -1)
        btk_text_view_move_iter_by_lines (text_view, &newplace, ++count);

      if (count != 0)
        btk_text_layout_move_iter_to_line_end (text_view->layout, &newplace, count);
      break;

    case BTK_MOVEMENT_PARAGRAPHS:
      if (count > 0)
        {
          if (!btk_text_iter_ends_line (&newplace))
            {
              btk_text_iter_forward_to_line_end (&newplace);
              --count;
            }
          btk_text_iter_forward_visible_lines (&newplace, count);
          btk_text_iter_forward_to_line_end (&newplace);
        }
      else if (count < 0)
        {
          if (btk_text_iter_get_line_offset (&newplace) > 0)
	    btk_text_iter_set_line_offset (&newplace, 0);
          btk_text_iter_forward_visible_lines (&newplace, count);
          btk_text_iter_set_line_offset (&newplace, 0);
        }
      break;

    case BTK_MOVEMENT_PARAGRAPH_ENDS:
      if (count > 0)
        {
          if (!btk_text_iter_ends_line (&newplace))
            btk_text_iter_forward_to_line_end (&newplace);
        }
      else if (count < 0)
        {
          btk_text_iter_set_line_offset (&newplace, 0);
        }
      break;

    case BTK_MOVEMENT_BUFFER_ENDS:
      if (count > 0)
        btk_text_buffer_get_end_iter (get_buffer (text_view), &newplace);
      else if (count < 0)
        btk_text_buffer_get_iter_at_offset (get_buffer (text_view), &newplace, 0);
     break;
      
    default:
      break;
    }

  /* call move_cursor() even if the cursor hasn't moved, since it 
     cancels the selection
  */
  move_cursor (text_view, &newplace, extend_selection);

  if (!btk_text_iter_equal (&insert, &newplace))
    {
      DV(g_print (B_STRLOC": scrolling onscreen\n"));
      btk_text_view_scroll_mark_onscreen (text_view,
                                          btk_text_buffer_get_insert (get_buffer (text_view)));

      if (step == BTK_MOVEMENT_DISPLAY_LINES)
        btk_text_view_set_virtual_cursor_pos (text_view, cursor_x_pos, -1);
    }
  else if (leave_direction != -1)
    {
      if (!btk_widget_keynav_failed (BTK_WIDGET (text_view),
                                     leave_direction))
        {
          g_signal_emit_by_name (text_view, "move-focus", leave_direction);
        }
    }
  else if (! cancel_selection)
    {
      btk_widget_error_bell (BTK_WIDGET (text_view));
    }

  btk_text_view_check_cursor_blink (text_view);
  btk_text_view_pend_cursor_blink (text_view);
}

static void
btk_text_view_move_cursor (BtkTextView     *text_view,
                           BtkMovementStep  step,
                           gint             count,
                           gboolean         extend_selection)
{
  btk_text_view_move_cursor_internal (text_view, step, count, extend_selection);
}

static void
btk_text_view_page_horizontally (BtkTextView     *text_view,
                                 gint             count,
                                 gboolean         extend_selection)
{
  btk_text_view_move_cursor_internal (text_view, BTK_MOVEMENT_HORIZONTAL_PAGES,
                                      count, extend_selection);
}


static gboolean
btk_text_view_move_viewport (BtkTextView     *text_view,
                             BtkScrollStep    step,
                             gint             count)
{
  BtkAdjustment *adjustment;
  gdouble increment;
  
  switch (step) 
    {
    case BTK_SCROLL_STEPS:
    case BTK_SCROLL_PAGES:
    case BTK_SCROLL_ENDS:
      adjustment = get_vadjustment (text_view);
      break;
    case BTK_SCROLL_HORIZONTAL_STEPS:
    case BTK_SCROLL_HORIZONTAL_PAGES:
    case BTK_SCROLL_HORIZONTAL_ENDS:
      adjustment = get_hadjustment (text_view);
      break;
    default:
      adjustment = get_vadjustment (text_view);
      break;
    }

  switch (step) 
    {
    case BTK_SCROLL_STEPS:
    case BTK_SCROLL_HORIZONTAL_STEPS:
      increment = adjustment->step_increment;
      break;
    case BTK_SCROLL_PAGES:
    case BTK_SCROLL_HORIZONTAL_PAGES:
      increment = adjustment->page_increment;
      break;
    case BTK_SCROLL_ENDS:
    case BTK_SCROLL_HORIZONTAL_ENDS:
      increment = adjustment->upper - adjustment->lower;
      break;
    default:
      increment = 0.0;
      break;
    }

  return set_adjustment_clamped (adjustment, adjustment->value + count * increment);
}

static void
btk_text_view_set_anchor (BtkTextView *text_view)
{
  BtkTextIter insert;

  btk_text_buffer_get_iter_at_mark (get_buffer (text_view), &insert,
                                    btk_text_buffer_get_insert (get_buffer (text_view)));

  btk_text_buffer_create_mark (get_buffer (text_view), "anchor", &insert, TRUE);
}

static gboolean
btk_text_view_scroll_pages (BtkTextView *text_view,
                            gint         count,
                            gboolean     extend_selection)
{
  gdouble newval;
  gdouble oldval;
  BtkAdjustment *adj;
  gint cursor_x_pos, cursor_y_pos;
  BtkTextMark *insert_mark;
  BtkTextIter old_insert;
  BtkTextIter new_insert;
  BtkTextIter anchor;
  gint y0, y1;

  g_return_val_if_fail (text_view->vadjustment != NULL, FALSE);
  
  adj = text_view->vadjustment;

  insert_mark = btk_text_buffer_get_insert (get_buffer (text_view));

  /* Make sure we start from the current cursor position, even
   * if it was offscreen, but don't queue more scrolls if we're
   * already behind.
   */
  if (text_view->pending_scroll)
    cancel_pending_scroll (text_view);
  else
    btk_text_view_scroll_mark_onscreen (text_view, insert_mark);

  /* Validate the rebunnyion that will be brought into view by the cursor motion
   */
  btk_text_buffer_get_iter_at_mark (get_buffer (text_view),
                                    &old_insert, insert_mark);

  if (count < 0)
    {
      btk_text_view_get_first_para_iter (text_view, &anchor);
      y0 = adj->page_size;
      y1 = adj->page_size + count * adj->page_increment;
    }
  else
    {
      btk_text_view_get_first_para_iter (text_view, &anchor);
      y0 = count * adj->page_increment + adj->page_size;
      y1 = 0;
    }

  btk_text_layout_validate_yrange (text_view->layout, &anchor, y0, y1);
  /* FIXME do we need to update the adjustment ranges here? */

  new_insert = old_insert;

  if (count < 0 && adj->value <= (adj->lower + 1e-12))
    {
      /* already at top, just be sure we are at offset 0 */
      btk_text_buffer_get_start_iter (get_buffer (text_view), &new_insert);
      move_cursor (text_view, &new_insert, extend_selection);
    }
  else if (count > 0 && adj->value >= (adj->upper - adj->page_size - 1e-12))
    {
      /* already at bottom, just be sure we are at the end */
      btk_text_buffer_get_end_iter (get_buffer (text_view), &new_insert);
      move_cursor (text_view, &new_insert, extend_selection);
    }
  else
    {
      btk_text_view_get_virtual_cursor_pos (text_view, NULL, &cursor_x_pos, &cursor_y_pos);

      oldval = adj->value;
      newval = adj->value;

      newval += count * adj->page_increment;

      set_adjustment_clamped (adj, newval);
      cursor_y_pos += adj->value - oldval;

      btk_text_layout_get_iter_at_pixel (text_view->layout, &new_insert, cursor_x_pos, cursor_y_pos);
      clamp_iter_onscreen (text_view, &new_insert);
      move_cursor (text_view, &new_insert, extend_selection);

      btk_text_view_set_virtual_cursor_pos (text_view, cursor_x_pos, cursor_y_pos);
    }
  
  /* Adjust to have the cursor _entirely_ onscreen, move_mark_onscreen
   * only guarantees 1 pixel onscreen.
   */
  DV(g_print (B_STRLOC": scrolling onscreen\n"));
  btk_text_view_scroll_mark_onscreen (text_view, insert_mark);

  return !btk_text_iter_equal (&old_insert, &new_insert);
}

static gboolean
btk_text_view_scroll_hpages (BtkTextView *text_view,
                             gint         count,
                             gboolean     extend_selection)
{
  gdouble newval;
  gdouble oldval;
  BtkAdjustment *adj;
  gint cursor_x_pos, cursor_y_pos;
  BtkTextMark *insert_mark;
  BtkTextIter old_insert;
  BtkTextIter new_insert;
  gint y, height;
  
  g_return_val_if_fail (text_view->hadjustment != NULL, FALSE);

  adj = text_view->hadjustment;

  insert_mark = btk_text_buffer_get_insert (get_buffer (text_view));

  /* Make sure we start from the current cursor position, even
   * if it was offscreen, but don't queue more scrolls if we're
   * already behind.
   */
  if (text_view->pending_scroll)
    cancel_pending_scroll (text_view);
  else
    btk_text_view_scroll_mark_onscreen (text_view, insert_mark);

  /* Validate the line that we're moving within.
   */
  btk_text_buffer_get_iter_at_mark (get_buffer (text_view),
                                    &old_insert, insert_mark);

  btk_text_layout_get_line_yrange (text_view->layout, &old_insert, &y, &height);
  btk_text_layout_validate_yrange (text_view->layout, &old_insert, y, y + height);
  /* FIXME do we need to update the adjustment ranges here? */

  new_insert = old_insert;

  if (count < 0 && adj->value <= (adj->lower + 1e-12))
    {
      /* already at far left, just be sure we are at offset 0 */
      btk_text_iter_set_line_offset (&new_insert, 0);
      move_cursor (text_view, &new_insert, extend_selection);
    }
  else if (count > 0 && adj->value >= (adj->upper - adj->page_size - 1e-12))
    {
      /* already at far right, just be sure we are at the end */
      if (!btk_text_iter_ends_line (&new_insert))
	  btk_text_iter_forward_to_line_end (&new_insert);
      move_cursor (text_view, &new_insert, extend_selection);
    }
  else
    {
      btk_text_view_get_virtual_cursor_pos (text_view, NULL, &cursor_x_pos, &cursor_y_pos);

      oldval = adj->value;
      newval = adj->value;

      newval += count * adj->page_increment;

      set_adjustment_clamped (adj, newval);
      cursor_x_pos += adj->value - oldval;

      btk_text_layout_get_iter_at_pixel (text_view->layout, &new_insert, cursor_x_pos, cursor_y_pos);
      clamp_iter_onscreen (text_view, &new_insert);
      move_cursor (text_view, &new_insert, extend_selection);

      btk_text_view_set_virtual_cursor_pos (text_view, cursor_x_pos, cursor_y_pos);
    }

  /*  FIXME for lines shorter than the overall widget width, this results in a
   *  "bounce" effect as we scroll to the right of the widget, then scroll
   *  back to get the end of the line onscreen.
   *      http://bugzilla.bunny.org/show_bug.cgi?id=68963
   */
  
  /* Adjust to have the cursor _entirely_ onscreen, move_mark_onscreen
   * only guarantees 1 pixel onscreen.
   */
  DV(g_print (B_STRLOC": scrolling onscreen\n"));
  btk_text_view_scroll_mark_onscreen (text_view, insert_mark);

  return !btk_text_iter_equal (&old_insert, &new_insert);
}

static gboolean
whitespace (gunichar ch, gpointer user_data)
{
  return (ch == ' ' || ch == '\t');
}

static gboolean
not_whitespace (gunichar ch, gpointer user_data)
{
  return !whitespace (ch, user_data);
}

static gboolean
find_whitepace_rebunnyion (const BtkTextIter *center,
                       BtkTextIter *start, BtkTextIter *end)
{
  *start = *center;
  *end = *center;

  if (btk_text_iter_backward_find_char (start, not_whitespace, NULL, NULL))
    btk_text_iter_forward_char (start); /* we want the first whitespace... */
  if (whitespace (btk_text_iter_get_char (end), NULL))
    btk_text_iter_forward_find_char (end, not_whitespace, NULL, NULL);

  return !btk_text_iter_equal (start, end);
}

static void
btk_text_view_insert_at_cursor (BtkTextView *text_view,
                                const gchar *str)
{
  if (!btk_text_buffer_insert_interactive_at_cursor (get_buffer (text_view), str, -1,
                                                     text_view->editable))
    {
      btk_widget_error_bell (BTK_WIDGET (text_view));
    }
}

static void
btk_text_view_delete_from_cursor (BtkTextView   *text_view,
                                  BtkDeleteType  type,
                                  gint           count)
{
  BtkTextIter insert;
  BtkTextIter start;
  BtkTextIter end;
  gboolean leave_one = FALSE;

  btk_text_view_reset_im_context (text_view);

  if (type == BTK_DELETE_CHARS)
    {
      /* Char delete deletes the selection, if one exists */
      if (btk_text_buffer_delete_selection (get_buffer (text_view), TRUE,
                                            text_view->editable))
        return;
    }

  btk_text_buffer_get_iter_at_mark (get_buffer (text_view), &insert,
                                    btk_text_buffer_get_insert (get_buffer (text_view)));

  start = insert;
  end = insert;

  switch (type)
    {
    case BTK_DELETE_CHARS:
      btk_text_iter_forward_cursor_positions (&end, count);
      break;

    case BTK_DELETE_WORD_ENDS:
      if (count > 0)
        btk_text_iter_forward_word_ends (&end, count);
      else if (count < 0)
        btk_text_iter_backward_word_starts (&start, 0 - count);
      break;

    case BTK_DELETE_WORDS:
      break;

    case BTK_DELETE_DISPLAY_LINE_ENDS:
      break;

    case BTK_DELETE_DISPLAY_LINES:
      break;

    case BTK_DELETE_PARAGRAPH_ENDS:
      if (count > 0)
        {
          /* If we're already at a newline, we need to
           * simply delete that newline, instead of
           * moving to the next one.
           */
          if (btk_text_iter_ends_line (&end))
            {
              btk_text_iter_forward_line (&end);
              --count;
            }

          while (count > 0)
            {
              if (!btk_text_iter_forward_to_line_end (&end))
                break;

              --count;
            }
        }
      else if (count < 0)
        {
          if (btk_text_iter_starts_line (&start))
            {
              btk_text_iter_backward_line (&start);
              if (!btk_text_iter_ends_line (&end))
                btk_text_iter_forward_to_line_end (&start);
            }
          else
            {
              btk_text_iter_set_line_offset (&start, 0);
            }
          ++count;

          btk_text_iter_backward_lines (&start, -count);
        }
      break;

    case BTK_DELETE_PARAGRAPHS:
      if (count > 0)
        {
          btk_text_iter_set_line_offset (&start, 0);
          btk_text_iter_forward_to_line_end (&end);

          /* Do the lines beyond the first. */
          while (count > 1)
            {
              btk_text_iter_forward_to_line_end (&end);

              --count;
            }
        }

      /* FIXME negative count? */

      break;

    case BTK_DELETE_WHITESPACE:
      {
        find_whitepace_rebunnyion (&insert, &start, &end);
      }
      break;

    default:
      break;
    }

  if (!btk_text_iter_equal (&start, &end))
    {
      btk_text_buffer_begin_user_action (get_buffer (text_view));

      if (btk_text_buffer_delete_interactive (get_buffer (text_view), &start, &end,
                                              text_view->editable))
        {
          if (leave_one)
            btk_text_buffer_insert_interactive_at_cursor (get_buffer (text_view),
                                                          " ", 1,
                                                          text_view->editable);
        }
      else
        {
          btk_widget_error_bell (BTK_WIDGET (text_view));
        }

      btk_text_buffer_end_user_action (get_buffer (text_view));
      btk_text_view_set_virtual_cursor_pos (text_view, -1, -1);

      DV(g_print (B_STRLOC": scrolling onscreen\n"));
      btk_text_view_scroll_mark_onscreen (text_view,
                                          btk_text_buffer_get_insert (get_buffer (text_view)));
    }
  else
    {
      btk_widget_error_bell (BTK_WIDGET (text_view));
    }
}

static void
btk_text_view_backspace (BtkTextView *text_view)
{
  BtkTextIter insert;

  btk_text_view_reset_im_context (text_view);

  /* Backspace deletes the selection, if one exists */
  if (btk_text_buffer_delete_selection (get_buffer (text_view), TRUE,
                                        text_view->editable))
    return;

  btk_text_buffer_get_iter_at_mark (get_buffer (text_view),
                                    &insert,
                                    btk_text_buffer_get_insert (get_buffer (text_view)));

  if (btk_text_buffer_backspace (get_buffer (text_view), &insert,
				 TRUE, text_view->editable))
    {
      btk_text_view_set_virtual_cursor_pos (text_view, -1, -1);
      DV(g_print (B_STRLOC": scrolling onscreen\n"));
      btk_text_view_scroll_mark_onscreen (text_view,
                                          btk_text_buffer_get_insert (get_buffer (text_view)));
    }
  else
    {
      btk_widget_error_bell (BTK_WIDGET (text_view));
    }
}

static void
btk_text_view_cut_clipboard (BtkTextView *text_view)
{
  BtkClipboard *clipboard = btk_widget_get_clipboard (BTK_WIDGET (text_view),
						      BDK_SELECTION_CLIPBOARD);
  
  btk_text_buffer_cut_clipboard (get_buffer (text_view),
				 clipboard,
				 text_view->editable);
  DV(g_print (B_STRLOC": scrolling onscreen\n"));
  btk_text_view_scroll_mark_onscreen (text_view,
                                      btk_text_buffer_get_insert (get_buffer (text_view)));
}

static void
btk_text_view_copy_clipboard (BtkTextView *text_view)
{
  BtkClipboard *clipboard = btk_widget_get_clipboard (BTK_WIDGET (text_view),
						      BDK_SELECTION_CLIPBOARD);
  
  btk_text_buffer_copy_clipboard (get_buffer (text_view),
				  clipboard);

  /* on copy do not scroll, we are already onscreen */
}

static void
btk_text_view_paste_clipboard (BtkTextView *text_view)
{
  BtkClipboard *clipboard = btk_widget_get_clipboard (BTK_WIDGET (text_view),
						      BDK_SELECTION_CLIPBOARD);
  
  btk_text_buffer_paste_clipboard (get_buffer (text_view),
				   clipboard,
				   NULL,
				   text_view->editable);
}

static void
btk_text_view_paste_done_handler (BtkTextBuffer *buffer,
                                  BtkClipboard  *clipboard,
                                  gpointer       data)
{
  BtkTextView *text_view = data;
  BtkTextViewPrivate *priv;

  priv = BTK_TEXT_VIEW_GET_PRIVATE (text_view);

  if (priv->scroll_after_paste)
    {
      DV(g_print (B_STRLOC": scrolling onscreen\n"));
      btk_text_view_scroll_mark_onscreen (text_view, btk_text_buffer_get_insert (buffer));
    }

  priv->scroll_after_paste = TRUE;
}

static void
btk_text_view_toggle_overwrite (BtkTextView *text_view)
{
  if (text_view->text_window)
    text_window_invalidate_cursors (text_view->text_window);

  text_view->overwrite_mode = !text_view->overwrite_mode;

  if (text_view->layout)
    btk_text_layout_set_overwrite_mode (text_view->layout,
					text_view->overwrite_mode && text_view->editable);

  if (text_view->text_window)
    text_window_invalidate_cursors (text_view->text_window);

  btk_text_view_pend_cursor_blink (text_view);

  g_object_notify (G_OBJECT (text_view), "overwrite");
}

/**
 * btk_text_view_get_overwrite:
 * @text_view: a #BtkTextView
 *
 * Returns whether the #BtkTextView is in overwrite mode or not.
 *
 * Return value: whether @text_view is in overwrite mode or not.
 * 
 * Since: 2.4
 **/
gboolean
btk_text_view_get_overwrite (BtkTextView *text_view)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), FALSE);

  return text_view->overwrite_mode;
}

/**
 * btk_text_view_set_overwrite:
 * @text_view: a #BtkTextView
 * @overwrite: %TRUE to turn on overwrite mode, %FALSE to turn it off
 *
 * Changes the #BtkTextView overwrite mode.
 *
 * Since: 2.4
 **/
void
btk_text_view_set_overwrite (BtkTextView *text_view,
			     gboolean     overwrite)
{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));
  overwrite = overwrite != FALSE;

  if (text_view->overwrite_mode != overwrite)
    btk_text_view_toggle_overwrite (text_view);
}

/**
 * btk_text_view_set_accepts_tab:
 * @text_view: A #BtkTextView
 * @accepts_tab: %TRUE if pressing the Tab key should insert a tab 
 *    character, %FALSE, if pressing the Tab key should move the 
 *    keyboard focus.
 * 
 * Sets the behavior of the text widget when the Tab key is pressed. 
 * If @accepts_tab is %TRUE, a tab character is inserted. If @accepts_tab 
 * is %FALSE the keyboard focus is moved to the next widget in the focus 
 * chain.
 * 
 * Since: 2.4
 **/
void
btk_text_view_set_accepts_tab (BtkTextView *text_view,
			       gboolean     accepts_tab)
{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));

  accepts_tab = accepts_tab != FALSE;

  if (text_view->accepts_tab != accepts_tab)
    {
      text_view->accepts_tab = accepts_tab;

      g_object_notify (G_OBJECT (text_view), "accepts-tab");
    }
}

/**
 * btk_text_view_get_accepts_tab:
 * @text_view: A #BtkTextView
 * 
 * Returns whether pressing the Tab key inserts a tab characters.
 * btk_text_view_set_accepts_tab().
 * 
 * Return value: %TRUE if pressing the Tab key inserts a tab character, 
 *   %FALSE if pressing the Tab key moves the keyboard focus.
 * 
 * Since: 2.4
 **/
gboolean
btk_text_view_get_accepts_tab (BtkTextView *text_view)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), FALSE);

  return text_view->accepts_tab;
}

static void
btk_text_view_compat_move_focus (BtkTextView     *text_view,
                                 BtkDirectionType direction_type)
{
  GSignalInvocationHint *hint = g_signal_get_invocation_hint (text_view);

  /*  as of BTK+ 2.12, the "move-focus" signal has been moved to BtkWidget,
   *  the evil code below makes sure that both emitting the signal and
   *  calling the virtual function directly continue to work as expetcted
   */

  if (hint->signal_id == g_signal_lookup ("move-focus", BTK_TYPE_WIDGET))
    {
      /*  if this is a signal emission, chain up  */

      gboolean retval;

      g_signal_chain_from_overridden_handler (text_view,
                                              direction_type, &retval);
    }
  else
    {
      /*  otherwise emit the signal, since somebody called the virtual
       *  function directly
       */

      g_signal_emit_by_name (text_view, "move-focus", direction_type);
    }
}

/*
 * Selections
 */

static void
btk_text_view_unselect (BtkTextView *text_view)
{
  BtkTextIter insert;

  btk_text_buffer_get_iter_at_mark (get_buffer (text_view), &insert,
                                    btk_text_buffer_get_insert (get_buffer (text_view)));

  btk_text_buffer_move_mark (get_buffer (text_view),
                             btk_text_buffer_get_selection_bound (get_buffer (text_view)),
                             &insert);
}

static void
get_iter_at_pointer (BtkTextView *text_view,
                     BtkTextIter *iter,
		     gint        *x,
		     gint        *y)
{
  gint xcoord, ycoord;
  BdkModifierType state;

  bdk_window_get_pointer (text_view->text_window->bin_window,
                          &xcoord, &ycoord, &state);
  
  btk_text_layout_get_iter_at_pixel (text_view->layout,
                                     iter,
                                     xcoord + text_view->xoffset,
                                     ycoord + text_view->yoffset);
  if (x)
    *x = xcoord;

  if (y)
    *y = ycoord;
}

static void
move_mark_to_pointer_and_scroll (BtkTextView *text_view,
                                 const gchar *mark_name)
{
  BtkTextIter newplace;
  BtkTextMark *mark;

  get_iter_at_pointer (text_view, &newplace, NULL, NULL);
  
  mark = btk_text_buffer_get_mark (get_buffer (text_view), mark_name);
  
  /* This may invalidate the layout */
  DV(g_print (B_STRLOC": move mark\n"));
  
  btk_text_buffer_move_mark (get_buffer (text_view),
			     mark,
			     &newplace);
  
  DV(g_print (B_STRLOC": scrolling onscreen\n"));
  btk_text_view_scroll_mark_onscreen (text_view, mark);

  DV (g_print ("first validate idle leaving %s is %d\n",
               B_STRLOC, text_view->first_validate_idle));
}

static gboolean
selection_scan_timeout (gpointer data)
{
  BtkTextView *text_view;

  text_view = BTK_TEXT_VIEW (data);

  DV(g_print (B_STRLOC": calling move_mark_to_pointer_and_scroll\n"));
  btk_text_view_scroll_mark_onscreen (text_view, 
				      btk_text_buffer_get_insert (get_buffer (text_view)));

  return TRUE; /* remain installed. */
}

#define UPPER_OFFSET_ANCHOR 0.8
#define LOWER_OFFSET_ANCHOR 0.2

static gboolean
check_scroll (gdouble offset, BtkAdjustment *adj)
{
  if ((offset > UPPER_OFFSET_ANCHOR &&
       adj->value + adj->page_size < adj->upper) ||
      (offset < LOWER_OFFSET_ANCHOR &&
       adj->value > adj->lower))
    return TRUE;

  return FALSE;
}

static gint
drag_scan_timeout (gpointer data)
{
  BtkTextView *text_view;
  BtkTextIter newplace;
  gint x, y, width, height;
  gdouble pointer_xoffset, pointer_yoffset;

  text_view = BTK_TEXT_VIEW (data);

  get_iter_at_pointer (text_view, &newplace, &x, &y);
  width = bdk_window_get_width (text_view->text_window->bin_window);
  height = bdk_window_get_height (text_view->text_window->bin_window);

  btk_text_buffer_move_mark (get_buffer (text_view),
                             text_view->dnd_mark,
                             &newplace);

  pointer_xoffset = (gdouble) x / width;
  pointer_yoffset = (gdouble) y / height;

  if (check_scroll (pointer_xoffset, text_view->hadjustment) ||
      check_scroll (pointer_yoffset, text_view->vadjustment))
    {
      /* do not make offsets surpass lower nor upper anchors, this makes
       * scrolling speed relative to the distance of the pointer to the
       * anchors when it moves beyond them.
       */
      pointer_xoffset = CLAMP (pointer_xoffset, LOWER_OFFSET_ANCHOR, UPPER_OFFSET_ANCHOR);
      pointer_yoffset = CLAMP (pointer_yoffset, LOWER_OFFSET_ANCHOR, UPPER_OFFSET_ANCHOR);

      btk_text_view_scroll_to_mark (text_view,
                                    text_view->dnd_mark,
                                    0., TRUE, pointer_xoffset, pointer_yoffset);
    }

  return TRUE;
}

typedef enum 
{
  SELECT_CHARACTERS,
  SELECT_WORDS,
  SELECT_LINES
} SelectionGranularity;

/*
 * Move @start and @end to the boundaries of the selection unit (indicated by 
 * @granularity) which contained @start initially.
 * If the selction unit is SELECT_WORDS and @start is not contained in a word
 * the selection is extended to all the white spaces between the end of the 
 * word preceding @start and the start of the one following.
 */
static void
extend_selection (BtkTextView *text_view, 
		  SelectionGranularity granularity, 
		  BtkTextIter *start, 
		  BtkTextIter *end)
{
  *end = *start;

  if (granularity == SELECT_WORDS) 
    {
      if (btk_text_iter_inside_word (start))
	{
	  if (!btk_text_iter_starts_word (start))
	    btk_text_iter_backward_visible_word_start (start);
	  
	  if (!btk_text_iter_ends_word (end))
	    {
	      if (!btk_text_iter_forward_visible_word_end (end))
		btk_text_iter_forward_to_end (end);
	    }
	}
      else
	{
	  BtkTextIter tmp;

	  tmp = *start;
	  if (btk_text_iter_backward_visible_word_start (&tmp))
	    btk_text_iter_forward_visible_word_end (&tmp);

	  if (btk_text_iter_get_line (&tmp) == btk_text_iter_get_line (start))
	    *start = tmp;
	  else
	    btk_text_iter_set_line_offset (start, 0);

	  tmp = *end;
	  if (!btk_text_iter_forward_visible_word_end (&tmp))
	    btk_text_iter_forward_to_end (&tmp);

	  if (btk_text_iter_ends_word (&tmp))
	    btk_text_iter_backward_visible_word_start (&tmp);

	  if (btk_text_iter_get_line (&tmp) == btk_text_iter_get_line (end))
	    *end = tmp;
	  else
	    btk_text_iter_forward_to_line_end (end);
	}
    }
  else if (granularity == SELECT_LINES) 
    {
      if (btk_text_view_starts_display_line (text_view, start))
	{
	  /* If on a display line boundary, we assume the user
	   * clicked off the end of a line and we therefore select
	   * the line before the boundary.
	   */
	  btk_text_view_backward_display_line_start (text_view, start);
	}
      else
	{
	  /* start isn't on the start of a line, so we move it to the
	   * start, and move end to the end unless it's already there.
	   */
	  btk_text_view_backward_display_line_start (text_view, start);
	  
	  if (!btk_text_view_starts_display_line (text_view, end))
	    btk_text_view_forward_display_line_end (text_view, end);
	}
    }
}
 

typedef struct
{
  SelectionGranularity granularity;
  BtkTextMark *orig_start;
  BtkTextMark *orig_end;
} SelectionData;

static void
selection_data_free (SelectionData *data)
{
  if (data->orig_start != NULL)
    btk_text_buffer_delete_mark (btk_text_mark_get_buffer (data->orig_start),
                                 data->orig_start);
  if (data->orig_end != NULL)
    btk_text_buffer_delete_mark (btk_text_mark_get_buffer (data->orig_end),
                                 data->orig_end);
  g_free (data);
}

static gint
selection_motion_event_handler (BtkTextView    *text_view, 
				BdkEventMotion *event, 
				SelectionData  *data)
{
  bdk_event_request_motions (event);

  if (data->granularity == SELECT_CHARACTERS) 
    {
      move_mark_to_pointer_and_scroll (text_view, "insert");
    }
  else 
    {
      BtkTextIter cursor, start, end;
      BtkTextIter orig_start, orig_end;
      BtkTextBuffer *buffer;
      
      buffer = get_buffer (text_view);

      btk_text_buffer_get_iter_at_mark (buffer, &orig_start, data->orig_start);
      btk_text_buffer_get_iter_at_mark (buffer, &orig_end, data->orig_end);

      get_iter_at_pointer (text_view, &cursor, NULL, NULL);
      
      start = cursor;
      extend_selection (text_view, data->granularity, &start, &end);

      /* either the selection extends to the front, or end (or not) */
      if (btk_text_iter_compare (&cursor, &orig_start) < 0)
        btk_text_buffer_select_range (buffer, &start, &orig_end);
      else
        btk_text_buffer_select_range (buffer, &end, &orig_start);

      btk_text_view_scroll_mark_onscreen (text_view, 
					  btk_text_buffer_get_insert (buffer));
    }

  /* If we had to scroll offscreen, insert a timeout to do so
   * again. Note that in the timeout, even if the mouse doesn't
   * move, due to this scroll xoffset/yoffset will have changed
   * and we'll need to scroll again.
   */
  if (text_view->scroll_timeout != 0) /* reset on every motion event */
    g_source_remove (text_view->scroll_timeout);
  
  text_view->scroll_timeout =
    bdk_threads_add_timeout (50, selection_scan_timeout, text_view);

  return TRUE;
}

static void
btk_text_view_start_selection_drag (BtkTextView       *text_view,
                                    const BtkTextIter *iter,
                                    BdkEventButton    *button)
{
  BtkTextIter cursor, ins, bound;
  BtkTextIter orig_start, orig_end;
  BtkTextBuffer *buffer;
  SelectionData *data;

  if (text_view->selection_drag_handler != 0)
    return;
  
  data = g_new0 (SelectionData, 1);

  if (button->type == BDK_2BUTTON_PRESS)
    data->granularity = SELECT_WORDS;
  else if (button->type == BDK_3BUTTON_PRESS)
    data->granularity = SELECT_LINES;
  else 
    data->granularity = SELECT_CHARACTERS;

  btk_grab_add (BTK_WIDGET (text_view));

  buffer = get_buffer (text_view);
  
  cursor = *iter;
  ins = cursor;
  
  extend_selection (text_view, data->granularity, &ins, &bound);
  orig_start = ins;
  orig_end = bound;

  if (button->state & BTK_EXTEND_SELECTION_MOD_MASK)
    {
      /* Extend selection */
      BtkTextIter old_ins, old_bound;
      BtkTextIter old_start, old_end;

      btk_text_buffer_get_iter_at_mark (buffer, &old_ins, btk_text_buffer_get_insert (buffer));
      btk_text_buffer_get_iter_at_mark (buffer, &old_bound, btk_text_buffer_get_selection_bound (buffer));
      old_start = old_ins;
      old_end = old_bound;
      btk_text_iter_order (&old_start, &old_end);
      
      /* move the front cursor, if the mouse is in front of the selection. Should the
       * cursor however be inside the selection (this happens on tripple click) then we
       * move the side which was last moved (current insert mark) */
      if (btk_text_iter_compare (&cursor, &old_start) <= 0 ||
          (btk_text_iter_compare (&cursor, &old_end) < 0 && 
           btk_text_iter_compare (&old_ins, &old_bound) <= 0))
        {
          bound = old_end;
          orig_start = old_end;
          orig_end = old_end;
        }
      else
        {
          ins = bound;
          bound = old_start;
          orig_end = bound;
          orig_start = bound;
        }
    }

  btk_text_buffer_select_range (buffer, &ins, &bound);

  btk_text_iter_order (&orig_start, &orig_end);
  data->orig_start = btk_text_buffer_create_mark (buffer, NULL,
                                                  &orig_start, TRUE);
  data->orig_end = btk_text_buffer_create_mark (buffer, NULL,
                                                &orig_end, TRUE);

  btk_text_view_check_cursor_blink (text_view);

  text_view->selection_drag_handler = g_signal_connect_data (text_view,
                                                             "motion-notify-event",
                                                             G_CALLBACK (selection_motion_event_handler),
                                                             data,
                                                             (GClosureNotify) selection_data_free, 0);  
}

/* returns whether we were really dragging */
static gboolean
btk_text_view_end_selection_drag (BtkTextView    *text_view) 
{
  if (text_view->selection_drag_handler == 0)
    return FALSE;

  g_signal_handler_disconnect (text_view, text_view->selection_drag_handler);
  text_view->selection_drag_handler = 0;

  if (text_view->scroll_timeout != 0)
    {
      g_source_remove (text_view->scroll_timeout);
      text_view->scroll_timeout = 0;
    }

  btk_grab_remove (BTK_WIDGET (text_view));

  return TRUE;
}

/*
 * Layout utils
 */

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

static void
btk_text_view_check_keymap_direction (BtkTextView *text_view)
{
  if (text_view->layout)
    {
      BtkSettings *settings = btk_widget_get_settings (BTK_WIDGET (text_view));
      BdkKeymap *keymap = bdk_keymap_get_for_display (btk_widget_get_display (BTK_WIDGET (text_view)));
      BtkTextDirection new_cursor_dir;
      BtkTextDirection new_keyboard_dir;
      gboolean split_cursor;

      g_object_get (settings,
		    "btk-split-cursor", &split_cursor,
		    NULL);
      
      if (bdk_keymap_get_direction (keymap) == BANGO_DIRECTION_RTL)
	new_keyboard_dir = BTK_TEXT_DIR_RTL;
      else
	new_keyboard_dir  = BTK_TEXT_DIR_LTR;
  
      if (split_cursor)
	new_cursor_dir = BTK_TEXT_DIR_NONE;
      else
	new_cursor_dir = new_keyboard_dir;
      
      btk_text_layout_set_cursor_direction (text_view->layout, new_cursor_dir);
      btk_text_layout_set_keyboard_direction (text_view->layout, new_keyboard_dir);
    }
}

static void
btk_text_view_ensure_layout (BtkTextView *text_view)
{
  BtkWidget *widget;

  widget = BTK_WIDGET (text_view);

  if (text_view->layout == NULL)
    {
      BtkTextAttributes *style;
      BangoContext *ltr_context, *rtl_context;
      GSList *tmp_list;

      DV(g_print(B_STRLOC"\n"));
      
      text_view->layout = btk_text_layout_new ();

      g_signal_connect (text_view->layout,
			"invalidated",
			G_CALLBACK (invalidated_handler),
			text_view);

      g_signal_connect (text_view->layout,
			"changed",
			G_CALLBACK (changed_handler),
			text_view);

      g_signal_connect (text_view->layout,
			"allocate-child",
			G_CALLBACK (btk_text_view_child_allocated),
			text_view);
      
      if (get_buffer (text_view))
        btk_text_layout_set_buffer (text_view->layout, get_buffer (text_view));

      if ((btk_widget_has_focus (widget) && text_view->cursor_visible))
        btk_text_view_pend_cursor_blink (text_view);
      else
        btk_text_layout_set_cursor_visible (text_view->layout, FALSE);

      btk_text_layout_set_overwrite_mode (text_view->layout,
					  text_view->overwrite_mode && text_view->editable);

      ltr_context = btk_widget_create_bango_context (BTK_WIDGET (text_view));
      bango_context_set_base_dir (ltr_context, BANGO_DIRECTION_LTR);
      rtl_context = btk_widget_create_bango_context (BTK_WIDGET (text_view));
      bango_context_set_base_dir (rtl_context, BANGO_DIRECTION_RTL);

      btk_text_layout_set_contexts (text_view->layout, ltr_context, rtl_context);

      g_object_unref (ltr_context);
      g_object_unref (rtl_context);

      btk_text_view_check_keymap_direction (text_view);

      style = btk_text_attributes_new ();

      btk_widget_ensure_style (widget);
      btk_text_view_set_attributes_from_style (text_view,
                                               style, widget->style);

      style->pixels_above_lines = text_view->pixels_above_lines;
      style->pixels_below_lines = text_view->pixels_below_lines;
      style->pixels_inside_wrap = text_view->pixels_inside_wrap;
      style->left_margin = text_view->left_margin;
      style->right_margin = text_view->right_margin;
      style->indent = text_view->indent;
      style->tabs = text_view->tabs ? bango_tab_array_copy (text_view->tabs) : NULL;

      style->wrap_mode = text_view->wrap_mode;
      style->justification = text_view->justify;
      style->direction = btk_widget_get_direction (BTK_WIDGET (text_view));

      btk_text_layout_set_default_style (text_view->layout, style);

      btk_text_attributes_unref (style);

      /* Set layout for all anchored children */

      tmp_list = text_view->children;
      while (tmp_list != NULL)
        {
          BtkTextViewChild *vc = tmp_list->data;

          if (vc->anchor)
            {
              btk_text_anchored_child_set_layout (vc->widget,
                                                  text_view->layout);
              /* vc may now be invalid! */
            }

          tmp_list = g_slist_next (tmp_list);
        }

      btk_text_view_invalidate (text_view);
    }
}

/**
 * btk_text_view_get_default_attributes:
 * @text_view: a #BtkTextView
 * 
 * Obtains a copy of the default text attributes. These are the
 * attributes used for text unless a tag overrides them.
 * You'd typically pass the default attributes in to
 * btk_text_iter_get_attributes() in order to get the
 * attributes in effect at a given text position.
 *
 * The return value is a copy owned by the caller of this function,
 * and should be freed.
 * 
 * Return value: a new #BtkTextAttributes
 **/
BtkTextAttributes*
btk_text_view_get_default_attributes (BtkTextView *text_view)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), NULL);
  
  btk_text_view_ensure_layout (text_view);

  return btk_text_attributes_copy (text_view->layout->default_style);
}

static void
btk_text_view_destroy_layout (BtkTextView *text_view)
{
  if (text_view->layout)
    {
      GSList *tmp_list;

      btk_text_view_remove_validate_idles (text_view);

      g_signal_handlers_disconnect_by_func (text_view->layout,
					    invalidated_handler,
					    text_view);
      g_signal_handlers_disconnect_by_func (text_view->layout,
					    changed_handler,
					    text_view);

      /* Remove layout from all anchored children */
      tmp_list = text_view->children;
      while (tmp_list != NULL)
        {
          BtkTextViewChild *vc = tmp_list->data;

          if (vc->anchor)
            {
              btk_text_anchored_child_set_layout (vc->widget, NULL);
              /* vc may now be invalid! */
            }

          tmp_list = g_slist_next (tmp_list);
        }

      btk_text_view_stop_cursor_blink (text_view);
      btk_text_view_end_selection_drag (text_view);

      g_object_unref (text_view->layout);
      text_view->layout = NULL;
    }
}

/**
 * btk_text_view_reset_im_context:
 * @text_view: a #BtkTextView
 *
 * Reset the input method context of the text view if needed.
 *
 * This can be necessary in the case where modifying the buffer
 * would confuse on-going input method behavior.
 *
 * Since: 2.22
 */
void
btk_text_view_reset_im_context (BtkTextView *text_view)
{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));

  if (text_view->need_im_reset)
    {
      text_view->need_im_reset = FALSE;
      btk_im_context_reset (text_view->im_context);
    }
}

/**
 * btk_text_view_im_context_filter_keypress:
 * @text_view: a #BtkTextView
 * @event: the key event
 *
 * Allow the #BtkTextView input method to internally handle key press
 * and release events. If this function returns %TRUE, then no further
 * processing should be done for this key event. See
 * btk_im_context_filter_keypress().
 *
 * Note that you are expected to call this function from your handler
 * when overriding key event handling. This is needed in the case when
 * you need to insert your own key handling between the input method
 * and the default key event handling of the #BtkTextView.
 *
 * |[
 * static gboolean
 * btk_foo_bar_key_press_event (BtkWidget   *widget,
 *                              BdkEventKey *event)
 * {
 *   if ((key->keyval == BDK_Return || key->keyval == BDK_KP_Enter))
 *     {
 *       if (btk_text_view_im_context_filter_keypress (BTK_TEXT_VIEW (view), event))
 *         return TRUE;
 *     }
 *
 *     /&ast; Do some stuff &ast;/
 *
 *   return BTK_WIDGET_CLASS (btk_foo_bar_parent_class)->key_press_event (widget, event);
 * }
 * ]|
 *
 * Return value: %TRUE if the input method handled the key event.
 *
 * Since: 2.22
 */
gboolean
btk_text_view_im_context_filter_keypress (BtkTextView  *text_view,
                                          BdkEventKey  *event)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), FALSE);

  return btk_im_context_filter_keypress (text_view->im_context, event);
}

/*
 * DND feature
 */

static void
drag_begin_cb (BtkWidget      *widget,
               BdkDragContext *context,
               gpointer        data)
{
  BtkTextView   *text_view = BTK_TEXT_VIEW (widget);
  BtkTextBuffer *buffer = btk_text_view_get_buffer (text_view);
  BtkTextIter    start;
  BtkTextIter    end;
  BdkPixmap     *pixmap = NULL;

  g_signal_handlers_disconnect_by_func (widget, drag_begin_cb, NULL);

  if (btk_text_buffer_get_selection_bounds (buffer, &start, &end))
    pixmap = _btk_text_util_create_rich_drag_icon (widget, buffer, &start, &end);

  if (pixmap)
    {
      btk_drag_set_icon_pixmap (context,
                                bdk_drawable_get_colormap (pixmap),
                                pixmap,
                                NULL,
                                -2, -2);
      g_object_unref (pixmap);
    }
  else
    {
      btk_drag_set_icon_default (context);
    }
}

static void
btk_text_view_start_selection_dnd (BtkTextView       *text_view,
                                   const BtkTextIter *iter,
                                   BdkEventMotion    *event)
{
  BtkTargetList *target_list;

  text_view->drag_start_x = -1;
  text_view->drag_start_y = -1;
  text_view->pending_place_cursor_button = 0;

  target_list = btk_text_buffer_get_copy_target_list (get_buffer (text_view));

  g_signal_connect (text_view, "drag-begin",
                    G_CALLBACK (drag_begin_cb), NULL);
  btk_drag_begin (BTK_WIDGET (text_view), target_list,
		  BDK_ACTION_COPY | BDK_ACTION_MOVE,
		  1, (BdkEvent*)event);
}

static void
btk_text_view_drag_begin (BtkWidget        *widget,
                          BdkDragContext   *context)
{
  /* do nothing */
}

static void
btk_text_view_drag_end (BtkWidget        *widget,
                        BdkDragContext   *context)
{
}

static void
btk_text_view_drag_data_get (BtkWidget        *widget,
                             BdkDragContext   *context,
                             BtkSelectionData *selection_data,
                             guint             info,
                             guint             time)
{
  BtkTextView *text_view = BTK_TEXT_VIEW (widget);
  BtkTextBuffer *buffer = btk_text_view_get_buffer (text_view);

  if (info == BTK_TEXT_BUFFER_TARGET_INFO_BUFFER_CONTENTS)
    {
      btk_selection_data_set (selection_data,
                              bdk_atom_intern_static_string ("BTK_TEXT_BUFFER_CONTENTS"),
                              8, /* bytes */
                              (void*)&buffer,
                              sizeof (buffer));
    }
  else if (info == BTK_TEXT_BUFFER_TARGET_INFO_RICH_TEXT)
    {
      BtkTextIter start;
      BtkTextIter end;
      guint8 *str = NULL;
      gsize len;

      if (btk_text_buffer_get_selection_bounds (buffer, &start, &end))
        {
          /* Extract the selected text */
          str = btk_text_buffer_serialize (buffer, buffer,
                                           selection_data->target,
                                           &start, &end,
                                           &len);
        }

      if (str)
        {
          btk_selection_data_set (selection_data,
                                  selection_data->target,
                                  8, /* bytes */
                                  (guchar *) str, len);
          g_free (str);
        }
    }
  else
    {
      BtkTextIter start;
      BtkTextIter end;
      gchar *str = NULL;

      if (btk_text_buffer_get_selection_bounds (buffer, &start, &end))
        {
          /* Extract the selected text */
          str = btk_text_iter_get_visible_text (&start, &end);
        }

      if (str)
        {
          btk_selection_data_set_text (selection_data, str, -1);
          g_free (str);
        }
    }
}

static void
btk_text_view_drag_data_delete (BtkWidget        *widget,
                                BdkDragContext   *context)
{
  btk_text_buffer_delete_selection (BTK_TEXT_VIEW (widget)->buffer,
                                    TRUE, BTK_TEXT_VIEW (widget)->editable);
}

static void
btk_text_view_drag_leave (BtkWidget        *widget,
                          BdkDragContext   *context,
                          guint             time)
{
  BtkTextView *text_view;

  text_view = BTK_TEXT_VIEW (widget);

  btk_text_mark_set_visible (text_view->dnd_mark, FALSE);
  
  if (text_view->scroll_timeout != 0)
    g_source_remove (text_view->scroll_timeout);

  text_view->scroll_timeout = 0;
}

static gboolean
btk_text_view_drag_motion (BtkWidget        *widget,
                           BdkDragContext   *context,
                           gint              x,
                           gint              y,
                           guint             time)
{
  BtkTextIter newplace;
  BtkTextView *text_view;
  BtkTextIter start;
  BtkTextIter end;
  BdkRectangle target_rect;
  gint bx, by;
  BdkAtom target;
  BdkDragAction suggested_action = 0;
  
  text_view = BTK_TEXT_VIEW (widget);

  target_rect = text_view->text_window->allocation;
  
  if (x < target_rect.x ||
      y < target_rect.y ||
      x > (target_rect.x + target_rect.width) ||
      y > (target_rect.y + target_rect.height))
    return FALSE; /* outside the text window, allow parent widgets to handle event */

  btk_text_view_window_to_buffer_coords (text_view,
                                         BTK_TEXT_WINDOW_WIDGET,
                                         x, y,
                                         &bx, &by);

  btk_text_layout_get_iter_at_pixel (text_view->layout,
                                     &newplace,
                                     bx, by);  

  target = btk_drag_dest_find_target (widget, context,
                                      btk_drag_dest_get_target_list (widget));

  if (target == BDK_NONE)
    {
      /* can't accept any of the offered targets */
    }                                 
  else if (btk_text_buffer_get_selection_bounds (get_buffer (text_view),
                                                 &start, &end) &&
           btk_text_iter_compare (&newplace, &start) >= 0 &&
           btk_text_iter_compare (&newplace, &end) <= 0)
    {
      /* We're inside the selection. */
    }
  else
    {      
      if (btk_text_iter_can_insert (&newplace, text_view->editable))
        {
          BtkWidget *source_widget;
          
          suggested_action = bdk_drag_context_get_suggested_action (context);
          
          source_widget = btk_drag_get_source_widget (context);
          
          if (source_widget == widget)
            {
              /* Default to MOVE, unless the user has
               * pressed ctrl or alt to affect available actions
               */
              if ((bdk_drag_context_get_actions (context) & BDK_ACTION_MOVE) != 0)
                suggested_action = BDK_ACTION_MOVE;
            }
        }
      else
        {
          /* Can't drop here. */
        }
    }

  if (suggested_action != 0)
    {
      btk_text_mark_set_visible (text_view->dnd_mark,
                                 text_view->cursor_visible);
      
      bdk_drag_status (context, suggested_action, time);
    }
  else
    {
      bdk_drag_status (context, 0, time);
      btk_text_mark_set_visible (text_view->dnd_mark, FALSE);
    }
      
  if (!text_view->scroll_timeout)
    text_view->scroll_timeout =
      bdk_threads_add_timeout (100, drag_scan_timeout, text_view);

  /* TRUE return means don't propagate the drag motion to parent
   * widgets that may also be drop sites.
   */
  return TRUE;
}

static gboolean
btk_text_view_drag_drop (BtkWidget        *widget,
                         BdkDragContext   *context,
                         gint              x,
                         gint              y,
                         guint             time)
{
  BtkTextView *text_view;
  BtkTextIter drop_point;
  BdkAtom target = BDK_NONE;
  
  text_view = BTK_TEXT_VIEW (widget);
  
  if (text_view->scroll_timeout != 0)
    g_source_remove (text_view->scroll_timeout);

  text_view->scroll_timeout = 0;

  btk_text_mark_set_visible (text_view->dnd_mark, FALSE);

  btk_text_buffer_get_iter_at_mark (get_buffer (text_view),
                                    &drop_point,
                                    text_view->dnd_mark);

  if (btk_text_iter_can_insert (&drop_point, text_view->editable))
    target = btk_drag_dest_find_target (widget, context, NULL);

  if (target != BDK_NONE)
    btk_drag_get_data (widget, context, target, time);
  else
    btk_drag_finish (context, FALSE, FALSE, time);

  return TRUE;
}

static void
insert_text_data (BtkTextView      *text_view,
                  BtkTextIter      *drop_point,
                  BtkSelectionData *selection_data)
{
  guchar *str;

  str = btk_selection_data_get_text (selection_data);

  if (str)
    {
      if (!btk_text_buffer_insert_interactive (get_buffer (text_view),
                                               drop_point, (gchar *) str, -1,
                                               text_view->editable))
        {
          btk_widget_error_bell (BTK_WIDGET (text_view));
        }

      g_free (str);
    }
}

static void
btk_text_view_drag_data_received (BtkWidget        *widget,
                                  BdkDragContext   *context,
                                  gint              x,
                                  gint              y,
                                  BtkSelectionData *selection_data,
                                  guint             info,
                                  guint             time)
{
  BtkTextIter drop_point;
  BtkTextView *text_view;
  gboolean success = FALSE;
  BtkTextBuffer *buffer = NULL;

  text_view = BTK_TEXT_VIEW (widget);

  if (!text_view->dnd_mark)
    goto done;

  buffer = get_buffer (text_view);

  btk_text_buffer_get_iter_at_mark (buffer,
                                    &drop_point,
                                    text_view->dnd_mark);
  
  if (!btk_text_iter_can_insert (&drop_point, text_view->editable))
    goto done;

  success = TRUE;

  btk_text_buffer_begin_user_action (buffer);

  if (info == BTK_TEXT_BUFFER_TARGET_INFO_BUFFER_CONTENTS)
    {
      BtkTextBuffer *src_buffer = NULL;
      BtkTextIter start, end;
      gboolean copy_tags = TRUE;

      if (selection_data->length != sizeof (src_buffer))
        return;

      memcpy (&src_buffer, selection_data->data, sizeof (src_buffer));

      if (src_buffer == NULL)
        return;

      g_return_if_fail (BTK_IS_TEXT_BUFFER (src_buffer));

      if (btk_text_buffer_get_tag_table (src_buffer) !=
          btk_text_buffer_get_tag_table (buffer))
        {
          /*  try to find a suitable rich text target instead  */
          BdkAtom *atoms;
          gint     n_atoms;
          GList   *list;
          BdkAtom  target = BDK_NONE;

          copy_tags = FALSE;

          atoms = btk_text_buffer_get_deserialize_formats (buffer, &n_atoms);

          for (list = bdk_drag_context_list_targets (context); list; list = g_list_next (list))
            {
              gint i;

              for (i = 0; i < n_atoms; i++)
                if (GUINT_TO_POINTER (atoms[i]) == list->data)
                  {
                    target = atoms[i];
                    break;
                  }
            }

          g_free (atoms);

          if (target != BDK_NONE)
            {
              btk_drag_get_data (widget, context, target, time);
              btk_text_buffer_end_user_action (buffer);
              return;
            }
        }

      if (btk_text_buffer_get_selection_bounds (src_buffer,
                                                &start,
                                                &end))
        {
          if (copy_tags)
            btk_text_buffer_insert_range_interactive (buffer,
                                                      &drop_point,
                                                      &start,
                                                      &end,
                                                      text_view->editable);
          else
            {
              gchar *str;

              str = btk_text_iter_get_visible_text (&start, &end);
              btk_text_buffer_insert_interactive (buffer,
                                                  &drop_point, str, -1,
                                                  text_view->editable);
              g_free (str);
            }
        }
    }
  else if (selection_data->length > 0 &&
           info == BTK_TEXT_BUFFER_TARGET_INFO_RICH_TEXT)
    {
      gboolean retval;
      GError *error = NULL;

      retval = btk_text_buffer_deserialize (buffer, buffer,
                                            selection_data->target,
                                            &drop_point,
                                            (guint8 *) selection_data->data,
                                            selection_data->length,
                                            &error);

      if (!retval)
        {
          g_warning ("error pasting: %s\n", error->message);
          g_clear_error (&error);
        }
    }
  else
    insert_text_data (text_view, &drop_point, selection_data);

 done:
  btk_drag_finish (context, success,
		   success && bdk_drag_context_get_selected_action (context) == BDK_ACTION_MOVE,
		   time);

  if (success)
    {
      btk_text_buffer_get_iter_at_mark (buffer,
                                        &drop_point,
                                        text_view->dnd_mark);
      btk_text_buffer_place_cursor (buffer, &drop_point);

      btk_text_buffer_end_user_action (buffer);
    }
}

/**
 * btk_text_view_get_hadjustment:
 * @text_view: a #BtkTextView
 *
 * Gets the horizontal-scrolling #BtkAdjustment.
 *
 * Returns: (transfer none): pointer to the horizontal #BtkAdjustment
 *
 * Since: 2.22
 **/
BtkAdjustment*
btk_text_view_get_hadjustment (BtkTextView *text_view)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), NULL);

  return get_hadjustment (text_view);
}

/**
 * btk_text_view_get_vadjustment:
 * @text_view: a #BtkTextView
 *
 * Gets the vertical-scrolling #BtkAdjustment.
 *
 * Returns: (transfer none): pointer to the vertical #BtkAdjustment
 *
 * Since: 2.22
 **/
BtkAdjustment*
btk_text_view_get_vadjustment (BtkTextView *text_view)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), NULL);

  return get_vadjustment (text_view);
}

static BtkAdjustment*
get_hadjustment (BtkTextView *text_view)
{
  if (text_view->hadjustment == NULL)
    btk_text_view_set_scroll_adjustments (text_view,
                                          NULL, /* forces creation */
                                          text_view->vadjustment);

  return text_view->hadjustment;
}

static BtkAdjustment*
get_vadjustment (BtkTextView *text_view)
{
  if (text_view->vadjustment == NULL)
    btk_text_view_set_scroll_adjustments (text_view,
                                          text_view->hadjustment,
                                          NULL); /* forces creation */
  return text_view->vadjustment;
}


static void
btk_text_view_set_scroll_adjustments (BtkTextView   *text_view,
                                      BtkAdjustment *hadj,
                                      BtkAdjustment *vadj)
{
  gboolean need_adjust = FALSE;

  if (hadj)
    g_return_if_fail (BTK_IS_ADJUSTMENT (hadj));
  else
    hadj = BTK_ADJUSTMENT (btk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  if (vadj)
    g_return_if_fail (BTK_IS_ADJUSTMENT (vadj));
  else
    vadj = BTK_ADJUSTMENT (btk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));

  if (text_view->hadjustment && (text_view->hadjustment != hadj))
    {
      g_signal_handlers_disconnect_by_func (text_view->hadjustment,
					    btk_text_view_value_changed,
					    text_view);
      g_object_unref (text_view->hadjustment);
    }

  if (text_view->vadjustment && (text_view->vadjustment != vadj))
    {
      g_signal_handlers_disconnect_by_func (text_view->vadjustment,
					    btk_text_view_value_changed,
					    text_view);
      g_object_unref (text_view->vadjustment);
    }

  if (text_view->hadjustment != hadj)
    {
      text_view->hadjustment = hadj;
      g_object_ref_sink (text_view->hadjustment);
      
      g_signal_connect (text_view->hadjustment, "value-changed",
                        G_CALLBACK (btk_text_view_value_changed),
			text_view);
      need_adjust = TRUE;
    }

  if (text_view->vadjustment != vadj)
    {
      text_view->vadjustment = vadj;
      g_object_ref_sink (text_view->vadjustment);
      
      g_signal_connect (text_view->vadjustment, "value-changed",
                        G_CALLBACK (btk_text_view_value_changed),
			text_view);
      need_adjust = TRUE;
    }

  if (need_adjust)
    btk_text_view_value_changed (NULL, text_view);
}

/* FIXME this adjust_allocation is a big cut-and-paste from
 * BtkCList, needs to be some "official" way to do this
 * factored out.
 */
typedef struct
{
  BdkWindow *window;
  int dx;
  int dy;
} ScrollData;

/* The window to which widget->window is relative */
#define ALLOCATION_WINDOW(widget)		\
   (!btk_widget_get_has_window (widget) ?		\
    (widget)->window :                          \
     bdk_window_get_parent ((widget)->window))

static void
adjust_allocation_recurse (BtkWidget *widget,
			   gpointer   data)
{
  ScrollData *scroll_data = data;

  /* Need to really size allocate instead of just poking
   * into widget->allocation if the widget is not realized.
   * FIXME someone figure out why this was.
   */
  if (!btk_widget_get_realized (widget))
    {
      if (btk_widget_get_visible (widget))
	{
	  BdkRectangle tmp_rectangle = widget->allocation;
	  tmp_rectangle.x += scroll_data->dx;
          tmp_rectangle.y += scroll_data->dy;
          
	  btk_widget_size_allocate (widget, &tmp_rectangle);
	}
    }
  else
    {
      if (ALLOCATION_WINDOW (widget) == scroll_data->window)
	{
	  widget->allocation.x += scroll_data->dx;
          widget->allocation.y += scroll_data->dy;
          
	  if (BTK_IS_CONTAINER (widget))
	    btk_container_forall (BTK_CONTAINER (widget),
				  adjust_allocation_recurse,
				  data);
	}
    }
}

static void
adjust_allocation (BtkWidget *widget,
		   int        dx,
                   int        dy)
{
  ScrollData scroll_data;

  if (btk_widget_get_realized (widget))
    scroll_data.window = ALLOCATION_WINDOW (widget);
  else
    scroll_data.window = NULL;
    
  scroll_data.dx = dx;
  scroll_data.dy = dy;
  
  adjust_allocation_recurse (widget, &scroll_data);
}
            
static void
btk_text_view_value_changed (BtkAdjustment *adj,
                             BtkTextView   *text_view)
{
  BtkTextIter iter;
  gint line_top;
  gint dx = 0;
  gint dy = 0;
  
  /* Note that we oddly call this function with adj == NULL
   * sometimes
   */
  
  text_view->onscreen_validated = FALSE;

  DV(g_print(">Scroll offset changed %s/%g, onscreen_validated = FALSE ("B_STRLOC")\n",
             adj == text_view->hadjustment ? "hadj" : adj == text_view->vadjustment ? "vadj" : "none",
             adj ? adj->value : 0.0));
  
  if (adj == text_view->hadjustment)
    {
      dx = text_view->xoffset - (gint)adj->value;
      text_view->xoffset = adj->value;

      /* If the change is due to a size change we need 
       * to invalidate the entire text window because there might be
       * right-aligned or centered text 
       */
      if (text_view->width_changed)
	{
	  if (btk_widget_get_realized (BTK_WIDGET (text_view)))
	    bdk_window_invalidate_rect (text_view->text_window->bin_window, NULL, FALSE);
	  
	  text_view->width_changed = FALSE;
	}
    }
  else if (adj == text_view->vadjustment)
    {
      dy = text_view->yoffset - (gint)adj->value;
      text_view->yoffset = adj->value;

      if (text_view->layout)
        {
          btk_text_layout_get_line_at_y (text_view->layout, &iter, adj->value, &line_top);

          btk_text_buffer_move_mark (get_buffer (text_view), text_view->first_para_mark, &iter);

          text_view->first_para_pixels = adj->value - line_top;
        }
    }
  
  if (dx != 0 || dy != 0)
    {
      GSList *tmp_list;

      if (btk_widget_get_realized (BTK_WIDGET (text_view)))
        {
          if (dy != 0)
            {
              if (text_view->left_window)
                text_window_scroll (text_view->left_window, 0, dy);
              if (text_view->right_window)
                text_window_scroll (text_view->right_window, 0, dy);
            }
      
          if (dx != 0)
            {
              if (text_view->top_window)
                text_window_scroll (text_view->top_window, dx, 0);
              if (text_view->bottom_window)
                text_window_scroll (text_view->bottom_window, dx, 0);
            }
      
          /* It looks nicer to scroll the main area last, because
           * it takes a while, and making the side areas update
           * afterward emphasizes the slowness of scrolling the
           * main area.
           */
          text_window_scroll (text_view->text_window, dx, dy);
        }
      
      /* Children are now "moved" in the text window, poke
       * into widget->allocation for each child
       */
      tmp_list = text_view->children;
      while (tmp_list != NULL)
        {
          BtkTextViewChild *child = tmp_list->data;
          
          if (child->anchor)
            adjust_allocation (child->widget, dx, dy);
          
          tmp_list = g_slist_next (tmp_list);
        }
    }

  /* This could result in invalidation, which would install the
   * first_validate_idle, which would validate onscreen;
   * but we're going to go ahead and validate here, so
   * first_validate_idle shouldn't have anything to do.
   */
  btk_text_view_update_layout_width (text_view);
  
  /* We also update the IM spot location here, since the IM context
   * might do something that leads to validation.
   */
  btk_text_view_update_im_spot_location (text_view);

  /* note that validation of onscreen could invoke this function
   * recursively, by scrolling to maintain first_para, or in response
   * to updating the layout width, however there is no problem with
   * that, or shouldn't be.
   */
  btk_text_view_validate_onscreen (text_view);
  
  /* process exposes */
  if (btk_widget_get_realized (BTK_WIDGET (text_view)))
    {
      DV (g_print ("Processing updates (%s)\n", B_STRLOC));
      
      if (text_view->left_window)
        bdk_window_process_updates (text_view->left_window->bin_window, TRUE);

      if (text_view->right_window)
        bdk_window_process_updates (text_view->right_window->bin_window, TRUE);

      if (text_view->top_window)
        bdk_window_process_updates (text_view->top_window->bin_window, TRUE);
      
      if (text_view->bottom_window)
        bdk_window_process_updates (text_view->bottom_window->bin_window, TRUE);
  
      bdk_window_process_updates (text_view->text_window->bin_window, TRUE);
    }

  /* If this got installed, get rid of it, it's just a waste of time. */
  if (text_view->first_validate_idle != 0)
    {
      g_source_remove (text_view->first_validate_idle);
      text_view->first_validate_idle = 0;
    }

  /* Finally we update the IM cursor location again, to ensure any
   * changes made by the validation are pushed through.
   */
  btk_text_view_update_im_spot_location (text_view);
  
  DV(g_print(">End scroll offset changed handler ("B_STRLOC")\n"));
}

static void
btk_text_view_commit_handler (BtkIMContext  *context,
                              const gchar   *str,
                              BtkTextView   *text_view)
{
  btk_text_view_commit_text (text_view, str);
}

static void
btk_text_view_commit_text (BtkTextView   *text_view,
                           const gchar   *str)
{
  gboolean had_selection;
  
  btk_text_buffer_begin_user_action (get_buffer (text_view));

  had_selection = btk_text_buffer_get_selection_bounds (get_buffer (text_view),
                                                        NULL, NULL);
  
  btk_text_buffer_delete_selection (get_buffer (text_view), TRUE,
                                    text_view->editable);

  if (!strcmp (str, "\n"))
    {
      if (!btk_text_buffer_insert_interactive_at_cursor (get_buffer (text_view), "\n", 1,
                                                         text_view->editable))
        {
          btk_widget_error_bell (BTK_WIDGET (text_view));
        }
    }
  else
    {
      if (!had_selection && text_view->overwrite_mode)
	{
	  BtkTextIter insert;

	  btk_text_buffer_get_iter_at_mark (get_buffer (text_view),
					    &insert,
					    btk_text_buffer_get_insert (get_buffer (text_view)));
	  if (!btk_text_iter_ends_line (&insert))
	    btk_text_view_delete_from_cursor (text_view, BTK_DELETE_CHARS, 1);
	}

      if (!btk_text_buffer_insert_interactive_at_cursor (get_buffer (text_view), str, -1,
                                                         text_view->editable))
        {
          btk_widget_error_bell (BTK_WIDGET (text_view));
        }
    }

  btk_text_buffer_end_user_action (get_buffer (text_view));

  btk_text_view_set_virtual_cursor_pos (text_view, -1, -1);
  DV(g_print (B_STRLOC": scrolling onscreen\n"));
  btk_text_view_scroll_mark_onscreen (text_view,
                                      btk_text_buffer_get_insert (get_buffer (text_view)));
}

static void
btk_text_view_preedit_changed_handler (BtkIMContext *context,
				       BtkTextView  *text_view)
{
  gchar *str;
  BangoAttrList *attrs;
  gint cursor_pos;
  BtkTextIter iter;

  btk_text_buffer_get_iter_at_mark (text_view->buffer, &iter,
				    btk_text_buffer_get_insert (text_view->buffer));

  /* Keypress events are passed to input method even if cursor position is
   * not editable; so beep here if it's multi-key input sequence, input
   * method will be reset in key-press-event handler.
   */
  btk_im_context_get_preedit_string (context, &str, &attrs, &cursor_pos);

  if (str && str[0] && !btk_text_iter_can_insert (&iter, text_view->editable))
    {
      btk_widget_error_bell (BTK_WIDGET (text_view));
      goto out;
    }

  g_signal_emit (text_view, signals[PREEDIT_CHANGED], 0, str);

  if (text_view->layout)
    btk_text_layout_set_preedit_string (text_view->layout, str, attrs, cursor_pos);
  if (btk_widget_has_focus (BTK_WIDGET (text_view)))
    btk_text_view_scroll_mark_onscreen (text_view,
					btk_text_buffer_get_insert (get_buffer (text_view)));

out:
  bango_attr_list_unref (attrs);
  g_free (str);
}

static gboolean
btk_text_view_retrieve_surrounding_handler (BtkIMContext  *context,
					    BtkTextView   *text_view)
{
  BtkTextIter start;
  BtkTextIter end;
  gint pos;
  gchar *text;

  btk_text_buffer_get_iter_at_mark (text_view->buffer, &start,
				    btk_text_buffer_get_insert (text_view->buffer));
  end = start;

  pos = btk_text_iter_get_line_index (&start);
  btk_text_iter_set_line_offset (&start, 0);
  btk_text_iter_forward_to_line_end (&end);

  text = btk_text_iter_get_slice (&start, &end);
  btk_im_context_set_surrounding (context, text, -1, pos);
  g_free (text);

  return TRUE;
}

static gboolean
btk_text_view_delete_surrounding_handler (BtkIMContext  *context,
					  gint           offset,
					  gint           n_chars,
					  BtkTextView   *text_view)
{
  BtkTextIter start;
  BtkTextIter end;

  btk_text_buffer_get_iter_at_mark (text_view->buffer, &start,
				    btk_text_buffer_get_insert (text_view->buffer));
  end = start;

  btk_text_iter_forward_chars (&start, offset);
  btk_text_iter_forward_chars (&end, offset + n_chars);

  btk_text_buffer_delete_interactive (text_view->buffer, &start, &end,
				      text_view->editable);

  return TRUE;
}

static void
btk_text_view_mark_set_handler (BtkTextBuffer     *buffer,
                                const BtkTextIter *location,
                                BtkTextMark       *mark,
                                gpointer           data)
{
  BtkTextView *text_view = BTK_TEXT_VIEW (data);
  gboolean need_reset = FALSE;

  if (mark == btk_text_buffer_get_insert (buffer))
    {
      text_view->virtual_cursor_x = -1;
      text_view->virtual_cursor_y = -1;
      btk_text_view_update_im_spot_location (text_view);
      need_reset = TRUE;
    }
  else if (mark == btk_text_buffer_get_selection_bound (buffer))
    {
      need_reset = TRUE;
    }

  if (need_reset)
    btk_text_view_reset_im_context (text_view);
}

static void
btk_text_view_target_list_notify (BtkTextBuffer    *buffer,
                                  const GParamSpec *pspec,
                                  gpointer          data)
{
  BtkWidget     *widget = BTK_WIDGET (data);
  BtkTargetList *view_list;
  BtkTargetList *buffer_list;
  GList         *list;

  view_list = btk_drag_dest_get_target_list (widget);
  buffer_list = btk_text_buffer_get_paste_target_list (buffer);

  if (view_list)
    btk_target_list_ref (view_list);
  else
    view_list = btk_target_list_new (NULL, 0);

  list = view_list->list;
  while (list)
    {
      BtkTargetPair *pair = list->data;

      list = g_list_next (list); /* get next element before removing */

      if (pair->info >= BTK_TEXT_BUFFER_TARGET_INFO_TEXT &&
          pair->info <= BTK_TEXT_BUFFER_TARGET_INFO_BUFFER_CONTENTS)
        {
          btk_target_list_remove (view_list, pair->target);
        }
    }

  for (list = buffer_list->list; list; list = g_list_next (list))
    {
      BtkTargetPair *pair = list->data;

      btk_target_list_add (view_list, pair->target, pair->flags, pair->info);
    }

  btk_drag_dest_set_target_list (widget, view_list);
  btk_target_list_unref (view_list);
}

static void
btk_text_view_get_cursor_location  (BtkTextView   *text_view,
				    BdkRectangle  *pos)
{
  BtkTextIter insert;
  
  btk_text_buffer_get_iter_at_mark (get_buffer (text_view), &insert,
                                    btk_text_buffer_get_insert (get_buffer (text_view)));

  btk_text_layout_get_cursor_locations (text_view->layout, &insert, pos, NULL);
}

static void
btk_text_view_get_virtual_cursor_pos (BtkTextView *text_view,
                                      BtkTextIter *cursor,
                                      gint        *x,
                                      gint        *y)
{
  BtkTextIter insert;
  BdkRectangle pos;

  if (cursor)
    insert = *cursor;
  else
    btk_text_buffer_get_iter_at_mark (get_buffer (text_view), &insert,
                                      btk_text_buffer_get_insert (get_buffer (text_view)));

  if ((x && text_view->virtual_cursor_x == -1) ||
      (y && text_view->virtual_cursor_y == -1))
    btk_text_layout_get_cursor_locations (text_view->layout, &insert, &pos, NULL);

  if (x)
    {
      if (text_view->virtual_cursor_x != -1)
        *x = text_view->virtual_cursor_x;
      else
        *x = pos.x;
    }

  if (y)
    {
      if (text_view->virtual_cursor_x != -1)
        *y = text_view->virtual_cursor_y;
      else
        *y = pos.y + pos.height / 2;
    }
}

static void
btk_text_view_set_virtual_cursor_pos (BtkTextView *text_view,
                                      gint         x,
                                      gint         y)
{
  BdkRectangle pos;

  if (!text_view->layout)
    return;

  if (x == -1 || y == -1)
    btk_text_view_get_cursor_location (text_view, &pos);

  text_view->virtual_cursor_x = (x == -1) ? pos.x : x;
  text_view->virtual_cursor_y = (y == -1) ? pos.y + pos.height / 2 : y;
}

/* Quick hack of a popup menu
 */
static void
activate_cb (BtkWidget   *menuitem,
	     BtkTextView *text_view)
{
  const gchar *signal = g_object_get_data (G_OBJECT (menuitem), "btk-signal");
  g_signal_emit_by_name (text_view, signal);
}

static void
append_action_signal (BtkTextView  *text_view,
		      BtkWidget    *menu,
		      const gchar  *stock_id,
		      const gchar  *signal,
                      gboolean      sensitive)
{
  BtkWidget *menuitem = btk_image_menu_item_new_from_stock (stock_id, NULL);

  g_object_set_data (G_OBJECT (menuitem), I_("btk-signal"), (char *)signal);
  g_signal_connect (menuitem, "activate",
		    G_CALLBACK (activate_cb), text_view);

  btk_widget_set_sensitive (menuitem, sensitive);
  
  btk_widget_show (menuitem);
  btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
}

static void
btk_text_view_select_all (BtkWidget *widget,
			  gboolean select)
{
  BtkTextView *text_view = BTK_TEXT_VIEW (widget);
  BtkTextBuffer *buffer;
  BtkTextIter start_iter, end_iter, insert;

  buffer = text_view->buffer;
  if (select) 
    {
      btk_text_buffer_get_bounds (buffer, &start_iter, &end_iter);
      btk_text_buffer_select_range (buffer, &start_iter, &end_iter);
    }
  else 
    {
      btk_text_buffer_get_iter_at_mark (buffer, &insert,
					btk_text_buffer_get_insert (buffer));
      btk_text_buffer_move_mark_by_name (buffer, "selection_bound", &insert);
    }
}

static void
select_all_cb (BtkWidget   *menuitem,
	       BtkTextView *text_view)
{
  btk_text_view_select_all (BTK_WIDGET (text_view), TRUE);
}

static void
delete_cb (BtkTextView *text_view)
{
  btk_text_buffer_delete_selection (get_buffer (text_view), TRUE,
				    text_view->editable);
}

static void
popup_menu_detach (BtkWidget *attach_widget,
		   BtkMenu   *menu)
{
  BTK_TEXT_VIEW (attach_widget)->popup_menu = NULL;
}

static void
popup_position_func (BtkMenu   *menu,
                     gint      *x,
                     gint      *y,
                     gboolean  *push_in,
                     gpointer	user_data)
{
  BtkTextView *text_view;
  BtkWidget *widget;
  BdkRectangle cursor_rect;
  BdkRectangle onscreen_rect;
  gint root_x, root_y;
  BtkTextIter iter;
  BtkRequisition req;      
  BdkScreen *screen;
  gint monitor_num;
  BdkRectangle monitor;
      
  text_view = BTK_TEXT_VIEW (user_data);
  widget = BTK_WIDGET (text_view);
  
  g_return_if_fail (btk_widget_get_realized (widget));
  
  screen = btk_widget_get_screen (widget);

  bdk_window_get_origin (widget->window, &root_x, &root_y);

  btk_text_buffer_get_iter_at_mark (get_buffer (text_view),
                                    &iter,
                                    btk_text_buffer_get_insert (get_buffer (text_view)));

  btk_text_view_get_iter_location (text_view,
                                   &iter,
                                   &cursor_rect);

  btk_text_view_get_visible_rect (text_view, &onscreen_rect);
  
  btk_widget_size_request (text_view->popup_menu, &req);

  /* can't use rectangle_intersect since cursor rect can have 0 width */
  if (cursor_rect.x >= onscreen_rect.x &&
      cursor_rect.x < onscreen_rect.x + onscreen_rect.width &&
      cursor_rect.y >= onscreen_rect.y &&
      cursor_rect.y < onscreen_rect.y + onscreen_rect.height)
    {    
      btk_text_view_buffer_to_window_coords (text_view,
                                             BTK_TEXT_WINDOW_WIDGET,
                                             cursor_rect.x, cursor_rect.y,
                                             &cursor_rect.x, &cursor_rect.y);

      *x = root_x + cursor_rect.x + cursor_rect.width;
      *y = root_y + cursor_rect.y + cursor_rect.height;
    }
  else
    {
      /* Just center the menu, since cursor is offscreen. */      
      *x = root_x + (widget->allocation.width / 2 - req.width / 2);
      *y = root_y + (widget->allocation.height / 2 - req.height / 2);      
    }
  
  /* Ensure sanity */
  *x = CLAMP (*x, root_x, (root_x + widget->allocation.width));
  *y = CLAMP (*y, root_y, (root_y + widget->allocation.height));

  monitor_num = bdk_screen_get_monitor_at_point (screen, *x, *y);
  btk_menu_set_monitor (menu, monitor_num);
  bdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

  *x = CLAMP (*x, monitor.x, monitor.x + MAX (0, monitor.width - req.width));
  *y = CLAMP (*y, monitor.y, monitor.y + MAX (0, monitor.height - req.height));

  *push_in = FALSE;
}

typedef struct
{
  BtkTextView *text_view;
  gint button;
  guint time;
} PopupInfo;

static gboolean
range_contains_editable_text (const BtkTextIter *start,
                              const BtkTextIter *end,
                              gboolean default_editability)
{
  BtkTextIter iter = *start;

  while (btk_text_iter_compare (&iter, end) < 0)
    {
      if (btk_text_iter_editable (&iter, default_editability))
        return TRUE;
      
      btk_text_iter_forward_to_tag_toggle (&iter, NULL);
    }

  return FALSE;
}                             

static void
unichar_chosen_func (const char *text,
                     gpointer    data)
{
  BtkTextView *text_view = BTK_TEXT_VIEW (data);

  btk_text_view_commit_text (text_view, text);
}

static void
popup_targets_received (BtkClipboard     *clipboard,
			BtkSelectionData *data,
			gpointer          user_data)
{
  PopupInfo *info = user_data;
  BtkTextView *text_view = info->text_view;
  
  if (btk_widget_get_realized (BTK_WIDGET (text_view)))
    {
      /* We implicitely rely here on the fact that if we are pasting ourself, we'll
       * have text targets as well as the private BTK_TEXT_BUFFER_CONTENTS target.
       */
      gboolean clipboard_contains_text;
      BtkWidget *menuitem;
      BtkWidget *submenu;
      gboolean have_selection;
      gboolean can_insert;
      BtkTextIter iter;
      BtkTextIter sel_start, sel_end;
      gboolean show_input_method_menu;
      gboolean show_unicode_menu;
      
      clipboard_contains_text = btk_selection_data_targets_include_text (data);

      if (text_view->popup_menu)
	btk_widget_destroy (text_view->popup_menu);

      text_view->popup_menu = btk_menu_new ();
      
      btk_menu_attach_to_widget (BTK_MENU (text_view->popup_menu),
				 BTK_WIDGET (text_view),
				 popup_menu_detach);
      
      have_selection = btk_text_buffer_get_selection_bounds (get_buffer (text_view),
                                                             &sel_start, &sel_end);
      
      btk_text_buffer_get_iter_at_mark (get_buffer (text_view),
					&iter,
					btk_text_buffer_get_insert (get_buffer (text_view)));
      
      can_insert = btk_text_iter_can_insert (&iter, text_view->editable);
      
      append_action_signal (text_view, text_view->popup_menu, BTK_STOCK_CUT, "cut-clipboard",
			    have_selection &&
                            range_contains_editable_text (&sel_start, &sel_end,
                                                          text_view->editable));
      append_action_signal (text_view, text_view->popup_menu, BTK_STOCK_COPY, "copy-clipboard",
			    have_selection);
      append_action_signal (text_view, text_view->popup_menu, BTK_STOCK_PASTE, "paste-clipboard",
			    can_insert && clipboard_contains_text);
      
      menuitem = btk_image_menu_item_new_from_stock (BTK_STOCK_DELETE, NULL);
      btk_widget_set_sensitive (menuitem, 
				have_selection &&
				range_contains_editable_text (&sel_start, &sel_end,
							      text_view->editable));
      g_signal_connect_swapped (menuitem, "activate",
			        G_CALLBACK (delete_cb), text_view);
      btk_widget_show (menuitem);
      btk_menu_shell_append (BTK_MENU_SHELL (text_view->popup_menu), menuitem);

      menuitem = btk_separator_menu_item_new ();
      btk_widget_show (menuitem);
      btk_menu_shell_append (BTK_MENU_SHELL (text_view->popup_menu), menuitem);

      menuitem = btk_image_menu_item_new_from_stock (BTK_STOCK_SELECT_ALL, NULL);
      g_signal_connect (menuitem, "activate",
			G_CALLBACK (select_all_cb), text_view);
      btk_widget_show (menuitem);
      btk_menu_shell_append (BTK_MENU_SHELL (text_view->popup_menu), menuitem);

      g_object_get (btk_widget_get_settings (BTK_WIDGET (text_view)),
                    "btk-show-input-method-menu", &show_input_method_menu,
                    "btk-show-unicode-menu", &show_unicode_menu,
                    NULL);
      
      if (show_input_method_menu || show_unicode_menu)
        {
	  menuitem = btk_separator_menu_item_new ();
	  btk_widget_show (menuitem);
	  btk_menu_shell_append (BTK_MENU_SHELL (text_view->popup_menu), menuitem);
	}

      if (show_input_method_menu)
        {
	  menuitem = btk_menu_item_new_with_mnemonic (_("Input _Methods"));
	  btk_widget_show (menuitem);
	  btk_widget_set_sensitive (menuitem, can_insert);

	  submenu = btk_menu_new ();
	  btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), submenu);
	  btk_menu_shell_append (BTK_MENU_SHELL (text_view->popup_menu), menuitem);
	  
	  btk_im_multicontext_append_menuitems (BTK_IM_MULTICONTEXT (text_view->im_context),
						BTK_MENU_SHELL (submenu));
	}

      if (show_unicode_menu)
        {
	  menuitem = btk_menu_item_new_with_mnemonic (_("_Insert Unicode Control Character"));
	  btk_widget_show (menuitem);
	  btk_widget_set_sensitive (menuitem, can_insert);
      
	  submenu = btk_menu_new ();
	  btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), submenu);
	  btk_menu_shell_append (BTK_MENU_SHELL (text_view->popup_menu), menuitem);      
	  
	  _btk_text_util_append_special_char_menuitems (BTK_MENU_SHELL (submenu),
							unichar_chosen_func,
							text_view);
	}
	  
      g_signal_emit (text_view,
		     signals[POPULATE_POPUP],
		     0,
		     text_view->popup_menu);
      
      if (info->button)
	btk_menu_popup (BTK_MENU (text_view->popup_menu), NULL, NULL,
			NULL, NULL,
			info->button, info->time);
      else
	{
	  btk_menu_popup (BTK_MENU (text_view->popup_menu), NULL, NULL,
			  popup_position_func, text_view,
			  0, btk_get_current_event_time ());
	  btk_menu_shell_select_first (BTK_MENU_SHELL (text_view->popup_menu), FALSE);
	}
    }

  g_object_unref (text_view);
  g_free (info);
}

static void
btk_text_view_do_popup (BtkTextView    *text_view,
                        BdkEventButton *event)
{
  PopupInfo *info = g_new (PopupInfo, 1);

  /* In order to know what entries we should make sensitive, we
   * ask for the current targets of the clipboard, and when
   * we get them, then we actually pop up the menu.
   */
  info->text_view = g_object_ref (text_view);
  
  if (event)
    {
      info->button = event->button;
      info->time = event->time;
    }
  else
    {
      info->button = 0;
      info->time = btk_get_current_event_time ();
    }

  btk_clipboard_request_contents (btk_widget_get_clipboard (BTK_WIDGET (text_view),
							    BDK_SELECTION_CLIPBOARD),
				  bdk_atom_intern_static_string ("TARGETS"),
				  popup_targets_received,
				  info);
}

static gboolean
btk_text_view_popup_menu (BtkWidget *widget)
{
  btk_text_view_do_popup (BTK_TEXT_VIEW (widget), NULL);  
  return TRUE;
}

/* Child BdkWindows */


static BtkTextWindow*
text_window_new (BtkTextWindowType  type,
                 BtkWidget         *widget,
                 gint               width_request,
                 gint               height_request)
{
  BtkTextWindow *win;

  win = g_new (BtkTextWindow, 1);

  win->type = type;
  win->widget = widget;
  win->window = NULL;
  win->bin_window = NULL;
  win->requisition.width = width_request;
  win->requisition.height = height_request;
  win->allocation.width = width_request;
  win->allocation.height = height_request;
  win->allocation.x = 0;
  win->allocation.y = 0;

  return win;
}

static void
text_window_free (BtkTextWindow *win)
{
  if (win->window)
    text_window_unrealize (win);

  g_free (win);
}

static void
text_window_realize (BtkTextWindow *win,
                     BtkWidget     *widget)
{
  BdkWindowAttr attributes;
  gint attributes_mask;
  BdkCursor *cursor;

  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.x = win->allocation.x;
  attributes.y = win->allocation.y;
  attributes.width = win->allocation.width;
  attributes.height = win->allocation.height;
  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.visual = btk_widget_get_visual (win->widget);
  attributes.colormap = btk_widget_get_colormap (win->widget);
  attributes.event_mask = BDK_VISIBILITY_NOTIFY_MASK;

  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;

  win->window = bdk_window_new (widget->window,
                                &attributes,
                                attributes_mask);

  bdk_window_set_back_pixmap (win->window, NULL, FALSE);
  
  bdk_window_show (win->window);
  bdk_window_set_user_data (win->window, win->widget);
  bdk_window_lower (win->window);

  attributes.x = 0;
  attributes.y = 0;
  attributes.width = win->allocation.width;
  attributes.height = win->allocation.height;
  attributes.event_mask = (BDK_EXPOSURE_MASK            |
                           BDK_SCROLL_MASK              |
                           BDK_KEY_PRESS_MASK           |
                           BDK_BUTTON_PRESS_MASK        |
                           BDK_BUTTON_RELEASE_MASK      |
                           BDK_POINTER_MOTION_MASK      |
                           BDK_POINTER_MOTION_HINT_MASK |
                           btk_widget_get_events (win->widget));

  win->bin_window = bdk_window_new (win->window,
                                    &attributes,
                                    attributes_mask);

  bdk_window_show (win->bin_window);
  bdk_window_set_user_data (win->bin_window, win->widget);

  if (win->type == BTK_TEXT_WINDOW_TEXT)
    {
      if (btk_widget_is_sensitive (widget))
        {
          /* I-beam cursor */
          cursor = bdk_cursor_new_for_display (bdk_window_get_display (widget->window),
					       BDK_XTERM);
          bdk_window_set_cursor (win->bin_window, cursor);
          bdk_cursor_unref (cursor);
        } 

      btk_im_context_set_client_window (BTK_TEXT_VIEW (widget)->im_context,
                                        win->window);


      bdk_window_set_background (win->bin_window,
                                 &widget->style->base[btk_widget_get_state (widget)]);
    }
  else
    {
      bdk_window_set_background (win->bin_window,
                                 &widget->style->bg[btk_widget_get_state (widget)]);
    }

  g_object_set_qdata (G_OBJECT (win->window),
                      g_quark_from_static_string ("btk-text-view-text-window"),
                      win);

  g_object_set_qdata (G_OBJECT (win->bin_window),
                      g_quark_from_static_string ("btk-text-view-text-window"),
                      win);
}

static void
text_window_unrealize (BtkTextWindow *win)
{
  if (win->type == BTK_TEXT_WINDOW_TEXT)
    {
      btk_im_context_set_client_window (BTK_TEXT_VIEW (win->widget)->im_context,
                                        NULL);
    }

  bdk_window_set_user_data (win->window, NULL);
  bdk_window_set_user_data (win->bin_window, NULL);
  bdk_window_destroy (win->bin_window);
  bdk_window_destroy (win->window);
  win->window = NULL;
  win->bin_window = NULL;
}

static void
text_window_size_allocate (BtkTextWindow *win,
                           BdkRectangle  *rect)
{
  win->allocation = *rect;

  if (win->window)
    {
      bdk_window_move_resize (win->window,
                              rect->x, rect->y,
                              rect->width, rect->height);

      bdk_window_resize (win->bin_window,
                         rect->width, rect->height);
    }
}

static void
text_window_scroll        (BtkTextWindow *win,
                           gint           dx,
                           gint           dy)
{
  if (dx != 0 || dy != 0)
    {
      bdk_window_scroll (win->bin_window, dx, dy);
    }
}

static void
text_window_invalidate_rect (BtkTextWindow *win,
                             BdkRectangle  *rect)
{
  BdkRectangle window_rect;

  btk_text_view_buffer_to_window_coords (BTK_TEXT_VIEW (win->widget),
                                         win->type,
                                         rect->x,
                                         rect->y,
                                         &window_rect.x,
                                         &window_rect.y);

  window_rect.width = rect->width;
  window_rect.height = rect->height;
  
  /* Adjust the rect as appropriate */
  
  switch (win->type)
    {
    case BTK_TEXT_WINDOW_TEXT:
      break;

    case BTK_TEXT_WINDOW_LEFT:
    case BTK_TEXT_WINDOW_RIGHT:
      window_rect.x = 0;
      window_rect.width = win->allocation.width;
      break;

    case BTK_TEXT_WINDOW_TOP:
    case BTK_TEXT_WINDOW_BOTTOM:
      window_rect.y = 0;
      window_rect.height = win->allocation.height;
      break;

    default:
      g_warning ("%s: bug!", B_STRFUNC);
      return;
      break;
    }
          
  bdk_window_invalidate_rect (win->bin_window, &window_rect, FALSE);

#if 0
  {
    bairo_t *cr = bdk_bairo_create (win->bin_window);
    bdk_bairo_rectangle (cr, &window_rect);
    bairo_set_source_rgb  (cr, 1.0, 0.0, 0.0);	/* red */
    bairo_fill (cr);
    bairo_destroy (cr);
  }
#endif
}

static void
text_window_invalidate_cursors (BtkTextWindow *win)
{
  BtkTextView *text_view = BTK_TEXT_VIEW (win->widget);
  BtkTextIter  iter;
  BdkRectangle strong;
  BdkRectangle weak;
  gboolean     draw_arrow;
  gfloat       cursor_aspect_ratio;
  gint         stem_width;
  gint         arrow_width;

  btk_text_buffer_get_iter_at_mark (text_view->buffer, &iter,
                                    btk_text_buffer_get_insert (text_view->buffer));

  if (_btk_text_layout_get_block_cursor (text_view->layout, &strong))
    {
      text_window_invalidate_rect (win, &strong);
      return;
    }

  btk_text_layout_get_cursor_locations (text_view->layout, &iter,
                                        &strong, &weak);

  /* cursor width calculation as in btkstyle.c:draw_insertion_cursor(),
   * ignoring the text direction be exposing both sides of the cursor
   */

  draw_arrow = (strong.x != weak.x || strong.y != weak.y);

  btk_widget_style_get (win->widget,
                        "cursor-aspect-ratio", &cursor_aspect_ratio,
                        NULL);
  
  stem_width = strong.height * cursor_aspect_ratio + 1;
  arrow_width = stem_width + 1;

  strong.width = stem_width;

  /* round up to the next even number */
  if (stem_width & 1)
    stem_width++;

  strong.x     -= stem_width / 2;
  strong.width += stem_width;

  if (draw_arrow)
    {
      strong.x     -= arrow_width;
      strong.width += arrow_width * 2;
    }

  text_window_invalidate_rect (win, &strong);

  if (draw_arrow) /* == have weak */
    {
      stem_width = weak.height * cursor_aspect_ratio + 1;
      arrow_width = stem_width + 1;

      weak.width = stem_width;

      /* round up to the next even number */
      if (stem_width & 1)
        stem_width++;

      weak.x     -= stem_width / 2;
      weak.width += stem_width;

      weak.x     -= arrow_width;
      weak.width += arrow_width * 2;

      text_window_invalidate_rect (win, &weak);
    }
}

static gint
text_window_get_width (BtkTextWindow *win)
{
  return win->allocation.width;
}

static gint
text_window_get_height (BtkTextWindow *win)
{
  return win->allocation.height;
}

/* Windows */


/**
 * btk_text_view_get_window:
 * @text_view: a #BtkTextView
 * @win: window to get
 *
 * Retrieves the #BdkWindow corresponding to an area of the text view;
 * possible windows include the overall widget window, child windows
 * on the left, right, top, bottom, and the window that displays the
 * text buffer. Windows are %NULL and nonexistent if their width or
 * height is 0, and are nonexistent before the widget has been
 * realized.
 *
 * Return value: (transfer none): a #BdkWindow, or %NULL
 **/
BdkWindow*
btk_text_view_get_window (BtkTextView *text_view,
                          BtkTextWindowType win)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), NULL);

  switch (win)
    {
    case BTK_TEXT_WINDOW_WIDGET:
      return BTK_WIDGET (text_view)->window;
      break;

    case BTK_TEXT_WINDOW_TEXT:
      return text_view->text_window->bin_window;
      break;

    case BTK_TEXT_WINDOW_LEFT:
      if (text_view->left_window)
        return text_view->left_window->bin_window;
      else
        return NULL;
      break;

    case BTK_TEXT_WINDOW_RIGHT:
      if (text_view->right_window)
        return text_view->right_window->bin_window;
      else
        return NULL;
      break;

    case BTK_TEXT_WINDOW_TOP:
      if (text_view->top_window)
        return text_view->top_window->bin_window;
      else
        return NULL;
      break;

    case BTK_TEXT_WINDOW_BOTTOM:
      if (text_view->bottom_window)
        return text_view->bottom_window->bin_window;
      else
        return NULL;
      break;

    case BTK_TEXT_WINDOW_PRIVATE:
      g_warning ("%s: You can't get BTK_TEXT_WINDOW_PRIVATE, it has \"PRIVATE\" in the name because it is private.", B_STRFUNC);
      return NULL;
      break;
    }

  g_warning ("%s: Unknown BtkTextWindowType", B_STRFUNC);
  return NULL;
}

/**
 * btk_text_view_get_window_type:
 * @text_view: a #BtkTextView
 * @window: a window type
 *
 * Usually used to find out which window an event corresponds to.
 * If you connect to an event signal on @text_view, this function
 * should be called on <literal>event-&gt;window</literal> to
 * see which window it was.
 *
 * Return value: the window type.
 **/
BtkTextWindowType
btk_text_view_get_window_type (BtkTextView *text_view,
                               BdkWindow   *window)
{
  BtkTextWindow *win;

  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), 0);
  g_return_val_if_fail (BDK_IS_WINDOW (window), 0);

  if (window == BTK_WIDGET (text_view)->window)
    return BTK_TEXT_WINDOW_WIDGET;

  win = g_object_get_qdata (G_OBJECT (window),
                            g_quark_try_string ("btk-text-view-text-window"));

  if (win)
    return win->type;
  else
    {
      return BTK_TEXT_WINDOW_PRIVATE;
    }
}

static void
buffer_to_widget (BtkTextView      *text_view,
                  gint              buffer_x,
                  gint              buffer_y,
                  gint             *window_x,
                  gint             *window_y)
{  
  if (window_x)
    {
      *window_x = buffer_x - text_view->xoffset;
      *window_x += text_view->text_window->allocation.x;
    }

  if (window_y)
    {
      *window_y = buffer_y - text_view->yoffset;
      *window_y += text_view->text_window->allocation.y;
    }
}

static void
widget_to_text_window (BtkTextWindow *win,
                       gint           widget_x,
                       gint           widget_y,
                       gint          *window_x,
                       gint          *window_y)
{
  if (window_x)
    *window_x = widget_x - win->allocation.x;

  if (window_y)
    *window_y = widget_y - win->allocation.y;
}

static void
buffer_to_text_window (BtkTextView   *text_view,
                       BtkTextWindow *win,
                       gint           buffer_x,
                       gint           buffer_y,
                       gint          *window_x,
                       gint          *window_y)
{
  if (win == NULL)
    {
      g_warning ("Attempt to convert text buffer coordinates to coordinates "
                 "for a nonexistent or private child window of BtkTextView");
      return;
    }

  buffer_to_widget (text_view,
                    buffer_x, buffer_y,
                    window_x, window_y);

  widget_to_text_window (win,
                         window_x ? *window_x : 0,
                         window_y ? *window_y : 0,
                         window_x,
                         window_y);
}

/**
 * btk_text_view_buffer_to_window_coords:
 * @text_view: a #BtkTextView
 * @win: a #BtkTextWindowType except #BTK_TEXT_WINDOW_PRIVATE
 * @buffer_x: buffer x coordinate
 * @buffer_y: buffer y coordinate
 * @window_x: (out) (allow-none): window x coordinate return location or %NULL
 * @window_y: (out) (allow-none): window y coordinate return location or %NULL
 *
 * Converts coordinate (@buffer_x, @buffer_y) to coordinates for the window
 * @win, and stores the result in (@window_x, @window_y). 
 *
 * Note that you can't convert coordinates for a nonexisting window (see 
 * btk_text_view_set_border_window_size()).
 **/
void
btk_text_view_buffer_to_window_coords (BtkTextView      *text_view,
                                       BtkTextWindowType win,
                                       gint              buffer_x,
                                       gint              buffer_y,
                                       gint             *window_x,
                                       gint             *window_y)
{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));

  switch (win)
    {
    case BTK_TEXT_WINDOW_WIDGET:
      buffer_to_widget (text_view,
                        buffer_x, buffer_y,
                        window_x, window_y);
      break;

    case BTK_TEXT_WINDOW_TEXT:
      if (window_x)
        *window_x = buffer_x - text_view->xoffset;
      if (window_y)
        *window_y = buffer_y - text_view->yoffset;
      break;

    case BTK_TEXT_WINDOW_LEFT:
      buffer_to_text_window (text_view,
                             text_view->left_window,
                             buffer_x, buffer_y,
                             window_x, window_y);
      break;

    case BTK_TEXT_WINDOW_RIGHT:
      buffer_to_text_window (text_view,
                             text_view->right_window,
                             buffer_x, buffer_y,
                             window_x, window_y);
      break;

    case BTK_TEXT_WINDOW_TOP:
      buffer_to_text_window (text_view,
                             text_view->top_window,
                             buffer_x, buffer_y,
                             window_x, window_y);
      break;

    case BTK_TEXT_WINDOW_BOTTOM:
      buffer_to_text_window (text_view,
                             text_view->bottom_window,
                             buffer_x, buffer_y,
                             window_x, window_y);
      break;

    case BTK_TEXT_WINDOW_PRIVATE:
      g_warning ("%s: can't get coords for private windows", B_STRFUNC);
      break;

    default:
      g_warning ("%s: Unknown BtkTextWindowType", B_STRFUNC);
      break;
    }
}

static void
widget_to_buffer (BtkTextView *text_view,
                  gint         widget_x,
                  gint         widget_y,
                  gint        *buffer_x,
                  gint        *buffer_y)
{  
  if (buffer_x)
    {
      *buffer_x = widget_x + text_view->xoffset;
      *buffer_x -= text_view->text_window->allocation.x;
    }

  if (buffer_y)
    {
      *buffer_y = widget_y + text_view->yoffset;
      *buffer_y -= text_view->text_window->allocation.y;
    }
}

static void
text_window_to_widget (BtkTextWindow *win,
                       gint           window_x,
                       gint           window_y,
                       gint          *widget_x,
                       gint          *widget_y)
{
  if (widget_x)
    *widget_x = window_x + win->allocation.x;

  if (widget_y)
    *widget_y = window_y + win->allocation.y;
}

static void
text_window_to_buffer (BtkTextView   *text_view,
                       BtkTextWindow *win,
                       gint           window_x,
                       gint           window_y,
                       gint          *buffer_x,
                       gint          *buffer_y)
{
  if (win == NULL)
    {
      g_warning ("Attempt to convert BtkTextView buffer coordinates into "
                 "coordinates for a nonexistent child window.");
      return;
    }

  text_window_to_widget (win,
                         window_x,
                         window_y,
                         buffer_x,
                         buffer_y);

  widget_to_buffer (text_view,
                    buffer_x ? *buffer_x : 0,
                    buffer_y ? *buffer_y : 0,
                    buffer_x,
                    buffer_y);
}

/**
 * btk_text_view_window_to_buffer_coords:
 * @text_view: a #BtkTextView
 * @win: a #BtkTextWindowType except #BTK_TEXT_WINDOW_PRIVATE
 * @window_x: window x coordinate
 * @window_y: window y coordinate
 * @buffer_x: (out) (allow-none): buffer x coordinate return location or %NULL
 * @buffer_y: (out) (allow-none): buffer y coordinate return location or %NULL
 *
 * Converts coordinates on the window identified by @win to buffer
 * coordinates, storing the result in (@buffer_x,@buffer_y).
 *
 * Note that you can't convert coordinates for a nonexisting window (see 
 * btk_text_view_set_border_window_size()).
 **/
void
btk_text_view_window_to_buffer_coords (BtkTextView      *text_view,
                                       BtkTextWindowType win,
                                       gint              window_x,
                                       gint              window_y,
                                       gint             *buffer_x,
                                       gint             *buffer_y)
{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));

  switch (win)
    {
    case BTK_TEXT_WINDOW_WIDGET:
      widget_to_buffer (text_view,
                        window_x, window_y,
                        buffer_x, buffer_y);
      break;

    case BTK_TEXT_WINDOW_TEXT:
      if (buffer_x)
        *buffer_x = window_x + text_view->xoffset;
      if (buffer_y)
        *buffer_y = window_y + text_view->yoffset;
      break;

    case BTK_TEXT_WINDOW_LEFT:
      text_window_to_buffer (text_view,
                             text_view->left_window,
                             window_x, window_y,
                             buffer_x, buffer_y);
      break;

    case BTK_TEXT_WINDOW_RIGHT:
      text_window_to_buffer (text_view,
                             text_view->right_window,
                             window_x, window_y,
                             buffer_x, buffer_y);
      break;

    case BTK_TEXT_WINDOW_TOP:
      text_window_to_buffer (text_view,
                             text_view->top_window,
                             window_x, window_y,
                             buffer_x, buffer_y);
      break;

    case BTK_TEXT_WINDOW_BOTTOM:
      text_window_to_buffer (text_view,
                             text_view->bottom_window,
                             window_x, window_y,
                             buffer_x, buffer_y);
      break;

    case BTK_TEXT_WINDOW_PRIVATE:
      g_warning ("%s: can't get coords for private windows", B_STRFUNC);
      break;

    default:
      g_warning ("%s: Unknown BtkTextWindowType", B_STRFUNC);
      break;
    }
}

static void
set_window_width (BtkTextView      *text_view,
                  gint              width,
                  BtkTextWindowType type,
                  BtkTextWindow   **winp)
{
  if (width == 0)
    {
      if (*winp)
        {
          text_window_free (*winp);
          *winp = NULL;
          btk_widget_queue_resize (BTK_WIDGET (text_view));
        }
    }
  else
    {
      if (*winp == NULL)
        {
          *winp = text_window_new (type,
                                   BTK_WIDGET (text_view),
                                   width, 0);
          /* if the widget is already realized we need to realize the child manually */
          if (btk_widget_get_realized (BTK_WIDGET (text_view)))
            text_window_realize (*winp, BTK_WIDGET (text_view));
        }
      else
        {
          if ((*winp)->requisition.width == width)
            return;

          (*winp)->requisition.width = width;
        }

      btk_widget_queue_resize (BTK_WIDGET (text_view));
    }
}


static void
set_window_height (BtkTextView      *text_view,
                   gint              height,
                   BtkTextWindowType type,
                   BtkTextWindow   **winp)
{
  if (height == 0)
    {
      if (*winp)
        {
          text_window_free (*winp);
          *winp = NULL;
          btk_widget_queue_resize (BTK_WIDGET (text_view));
        }
    }
  else
    {
      if (*winp == NULL)
        {
          *winp = text_window_new (type,
                                   BTK_WIDGET (text_view),
                                   0, height);

          /* if the widget is already realized we need to realize the child manually */
          if (btk_widget_get_realized (BTK_WIDGET (text_view)))
            text_window_realize (*winp, BTK_WIDGET (text_view));
        }
      else
        {
          if ((*winp)->requisition.height == height)
            return;

          (*winp)->requisition.height = height;
        }

      btk_widget_queue_resize (BTK_WIDGET (text_view));
    }
}

/**
 * btk_text_view_set_border_window_size:
 * @text_view: a #BtkTextView
 * @type: window to affect
 * @size: width or height of the window
 *
 * Sets the width of %BTK_TEXT_WINDOW_LEFT or %BTK_TEXT_WINDOW_RIGHT,
 * or the height of %BTK_TEXT_WINDOW_TOP or %BTK_TEXT_WINDOW_BOTTOM.
 * Automatically destroys the corresponding window if the size is set
 * to 0, and creates the window if the size is set to non-zero.  This
 * function can only be used for the "border windows," it doesn't work
 * with #BTK_TEXT_WINDOW_WIDGET, #BTK_TEXT_WINDOW_TEXT, or
 * #BTK_TEXT_WINDOW_PRIVATE.
 **/
void
btk_text_view_set_border_window_size (BtkTextView      *text_view,
                                      BtkTextWindowType type,
                                      gint              size)

{
  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));
  g_return_if_fail (size >= 0);

  switch (type)
    {
    case BTK_TEXT_WINDOW_LEFT:
      set_window_width (text_view, size, BTK_TEXT_WINDOW_LEFT,
                        &text_view->left_window);
      break;

    case BTK_TEXT_WINDOW_RIGHT:
      set_window_width (text_view, size, BTK_TEXT_WINDOW_RIGHT,
                        &text_view->right_window);
      break;

    case BTK_TEXT_WINDOW_TOP:
      set_window_height (text_view, size, BTK_TEXT_WINDOW_TOP,
                         &text_view->top_window);
      break;

    case BTK_TEXT_WINDOW_BOTTOM:
      set_window_height (text_view, size, BTK_TEXT_WINDOW_BOTTOM,
                         &text_view->bottom_window);
      break;

    default:
      g_warning ("Can only set size of left/right/top/bottom border windows with btk_text_view_set_border_window_size()");
      break;
    }
}

/**
 * btk_text_view_get_border_window_size:
 * @text_view: a #BtkTextView
 * @type: window to return size from
 *
 * Gets the width of the specified border window. See
 * btk_text_view_set_border_window_size().
 *
 * Return value: width of window
 **/
gint
btk_text_view_get_border_window_size (BtkTextView       *text_view,
				      BtkTextWindowType  type)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), 0);
  
  switch (type)
    {
    case BTK_TEXT_WINDOW_LEFT:
      if (text_view->left_window)
        return text_view->left_window->requisition.width;
      break;
      
    case BTK_TEXT_WINDOW_RIGHT:
      if (text_view->right_window)
        return text_view->right_window->requisition.width;
      break;
      
    case BTK_TEXT_WINDOW_TOP:
      if (text_view->top_window)
        return text_view->top_window->requisition.height;
      break;

    case BTK_TEXT_WINDOW_BOTTOM:
      if (text_view->bottom_window)
        return text_view->bottom_window->requisition.height;
      break;
      
    default:
      g_warning ("Can only get size of left/right/top/bottom border windows with btk_text_view_get_border_window_size()");
      break;
    }

  return 0;
}

/*
 * Child widgets
 */

static BtkTextViewChild*
text_view_child_new_anchored (BtkWidget          *child,
                              BtkTextChildAnchor *anchor,
                              BtkTextLayout      *layout)
{
  BtkTextViewChild *vc;

  vc = g_new (BtkTextViewChild, 1);

  vc->type = BTK_TEXT_WINDOW_PRIVATE;
  vc->widget = child;
  vc->anchor = anchor;

  vc->from_top_of_line = 0;
  vc->from_left_of_buffer = 0;
  
  g_object_ref (vc->widget);
  g_object_ref (vc->anchor);

  g_object_set_data (G_OBJECT (child),
                     I_("btk-text-view-child"),
                     vc);

  btk_text_child_anchor_register_child (anchor, child, layout);
  
  return vc;
}

static BtkTextViewChild*
text_view_child_new_window (BtkWidget          *child,
                            BtkTextWindowType   type,
                            gint                x,
                            gint                y)
{
  BtkTextViewChild *vc;

  vc = g_new (BtkTextViewChild, 1);

  vc->widget = child;
  vc->anchor = NULL;

  vc->from_top_of_line = 0;
  vc->from_left_of_buffer = 0;
 
  g_object_ref (vc->widget);

  vc->type = type;
  vc->x = x;
  vc->y = y;

  g_object_set_data (G_OBJECT (child),
                     I_("btk-text-view-child"),
                     vc);
  
  return vc;
}

static void
text_view_child_free (BtkTextViewChild *child)
{
  g_object_set_data (G_OBJECT (child->widget),
                     I_("btk-text-view-child"), NULL);

  if (child->anchor)
    {
      btk_text_child_anchor_unregister_child (child->anchor,
                                              child->widget);
      g_object_unref (child->anchor);
    }

  g_object_unref (child->widget);

  g_free (child);
}

static void
text_view_child_set_parent_window (BtkTextView      *text_view,
				   BtkTextViewChild *vc)
{
  if (vc->anchor)
    btk_widget_set_parent_window (vc->widget,
                                  text_view->text_window->bin_window);
  else
    {
      BdkWindow *window;
      window = btk_text_view_get_window (text_view,
                                         vc->type);
      btk_widget_set_parent_window (vc->widget, window);
    }
}

static void
add_child (BtkTextView      *text_view,
           BtkTextViewChild *vc)
{
  text_view->children = g_slist_prepend (text_view->children,
                                         vc);

  if (btk_widget_get_realized (BTK_WIDGET (text_view)))
    text_view_child_set_parent_window (text_view, vc);
  
  btk_widget_set_parent (vc->widget, BTK_WIDGET (text_view));
}

/**
 * btk_text_view_add_child_at_anchor:
 * @text_view: a #BtkTextView
 * @child: a #BtkWidget
 * @anchor: a #BtkTextChildAnchor in the #BtkTextBuffer for @text_view
 * 
 * Adds a child widget in the text buffer, at the given @anchor.
 **/
void
btk_text_view_add_child_at_anchor (BtkTextView          *text_view,
                                   BtkWidget            *child,
                                   BtkTextChildAnchor   *anchor)
{
  BtkTextViewChild *vc;

  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));
  g_return_if_fail (BTK_IS_WIDGET (child));
  g_return_if_fail (BTK_IS_TEXT_CHILD_ANCHOR (anchor));
  g_return_if_fail (child->parent == NULL);

  btk_text_view_ensure_layout (text_view);

  vc = text_view_child_new_anchored (child, anchor,
                                     text_view->layout);

  add_child (text_view, vc);

  g_assert (vc->widget == child);
  g_assert (btk_widget_get_parent (child) == BTK_WIDGET (text_view));
}

/**
 * btk_text_view_add_child_in_window:
 * @text_view: a #BtkTextView
 * @child: a #BtkWidget
 * @which_window: which window the child should appear in
 * @xpos: X position of child in window coordinates
 * @ypos: Y position of child in window coordinates
 *
 * Adds a child at fixed coordinates in one of the text widget's
 * windows. The window must have nonzero size (see
 * btk_text_view_set_border_window_size()). Note that the child
 * coordinates are given relative to the #BdkWindow in question, and
 * that these coordinates have no sane relationship to scrolling. When
 * placing a child in #BTK_TEXT_WINDOW_WIDGET, scrolling is
 * irrelevant, the child floats above all scrollable areas. But when
 * placing a child in one of the scrollable windows (border windows or
 * text window), you'll need to compute the child's correct position
 * in buffer coordinates any time scrolling occurs or buffer changes
 * occur, and then call btk_text_view_move_child() to update the
 * child's position. Unfortunately there's no good way to detect that
 * scrolling has occurred, using the current API; a possible hack
 * would be to update all child positions when the scroll adjustments
 * change or the text buffer changes. See bug 64518 on
 * bugzilla.bunny.org for status of fixing this issue.
 **/
void
btk_text_view_add_child_in_window (BtkTextView       *text_view,
                                   BtkWidget         *child,
                                   BtkTextWindowType  which_window,
                                   gint               xpos,
                                   gint               ypos)
{
  BtkTextViewChild *vc;

  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));
  g_return_if_fail (BTK_IS_WIDGET (child));
  g_return_if_fail (child->parent == NULL);

  vc = text_view_child_new_window (child, which_window,
                                   xpos, ypos);

  add_child (text_view, vc);

  g_assert (vc->widget == child);
  g_assert (btk_widget_get_parent (child) == BTK_WIDGET (text_view));
}

/**
 * btk_text_view_move_child:
 * @text_view: a #BtkTextView
 * @child: child widget already added to the text view
 * @xpos: new X position in window coordinates
 * @ypos: new Y position in window coordinates
 *
 * Updates the position of a child, as for btk_text_view_add_child_in_window().
 **/
void
btk_text_view_move_child (BtkTextView *text_view,
                          BtkWidget   *child,
                          gint         xpos,
                          gint         ypos)
{
  BtkTextViewChild *vc;

  g_return_if_fail (BTK_IS_TEXT_VIEW (text_view));
  g_return_if_fail (BTK_IS_WIDGET (child));
  g_return_if_fail (child->parent == (BtkWidget*) text_view);

  vc = g_object_get_data (G_OBJECT (child),
                          "btk-text-view-child");

  g_assert (vc != NULL);

  if (vc->x == xpos &&
      vc->y == ypos)
    return;
  
  vc->x = xpos;
  vc->y = ypos;

  if (btk_widget_get_visible (child) &&
      btk_widget_get_visible (BTK_WIDGET (text_view)))
    btk_widget_queue_resize (child);
}


/* Iterator operations */

/**
 * btk_text_view_forward_display_line:
 * @text_view: a #BtkTextView
 * @iter: a #BtkTextIter
 * 
 * Moves the given @iter forward by one display (wrapped) line.
 * A display line is different from a paragraph. Paragraphs are
 * separated by newlines or other paragraph separator characters.
 * Display lines are created by line-wrapping a paragraph. If
 * wrapping is turned off, display lines and paragraphs will be the
 * same. Display lines are divided differently for each view, since
 * they depend on the view's width; paragraphs are the same in all
 * views, since they depend on the contents of the #BtkTextBuffer.
 * 
 * Return value: %TRUE if @iter was moved and is not on the end iterator
 **/
gboolean
btk_text_view_forward_display_line (BtkTextView *text_view,
                                    BtkTextIter *iter)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  btk_text_view_ensure_layout (text_view);

  return btk_text_layout_move_iter_to_next_line (text_view->layout, iter);
}

/**
 * btk_text_view_backward_display_line:
 * @text_view: a #BtkTextView
 * @iter: a #BtkTextIter
 * 
 * Moves the given @iter backward by one display (wrapped) line.
 * A display line is different from a paragraph. Paragraphs are
 * separated by newlines or other paragraph separator characters.
 * Display lines are created by line-wrapping a paragraph. If
 * wrapping is turned off, display lines and paragraphs will be the
 * same. Display lines are divided differently for each view, since
 * they depend on the view's width; paragraphs are the same in all
 * views, since they depend on the contents of the #BtkTextBuffer.
 * 
 * Return value: %TRUE if @iter was moved and is not on the end iterator
 **/
gboolean
btk_text_view_backward_display_line (BtkTextView *text_view,
                                     BtkTextIter *iter)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  btk_text_view_ensure_layout (text_view);

  return btk_text_layout_move_iter_to_previous_line (text_view->layout, iter);
}

/**
 * btk_text_view_forward_display_line_end:
 * @text_view: a #BtkTextView
 * @iter: a #BtkTextIter
 * 
 * Moves the given @iter forward to the next display line end.
 * A display line is different from a paragraph. Paragraphs are
 * separated by newlines or other paragraph separator characters.
 * Display lines are created by line-wrapping a paragraph. If
 * wrapping is turned off, display lines and paragraphs will be the
 * same. Display lines are divided differently for each view, since
 * they depend on the view's width; paragraphs are the same in all
 * views, since they depend on the contents of the #BtkTextBuffer.
 * 
 * Return value: %TRUE if @iter was moved and is not on the end iterator
 **/
gboolean
btk_text_view_forward_display_line_end (BtkTextView *text_view,
                                        BtkTextIter *iter)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  btk_text_view_ensure_layout (text_view);

  return btk_text_layout_move_iter_to_line_end (text_view->layout, iter, 1);
}

/**
 * btk_text_view_backward_display_line_start:
 * @text_view: a #BtkTextView
 * @iter: a #BtkTextIter
 * 
 * Moves the given @iter backward to the next display line start.
 * A display line is different from a paragraph. Paragraphs are
 * separated by newlines or other paragraph separator characters.
 * Display lines are created by line-wrapping a paragraph. If
 * wrapping is turned off, display lines and paragraphs will be the
 * same. Display lines are divided differently for each view, since
 * they depend on the view's width; paragraphs are the same in all
 * views, since they depend on the contents of the #BtkTextBuffer.
 * 
 * Return value: %TRUE if @iter was moved and is not on the end iterator
 **/
gboolean
btk_text_view_backward_display_line_start (BtkTextView *text_view,
                                           BtkTextIter *iter)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  btk_text_view_ensure_layout (text_view);

  return btk_text_layout_move_iter_to_line_end (text_view->layout, iter, -1);
}

/**
 * btk_text_view_starts_display_line:
 * @text_view: a #BtkTextView
 * @iter: a #BtkTextIter
 * 
 * Determines whether @iter is at the start of a display line.
 * See btk_text_view_forward_display_line() for an explanation of
 * display lines vs. paragraphs.
 * 
 * Return value: %TRUE if @iter begins a wrapped line
 **/
gboolean
btk_text_view_starts_display_line (BtkTextView       *text_view,
                                   const BtkTextIter *iter)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  btk_text_view_ensure_layout (text_view);

  return btk_text_layout_iter_starts_line (text_view->layout, iter);
}

/**
 * btk_text_view_move_visually:
 * @text_view: a #BtkTextView
 * @iter: a #BtkTextIter
 * @count: number of characters to move (negative moves left, 
 *    positive moves right)
 *
 * Move the iterator a given number of characters visually, treating
 * it as the strong cursor position. If @count is positive, then the
 * new strong cursor position will be @count positions to the right of
 * the old cursor position. If @count is negative then the new strong
 * cursor position will be @count positions to the left of the old
 * cursor position.
 *
 * In the presence of bi-directional text, the correspondence
 * between logical and visual order will depend on the direction
 * of the current run, and there may be jumps when the cursor
 * is moved off of the end of a run.
 * 
 * Return value: %TRUE if @iter moved and is not on the end iterator
 **/
gboolean
btk_text_view_move_visually (BtkTextView *text_view,
                             BtkTextIter *iter,
                             gint         count)
{
  g_return_val_if_fail (BTK_IS_TEXT_VIEW (text_view), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  btk_text_view_ensure_layout (text_view);

  return btk_text_layout_move_iter_visually (text_view->layout, iter, count);
}

#define __BTK_TEXT_VIEW_C__
#include "btkaliasdef.c"
