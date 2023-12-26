/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 2004-2006 Christian Hammond
 * Copyright (C) 2008 Cody Russell
 * Copyright (C) 2008 Red Hat, Inc.
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

#include <math.h>
#include <string.h>

#include "bdk/bdkkeysyms.h"
#include "btkalignment.h"
#include "btkbindings.h"
#include "btkcelleditable.h"
#include "btkclipboard.h"
#include "btkdnd.h"
#include "btkentry.h"
#include "btkentrybuffer.h"
#include "btkimagemenuitem.h"
#include "btkimcontextsimple.h"
#include "btkimmulticontext.h"
#include "btkintl.h"
#include "btklabel.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkmenu.h"
#include "btkmenuitem.h"
#include "btkseparatormenuitem.h"
#include "btkselection.h"
#include "btksettings.h"
#include "btkspinbutton.h"
#include "btkstock.h"
#include "btktextutil.h"
#include "btkwindow.h"
#include "btktreeview.h"
#include "btktreeselection.h"
#include "btkprivate.h"
#include "btkentryprivate.h"
#include "btkcelllayout.h"
#include "btktooltip.h"
#include "btkiconfactory.h"
#include "btkicontheme.h"
#include "btkalias.h"

#define BTK_ENTRY_COMPLETION_KEY "btk-entry-completion-key"

#define MIN_ENTRY_WIDTH  150
#define DRAW_TIMEOUT     20
#define COMPLETION_TIMEOUT 300
#define PASSWORD_HINT_MAX 8

#define MAX_ICONS 2

#define IS_VALID_ICON_POSITION(pos)               \
  ((pos) == BTK_ENTRY_ICON_PRIMARY ||                   \
   (pos) == BTK_ENTRY_ICON_SECONDARY)

static const BtkBorder default_inner_border = { 2, 2, 2, 2 };
static GQuark          quark_inner_border   = 0;
static GQuark          quark_password_hint  = 0;
static GQuark          quark_cursor_hadjustment = 0;
static GQuark          quark_capslock_feedback = 0;

typedef struct _BtkEntryPrivate BtkEntryPrivate;

#define BTK_ENTRY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_ENTRY, BtkEntryPrivate))

typedef struct
{
  BdkWindow *window;
  gchar *tooltip;
  guint insensitive    : 1;
  guint nonactivatable : 1;
  guint prelight       : 1;
  guint in_drag        : 1;
  guint pressed        : 1;

  BtkImageType  storage_type;
  BdkPixbuf    *pixbuf;
  gchar        *stock_id;
  gchar        *icon_name;
  GIcon        *gicon;

  BtkTargetList *target_list;
  BdkDragAction actions;
} EntryIconInfo;

struct _BtkEntryPrivate 
{
  BtkEntryBuffer* buffer;

  gfloat xalign;
  gint insert_pos;
  guint blink_time;  /* time in msec the cursor has blinked since last user event */
  guint interior_focus          : 1;
  guint real_changed            : 1;
  guint invisible_char_set      : 1;
  guint caps_lock_warning       : 1;
  guint caps_lock_warning_shown : 1;
  guint change_count            : 8;
  guint progress_pulse_mode     : 1;
  guint progress_pulse_way_back : 1;

  gint focus_width;
  BtkShadowType shadow_type;

  gdouble progress_fraction;
  gdouble progress_pulse_fraction;
  gdouble progress_pulse_current;

  EntryIconInfo *icons[MAX_ICONS];
  gint icon_margin;
  gint start_x;
  gint start_y;

  gchar *im_module;
};

typedef struct _BtkEntryPasswordHint BtkEntryPasswordHint;

struct _BtkEntryPasswordHint
{
  gint position;      /* Position (in text) of the last password hint */
  guint source_id;    /* Timeout source id */
};

typedef struct _BtkEntryCapslockFeedback BtkEntryCapslockFeedback;

struct _BtkEntryCapslockFeedback
{
  BtkWidget *entry;
  BtkWidget *window;
  BtkWidget *label;
};

enum {
  ACTIVATE,
  POPULATE_POPUP,
  MOVE_CURSOR,
  INSERT_AT_CURSOR,
  DELETE_FROM_CURSOR,
  BACKSPACE,
  CUT_CLIPBOARD,
  COPY_CLIPBOARD,
  PASTE_CLIPBOARD,
  TOGGLE_OVERWRITE,
  ICON_PRESS,
  ICON_RELEASE,
  PREEDIT_CHANGED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_BUFFER,
  PROP_CURSOR_POSITION,
  PROP_SELECTION_BOUND,
  PROP_EDITABLE,
  PROP_MAX_LENGTH,
  PROP_VISIBILITY,
  PROP_HAS_FRAME,
  PROP_INNER_BORDER,
  PROP_INVISIBLE_CHAR,
  PROP_ACTIVATES_DEFAULT,
  PROP_WIDTH_CHARS,
  PROP_SCROLL_OFFSET,
  PROP_TEXT,
  PROP_XALIGN,
  PROP_TRUNCATE_MULTILINE,
  PROP_SHADOW_TYPE,
  PROP_OVERWRITE_MODE,
  PROP_TEXT_LENGTH,
  PROP_INVISIBLE_CHAR_SET,
  PROP_CAPS_LOCK_WARNING,
  PROP_PROGRESS_FRACTION,
  PROP_PROGRESS_PULSE_STEP,
  PROP_PIXBUF_PRIMARY,
  PROP_PIXBUF_SECONDARY,
  PROP_STOCK_PRIMARY,
  PROP_STOCK_SECONDARY,
  PROP_ICON_NAME_PRIMARY,
  PROP_ICON_NAME_SECONDARY,
  PROP_GICON_PRIMARY,
  PROP_GICON_SECONDARY,
  PROP_STORAGE_TYPE_PRIMARY,
  PROP_STORAGE_TYPE_SECONDARY,
  PROP_ACTIVATABLE_PRIMARY,
  PROP_ACTIVATABLE_SECONDARY,
  PROP_SENSITIVE_PRIMARY,
  PROP_SENSITIVE_SECONDARY,
  PROP_TOOLTIP_TEXT_PRIMARY,
  PROP_TOOLTIP_TEXT_SECONDARY,
  PROP_TOOLTIP_MARKUP_PRIMARY,
  PROP_TOOLTIP_MARKUP_SECONDARY,
  PROP_IM_MODULE,
  PROP_EDITING_CANCELED
};

static guint signals[LAST_SIGNAL] = { 0 };

typedef enum {
  CURSOR_STANDARD,
  CURSOR_DND
} CursorType;

typedef enum
{
  DISPLAY_NORMAL,       /* The entry text is being shown */
  DISPLAY_INVISIBLE,    /* In invisible mode, text replaced by (eg) bullets */
  DISPLAY_BLANK         /* In invisible mode, nothing shown at all */
} DisplayMode;

/* GObject, BtkObject methods
 */
static void   btk_entry_editable_init        (BtkEditableClass     *iface);
static void   btk_entry_cell_editable_init   (BtkCellEditableIface *iface);
static void   btk_entry_set_property         (GObject          *object,
                                              guint             prop_id,
                                              const GValue     *value,
                                              GParamSpec       *pspec);
static void   btk_entry_get_property         (GObject          *object,
                                              guint             prop_id,
                                              GValue           *value,
                                              GParamSpec       *pspec);
static void   btk_entry_finalize             (GObject          *object);
static void   btk_entry_destroy              (BtkObject        *object);
static void   btk_entry_dispose              (GObject          *object);

/* BtkWidget methods
 */
static void   btk_entry_realize              (BtkWidget        *widget);
static void   btk_entry_unrealize            (BtkWidget        *widget);
static void   btk_entry_map                  (BtkWidget        *widget);
static void   btk_entry_unmap                (BtkWidget        *widget);
static void   btk_entry_size_request         (BtkWidget        *widget,
					      BtkRequisition   *requisition);
static void   btk_entry_size_allocate        (BtkWidget        *widget,
					      BtkAllocation    *allocation);
static void   btk_entry_draw_frame           (BtkWidget        *widget,
                                              BdkEventExpose   *event);
static void   btk_entry_draw_progress        (BtkWidget        *widget,
                                              BdkEventExpose   *event);
static gint   btk_entry_expose               (BtkWidget        *widget,
					      BdkEventExpose   *event);
static gint   btk_entry_button_press         (BtkWidget        *widget,
					      BdkEventButton   *event);
static gint   btk_entry_button_release       (BtkWidget        *widget,
					      BdkEventButton   *event);
static gint   btk_entry_enter_notify         (BtkWidget        *widget,
                                              BdkEventCrossing *event);
static gint   btk_entry_leave_notify         (BtkWidget        *widget,
                                              BdkEventCrossing *event);
static gint   btk_entry_motion_notify        (BtkWidget        *widget,
					      BdkEventMotion   *event);
static gint   btk_entry_key_press            (BtkWidget        *widget,
					      BdkEventKey      *event);
static gint   btk_entry_key_release          (BtkWidget        *widget,
					      BdkEventKey      *event);
static gint   btk_entry_focus_in             (BtkWidget        *widget,
					      BdkEventFocus    *event);
static gint   btk_entry_focus_out            (BtkWidget        *widget,
					      BdkEventFocus    *event);
static void   btk_entry_grab_focus           (BtkWidget        *widget);
static void   btk_entry_style_set            (BtkWidget        *widget,
					      BtkStyle         *previous_style);
static gboolean btk_entry_query_tooltip      (BtkWidget        *widget,
                                              gint              x,
                                              gint              y,
                                              gboolean          keyboard_tip,
                                              BtkTooltip       *tooltip);
static void   btk_entry_direction_changed    (BtkWidget        *widget,
					      BtkTextDirection  previous_dir);
static void   btk_entry_state_changed        (BtkWidget        *widget,
					      BtkStateType      previous_state);
static void   btk_entry_screen_changed       (BtkWidget        *widget,
					      BdkScreen        *old_screen);

static gboolean btk_entry_drag_drop          (BtkWidget        *widget,
                                              BdkDragContext   *context,
                                              gint              x,
                                              gint              y,
                                              guint             time);
static gboolean btk_entry_drag_motion        (BtkWidget        *widget,
					      BdkDragContext   *context,
					      gint              x,
					      gint              y,
					      guint             time);
static void     btk_entry_drag_leave         (BtkWidget        *widget,
					      BdkDragContext   *context,
					      guint             time);
static void     btk_entry_drag_data_received (BtkWidget        *widget,
					      BdkDragContext   *context,
					      gint              x,
					      gint              y,
					      BtkSelectionData *selection_data,
					      guint             info,
					      guint             time);
static void     btk_entry_drag_data_get      (BtkWidget        *widget,
					      BdkDragContext   *context,
					      BtkSelectionData *selection_data,
					      guint             info,
					      guint             time);
static void     btk_entry_drag_data_delete   (BtkWidget        *widget,
					      BdkDragContext   *context);
static void     btk_entry_drag_begin         (BtkWidget        *widget,
                                              BdkDragContext   *context);
static void     btk_entry_drag_end           (BtkWidget        *widget,
                                              BdkDragContext   *context);


/* BtkEditable method implementations
 */
static void     btk_entry_insert_text          (BtkEditable *editable,
						const gchar *new_text,
						gint         new_text_length,
						gint        *position);
static void     btk_entry_delete_text          (BtkEditable *editable,
						gint         start_pos,
						gint         end_pos);
static gchar *  btk_entry_get_chars            (BtkEditable *editable,
						gint         start_pos,
						gint         end_pos);
static void     btk_entry_real_set_position    (BtkEditable *editable,
						gint         position);
static gint     btk_entry_get_position         (BtkEditable *editable);
static void     btk_entry_set_selection_bounds (BtkEditable *editable,
						gint         start,
						gint         end);
static gboolean btk_entry_get_selection_bounds (BtkEditable *editable,
						gint        *start,
						gint        *end);

/* BtkCellEditable method implementations
 */
static void btk_entry_start_editing (BtkCellEditable *cell_editable,
				     BdkEvent        *event);

/* Default signal handlers
 */
static void btk_entry_real_insert_text   (BtkEditable     *editable,
					  const gchar     *new_text,
					  gint             new_text_length,
					  gint            *position);
static void btk_entry_real_delete_text   (BtkEditable     *editable,
					  gint             start_pos,
					  gint             end_pos);
static void btk_entry_move_cursor        (BtkEntry        *entry,
					  BtkMovementStep  step,
					  gint             count,
					  gboolean         extend_selection);
static void btk_entry_insert_at_cursor   (BtkEntry        *entry,
					  const gchar     *str);
static void btk_entry_delete_from_cursor (BtkEntry        *entry,
					  BtkDeleteType    type,
					  gint             count);
static void btk_entry_backspace          (BtkEntry        *entry);
static void btk_entry_cut_clipboard      (BtkEntry        *entry);
static void btk_entry_copy_clipboard     (BtkEntry        *entry);
static void btk_entry_paste_clipboard    (BtkEntry        *entry);
static void btk_entry_toggle_overwrite   (BtkEntry        *entry);
static void btk_entry_select_all         (BtkEntry        *entry);
static void btk_entry_real_activate      (BtkEntry        *entry);
static gboolean btk_entry_popup_menu     (BtkWidget       *widget);

static void keymap_direction_changed     (BdkKeymap       *keymap,
					  BtkEntry        *entry);
static void keymap_state_changed         (BdkKeymap       *keymap,
					  BtkEntry        *entry);
static void remove_capslock_feedback     (BtkEntry        *entry);

/* IM Context Callbacks
 */
static void     btk_entry_commit_cb               (BtkIMContext *context,
						   const gchar  *str,
						   BtkEntry     *entry);
static void     btk_entry_preedit_changed_cb      (BtkIMContext *context,
						   BtkEntry     *entry);
static gboolean btk_entry_retrieve_surrounding_cb (BtkIMContext *context,
						   BtkEntry     *entry);
static gboolean btk_entry_delete_surrounding_cb   (BtkIMContext *context,
						   gint          offset,
						   gint          n_chars,
						   BtkEntry     *entry);

/* Internal routines
 */
static void         btk_entry_enter_text               (BtkEntry       *entry,
                                                        const gchar    *str);
static void         btk_entry_set_positions            (BtkEntry       *entry,
							gint            current_pos,
							gint            selection_bound);
static void         btk_entry_draw_text                (BtkEntry       *entry);
static void         btk_entry_draw_cursor              (BtkEntry       *entry,
							CursorType      type);
static BangoLayout *btk_entry_ensure_layout            (BtkEntry       *entry,
                                                        gboolean        include_preedit);
static void         btk_entry_reset_layout             (BtkEntry       *entry);
static void         btk_entry_queue_draw               (BtkEntry       *entry);
static void         btk_entry_recompute                (BtkEntry       *entry);
static gint         btk_entry_find_position            (BtkEntry       *entry,
							gint            x);
static void         btk_entry_get_cursor_locations     (BtkEntry       *entry,
							CursorType      type,
							gint           *strong_x,
							gint           *weak_x);
static void         btk_entry_adjust_scroll            (BtkEntry       *entry);
static gint         btk_entry_move_visually            (BtkEntry       *editable,
							gint            start,
							gint            count);
static gint         btk_entry_move_logically           (BtkEntry       *entry,
							gint            start,
							gint            count);
static gint         btk_entry_move_forward_word        (BtkEntry       *entry,
							gint            start,
                                                        gboolean        allow_whitespace);
static gint         btk_entry_move_backward_word       (BtkEntry       *entry,
							gint            start,
                                                        gboolean        allow_whitespace);
static void         btk_entry_delete_whitespace        (BtkEntry       *entry);
static void         btk_entry_select_word              (BtkEntry       *entry);
static void         btk_entry_select_line              (BtkEntry       *entry);
static void         btk_entry_paste                    (BtkEntry       *entry,
							BdkAtom         selection);
static void         btk_entry_update_primary_selection (BtkEntry       *entry);
static void         btk_entry_do_popup                 (BtkEntry       *entry,
							BdkEventButton *event);
static gboolean     btk_entry_mnemonic_activate        (BtkWidget      *widget,
							gboolean        group_cycling);
static void         btk_entry_check_cursor_blink       (BtkEntry       *entry);
static void         btk_entry_pend_cursor_blink        (BtkEntry       *entry);
static void         btk_entry_reset_blink_time         (BtkEntry       *entry);
static void         btk_entry_get_text_area_size       (BtkEntry       *entry,
							gint           *x,
							gint           *y,
							gint           *width,
							gint           *height);
static void         get_text_area_size                 (BtkEntry       *entry,
							gint           *x,
							gint           *y,
							gint           *width,
							gint           *height);
static void         get_widget_window_size             (BtkEntry       *entry,
							gint           *x,
							gint           *y,
							gint           *width,
							gint           *height);
static void         btk_entry_move_adjustments         (BtkEntry             *entry);
static void         btk_entry_ensure_pixbuf            (BtkEntry             *entry,
                                                        BtkEntryIconPosition  icon_pos);


/* Completion */
static gint         btk_entry_completion_timeout       (gpointer            data);
static gboolean     btk_entry_completion_key_press     (BtkWidget          *widget,
							BdkEventKey        *event,
							gpointer            user_data);
static void         btk_entry_completion_changed       (BtkWidget          *entry,
							gpointer            user_data);
static gboolean     check_completion_callback          (BtkEntryCompletion *completion);
static void         clear_completion_callback          (BtkEntry           *entry,
							GParamSpec         *pspec);
static gboolean     accept_completion_callback         (BtkEntry           *entry);
static void         completion_insert_text_callback    (BtkEntry           *entry,
							const gchar        *text,
							gint                length,
							gint                position,
							BtkEntryCompletion *completion);
static void         disconnect_completion_signals      (BtkEntry           *entry,
							BtkEntryCompletion *completion);
static void         connect_completion_signals         (BtkEntry           *entry,
							BtkEntryCompletion *completion);

static void         begin_change                       (BtkEntry *entry);
static void         end_change                         (BtkEntry *entry);
static void         emit_changed                       (BtkEntry *entry);


static void         buffer_inserted_text               (BtkEntryBuffer *buffer, 
                                                        guint           position,
                                                        const gchar    *chars,
                                                        guint           n_chars,
                                                        BtkEntry       *entry);
static void         buffer_deleted_text                (BtkEntryBuffer *buffer, 
                                                        guint           position,
                                                        guint           n_chars,
                                                        BtkEntry       *entry);
static void         buffer_notify_text                 (BtkEntryBuffer *buffer, 
                                                        GParamSpec     *spec,
                                                        BtkEntry       *entry);
static void         buffer_notify_length               (BtkEntryBuffer *buffer, 
                                                        GParamSpec     *spec,
                                                        BtkEntry       *entry);
static void         buffer_notify_max_length           (BtkEntryBuffer *buffer, 
                                                        GParamSpec     *spec,
                                                        BtkEntry       *entry);
static void         buffer_connect_signals             (BtkEntry       *entry);
static void         buffer_disconnect_signals          (BtkEntry       *entry);
static BtkEntryBuffer *get_buffer                      (BtkEntry       *entry);


G_DEFINE_TYPE_WITH_CODE (BtkEntry, btk_entry, BTK_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BTK_TYPE_EDITABLE,
                                                btk_entry_editable_init)
                         G_IMPLEMENT_INTERFACE (BTK_TYPE_CELL_EDITABLE,
                                                btk_entry_cell_editable_init))

static void
add_move_binding (BtkBindingSet  *binding_set,
		  guint           keyval,
		  guint           modmask,
		  BtkMovementStep step,
		  gint            count)
{
  g_return_if_fail ((modmask & BDK_SHIFT_MASK) == 0);
  
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
btk_entry_class_init (BtkEntryClass *class)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (class);
  BtkWidgetClass *widget_class;
  BtkObjectClass *btk_object_class;
  BtkBindingSet *binding_set;

  widget_class = (BtkWidgetClass*) class;
  btk_object_class = (BtkObjectClass *)class;

  bobject_class->dispose = btk_entry_dispose;
  bobject_class->finalize = btk_entry_finalize;
  bobject_class->set_property = btk_entry_set_property;
  bobject_class->get_property = btk_entry_get_property;

  widget_class->map = btk_entry_map;
  widget_class->unmap = btk_entry_unmap;
  widget_class->realize = btk_entry_realize;
  widget_class->unrealize = btk_entry_unrealize;
  widget_class->size_request = btk_entry_size_request;
  widget_class->size_allocate = btk_entry_size_allocate;
  widget_class->expose_event = btk_entry_expose;
  widget_class->enter_notify_event = btk_entry_enter_notify;
  widget_class->leave_notify_event = btk_entry_leave_notify;
  widget_class->button_press_event = btk_entry_button_press;
  widget_class->button_release_event = btk_entry_button_release;
  widget_class->motion_notify_event = btk_entry_motion_notify;
  widget_class->key_press_event = btk_entry_key_press;
  widget_class->key_release_event = btk_entry_key_release;
  widget_class->focus_in_event = btk_entry_focus_in;
  widget_class->focus_out_event = btk_entry_focus_out;
  widget_class->grab_focus = btk_entry_grab_focus;
  widget_class->style_set = btk_entry_style_set;
  widget_class->query_tooltip = btk_entry_query_tooltip;
  widget_class->drag_begin = btk_entry_drag_begin;
  widget_class->drag_end = btk_entry_drag_end;
  widget_class->direction_changed = btk_entry_direction_changed;
  widget_class->state_changed = btk_entry_state_changed;
  widget_class->screen_changed = btk_entry_screen_changed;
  widget_class->mnemonic_activate = btk_entry_mnemonic_activate;

  widget_class->drag_drop = btk_entry_drag_drop;
  widget_class->drag_motion = btk_entry_drag_motion;
  widget_class->drag_leave = btk_entry_drag_leave;
  widget_class->drag_data_received = btk_entry_drag_data_received;
  widget_class->drag_data_get = btk_entry_drag_data_get;
  widget_class->drag_data_delete = btk_entry_drag_data_delete;

  widget_class->popup_menu = btk_entry_popup_menu;

  btk_object_class->destroy = btk_entry_destroy;

  class->move_cursor = btk_entry_move_cursor;
  class->insert_at_cursor = btk_entry_insert_at_cursor;
  class->delete_from_cursor = btk_entry_delete_from_cursor;
  class->backspace = btk_entry_backspace;
  class->cut_clipboard = btk_entry_cut_clipboard;
  class->copy_clipboard = btk_entry_copy_clipboard;
  class->paste_clipboard = btk_entry_paste_clipboard;
  class->toggle_overwrite = btk_entry_toggle_overwrite;
  class->activate = btk_entry_real_activate;
  class->get_text_area_size = btk_entry_get_text_area_size;
  
  quark_inner_border = g_quark_from_static_string ("btk-entry-inner-border");
  quark_password_hint = g_quark_from_static_string ("btk-entry-password-hint");
  quark_cursor_hadjustment = g_quark_from_static_string ("btk-hadjustment");
  quark_capslock_feedback = g_quark_from_static_string ("btk-entry-capslock-feedback");

  g_object_class_override_property (bobject_class,
                                    PROP_EDITING_CANCELED,
                                    "editing-canceled");

  g_object_class_install_property (bobject_class,
                                   PROP_BUFFER,
                                   g_param_spec_object ("buffer",
                                                        P_("Text Buffer"),
                                                        P_("Text buffer object which actually stores entry text"),
                                                        BTK_TYPE_ENTRY_BUFFER,
                                                        BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (bobject_class,
                                   PROP_CURSOR_POSITION,
                                   g_param_spec_int ("cursor-position",
                                                     P_("Cursor Position"),
                                                     P_("The current position of the insertion cursor in chars"),
                                                     0,
                                                     BTK_ENTRY_BUFFER_MAX_SIZE,
                                                     0,
                                                     BTK_PARAM_READABLE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_SELECTION_BOUND,
                                   g_param_spec_int ("selection-bound",
                                                     P_("Selection Bound"),
                                                     P_("The position of the opposite end of the selection from the cursor in chars"),
                                                     0,
                                                     BTK_ENTRY_BUFFER_MAX_SIZE,
                                                     0,
                                                     BTK_PARAM_READABLE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_EDITABLE,
                                   g_param_spec_boolean ("editable",
							 P_("Editable"),
							 P_("Whether the entry contents can be edited"),
                                                         TRUE,
							 BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_MAX_LENGTH,
                                   g_param_spec_int ("max-length",
                                                     P_("Maximum length"),
                                                     P_("Maximum number of characters for this entry. Zero if no maximum"),
                                                     0,
                                                     BTK_ENTRY_BUFFER_MAX_SIZE,
                                                     0,
                                                     BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_VISIBILITY,
                                   g_param_spec_boolean ("visibility",
							 P_("Visibility"),
							 P_("FALSE displays the \"invisible char\" instead of the actual text (password mode)"),
                                                         TRUE,
							 BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_HAS_FRAME,
                                   g_param_spec_boolean ("has-frame",
							 P_("Has Frame"),
							 P_("FALSE removes outside bevel from entry"),
                                                         TRUE,
							 BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_INNER_BORDER,
                                   g_param_spec_boxed ("inner-border",
                                                       P_("Inner Border"),
                                                       P_("Border between text and frame. Overrides the inner-border style property"),
                                                       BTK_TYPE_BORDER,
                                                       BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_INVISIBLE_CHAR,
                                   g_param_spec_unichar ("invisible-char",
							 P_("Invisible character"),
							 P_("The character to use when masking entry contents (in \"password mode\")"),
							 '*',
							 BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_ACTIVATES_DEFAULT,
                                   g_param_spec_boolean ("activates-default",
							 P_("Activates default"),
							 P_("Whether to activate the default widget (such as the default button in a dialog) when Enter is pressed"),
                                                         FALSE,
							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_WIDTH_CHARS,
                                   g_param_spec_int ("width-chars",
                                                     P_("Width in chars"),
                                                     P_("Number of characters to leave space for in the entry"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_SCROLL_OFFSET,
                                   g_param_spec_int ("scroll-offset",
                                                     P_("Scroll offset"),
                                                     P_("Number of pixels of the entry scrolled off the screen to the left"),
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     BTK_PARAM_READABLE));

  g_object_class_install_property (bobject_class,
                                   PROP_TEXT,
                                   g_param_spec_string ("text",
							P_("Text"),
							P_("The contents of the entry"),
							"",
							BTK_PARAM_READWRITE));

  /**
   * BtkEntry:xalign:
   *
   * The horizontal alignment, from 0 (left) to 1 (right). 
   * Reversed for RTL layouts.
   * 
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
                                   PROP_XALIGN,
                                   g_param_spec_float ("xalign",
						       P_("X align"),
						       P_("The horizontal alignment, from 0 (left) to 1 (right). Reversed for RTL layouts."),
						       0.0,
						       1.0,
						       0.0,
						       BTK_PARAM_READWRITE));

  /**
   * BtkEntry:truncate-multiline:
   *
   * When %TRUE, pasted multi-line text is truncated to the first line.
   *
   * Since: 2.10
   */
  g_object_class_install_property (bobject_class,
                                   PROP_TRUNCATE_MULTILINE,
                                   g_param_spec_boolean ("truncate-multiline",
                                                         P_("Truncate multiline"),
                                                         P_("Whether to truncate multiline pastes to one line."),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));

  /**
   * BtkEntry:shadow-type:
   *
   * Which kind of shadow to draw around the entry when 
   * #BtkEntry:has-frame is set to %TRUE.
   *
   * Since: 2.12
   */
  g_object_class_install_property (bobject_class,
                                   PROP_SHADOW_TYPE,
                                   g_param_spec_enum ("shadow-type",
                                                      P_("Shadow type"),
                                                      P_("Which kind of shadow to draw around the entry when has-frame is set"),
                                                      BTK_TYPE_SHADOW_TYPE,
                                                      BTK_SHADOW_IN,
                                                      BTK_PARAM_READWRITE));

  /**
   * BtkEntry:overwrite-mode:
   *
   * If text is overwritten when typing in the #BtkEntry.
   *
   * Since: 2.14
   */
  g_object_class_install_property (bobject_class,
                                   PROP_OVERWRITE_MODE,
                                   g_param_spec_boolean ("overwrite-mode",
                                                         P_("Overwrite mode"),
                                                         P_("Whether new text overwrites existing text"),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));

  /**
   * BtkEntry:text-length:
   *
   * The length of the text in the #BtkEntry.
   *
   * Since: 2.14
   */
  g_object_class_install_property (bobject_class,
                                   PROP_TEXT_LENGTH,
                                   g_param_spec_uint ("text-length",
                                                      P_("Text length"),
                                                      P_("Length of the text currently in the entry"),
                                                      0, 
                                                      G_MAXUINT16,
                                                      0,
                                                      BTK_PARAM_READABLE));
  /**
   * BtkEntry:invisible-char-set:
   *
   * Whether the invisible char has been set for the #BtkEntry.
   *
   * Since: 2.16
   */
  g_object_class_install_property (bobject_class,
                                   PROP_INVISIBLE_CHAR_SET,
                                   g_param_spec_boolean ("invisible-char-set",
                                                         P_("Invisible char set"),
                                                         P_("Whether the invisible char has been set"),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));

  /**
   * BtkEntry:caps-lock-warning
   *
   * Whether password entries will show a warning when Caps Lock is on.
   *
   * Note that the warning is shown using a secondary icon, and thus
   * does not work if you are using the secondary icon position for some 
   * other purpose.
   *
   * Since: 2.16
   */
  g_object_class_install_property (bobject_class,
                                   PROP_CAPS_LOCK_WARNING,
                                   g_param_spec_boolean ("caps-lock-warning",
                                                         P_("Caps Lock warning"),
                                                         P_("Whether password entries will show a warning when Caps Lock is on"),
                                                         TRUE,
                                                         BTK_PARAM_READWRITE));

  /**
   * BtkEntry:progress-fraction:
   *
   * The current fraction of the task that's been completed.
   *
   * Since: 2.16
   */
  g_object_class_install_property (bobject_class,
                                   PROP_PROGRESS_FRACTION,
                                   g_param_spec_double ("progress-fraction",
                                                        P_("Progress Fraction"),
                                                        P_("The current fraction of the task that's been completed"),
                                                        0.0,
                                                        1.0,
                                                        0.0,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkEntry:progress-pulse-step:
   *
   * The fraction of total entry width to move the progress
   * bouncing block for each call to btk_entry_progress_pulse().
   *
   * Since: 2.16
   */
  g_object_class_install_property (bobject_class,
                                   PROP_PROGRESS_PULSE_STEP,
                                   g_param_spec_double ("progress-pulse-step",
                                                        P_("Progress Pulse Step"),
                                                        P_("The fraction of total entry width to move the progress bouncing block for each call to btk_entry_progress_pulse()"),
                                                        0.0,
                                                        1.0,
                                                        0.1,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkEntry:primary-icon-pixbuf:
   *
   * A pixbuf to use as the primary icon for the entry.
   *
   * Since: 2.16
   */
  g_object_class_install_property (bobject_class,
                                   PROP_PIXBUF_PRIMARY,
                                   g_param_spec_object ("primary-icon-pixbuf",
                                                        P_("Primary pixbuf"),
                                                        P_("Primary pixbuf for the entry"),
                                                        BDK_TYPE_PIXBUF,
                                                        BTK_PARAM_READWRITE));
  
  /**
   * BtkEntry:secondary-icon-pixbuf:
   *
   * An pixbuf to use as the secondary icon for the entry.
   *
   * Since: 2.16
   */
  g_object_class_install_property (bobject_class,
                                   PROP_PIXBUF_SECONDARY,
                                   g_param_spec_object ("secondary-icon-pixbuf",
                                                        P_("Secondary pixbuf"),
                                                        P_("Secondary pixbuf for the entry"),
                                                        BDK_TYPE_PIXBUF,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkEntry:primary-icon-stock:
   *
   * The stock id to use for the primary icon for the entry.
   *
   * Since: 2.16
   */
  g_object_class_install_property (bobject_class,
                                   PROP_STOCK_PRIMARY,
                                   g_param_spec_string ("primary-icon-stock",
                                                        P_("Primary stock ID"),
                                                        P_("Stock ID for primary icon"),
                                                        NULL,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkEntry:secondary-icon-stock:
   *
   * The stock id to use for the secondary icon for the entry.
   *
   * Since: 2.16
   */
  g_object_class_install_property (bobject_class,
                                   PROP_STOCK_SECONDARY,
                                   g_param_spec_string ("secondary-icon-stock",
                                                        P_("Secondary stock ID"),
                                                        P_("Stock ID for secondary icon"),
                                                        NULL,
                                                        BTK_PARAM_READWRITE));
  
  /**
   * BtkEntry:primary-icon-name:
   *
   * The icon name to use for the primary icon for the entry.
   *
   * Since: 2.16
   */
  g_object_class_install_property (bobject_class,
                                   PROP_ICON_NAME_PRIMARY,
                                   g_param_spec_string ("primary-icon-name",
                                                        P_("Primary icon name"),
                                                        P_("Icon name for primary icon"),
                                                        NULL,
                                                        BTK_PARAM_READWRITE));
  
  /**
   * BtkEntry:secondary-icon-name:
   *
   * The icon name to use for the secondary icon for the entry.
   *
   * Since: 2.16
   */
  g_object_class_install_property (bobject_class,
                                   PROP_ICON_NAME_SECONDARY,
                                   g_param_spec_string ("secondary-icon-name",
                                                        P_("Secondary icon name"),
                                                        P_("Icon name for secondary icon"),
                                                        NULL,
                                                        BTK_PARAM_READWRITE));
  
  /**
   * BtkEntry:primary-icon-gicon:
   *
   * The #GIcon to use for the primary icon for the entry.
   *
   * Since: 2.16
   */
  g_object_class_install_property (bobject_class,
                                   PROP_GICON_PRIMARY,
                                   g_param_spec_object ("primary-icon-gicon",
                                                        P_("Primary GIcon"),
                                                        P_("GIcon for primary icon"),
                                                        G_TYPE_ICON,
                                                        BTK_PARAM_READWRITE));
  
  /**
   * BtkEntry:secondary-icon-gicon:
   *
   * The #GIcon to use for the secondary icon for the entry.
   *
   * Since: 2.16
   */
  g_object_class_install_property (bobject_class,
                                   PROP_GICON_SECONDARY,
                                   g_param_spec_object ("secondary-icon-gicon",
                                                        P_("Secondary GIcon"),
                                                        P_("GIcon for secondary icon"),
                                                        G_TYPE_ICON,
                                                        BTK_PARAM_READWRITE));
  
  /**
   * BtkEntry:primary-icon-storage-type:
   *
   * The representation which is used for the primary icon of the entry.
   *
   * Since: 2.16
   */
  g_object_class_install_property (bobject_class,
                                   PROP_STORAGE_TYPE_PRIMARY,
                                   g_param_spec_enum ("primary-icon-storage-type",
                                                      P_("Primary storage type"),
                                                      P_("The representation being used for primary icon"),
                                                      BTK_TYPE_IMAGE_TYPE,
                                                      BTK_IMAGE_EMPTY,
                                                      BTK_PARAM_READABLE));
  
  /**
   * BtkEntry:secondary-icon-storage-type:
   *
   * The representation which is used for the secondary icon of the entry.
   *
   * Since: 2.16
   */
  g_object_class_install_property (bobject_class,
                                   PROP_STORAGE_TYPE_SECONDARY,
                                   g_param_spec_enum ("secondary-icon-storage-type",
                                                      P_("Secondary storage type"),
                                                      P_("The representation being used for secondary icon"),
                                                      BTK_TYPE_IMAGE_TYPE,
                                                      BTK_IMAGE_EMPTY,
                                                      BTK_PARAM_READABLE));
  
  /**
   * BtkEntry:primary-icon-activatable:
   *
   * Whether the primary icon is activatable.
   *
   * BTK+ emits the #BtkEntry::icon-press and #BtkEntry::icon-release 
   * signals only on sensitive, activatable icons. 
   *
   * Sensitive, but non-activatable icons can be used for purely 
   * informational purposes.
   *
   * Since: 2.16
   */
  g_object_class_install_property (bobject_class,
                                   PROP_ACTIVATABLE_PRIMARY,
                                   g_param_spec_boolean ("primary-icon-activatable",
                                                         P_("Primary icon activatable"),
                                                         P_("Whether the primary icon is activatable"),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));
  
  /**
   * BtkEntry:secondary-icon-activatable:
   *
   * Whether the secondary icon is activatable.
   *
   * BTK+ emits the #BtkEntry::icon-press and #BtkEntry::icon-release 
   * signals only on sensitive, activatable icons.
   *
   * Sensitive, but non-activatable icons can be used for purely 
   * informational purposes.
   *
   * Since: 2.16
   */
  g_object_class_install_property (bobject_class,
                                   PROP_ACTIVATABLE_SECONDARY,
                                   g_param_spec_boolean ("secondary-icon-activatable",
                                                         P_("Secondary icon activatable"),
                                                         P_("Whether the secondary icon is activatable"),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));
  
  
  /**
   * BtkEntry:primary-icon-sensitive:
   *
   * Whether the primary icon is sensitive.
   *
   * An insensitive icon appears grayed out. BTK+ does not emit the 
   * #BtkEntry::icon-press and #BtkEntry::icon-release signals and 
   * does not allow DND from insensitive icons.
   *
   * An icon should be set insensitive if the action that would trigger
   * when clicked is currently not available.
   * 
   * Since: 2.16
   */
  g_object_class_install_property (bobject_class,
                                   PROP_SENSITIVE_PRIMARY,
                                   g_param_spec_boolean ("primary-icon-sensitive",
                                                         P_("Primary icon sensitive"),
                                                         P_("Whether the primary icon is sensitive"),
                                                         TRUE,
                                                         BTK_PARAM_READWRITE));
  
  /**
   * BtkEntry:secondary-icon-sensitive:
   *
   * Whether the secondary icon is sensitive.
   *
   * An insensitive icon appears grayed out. BTK+ does not emit the 
   * #BtkEntry::icon-press and #BtkEntry::icon-release signals and 
   * does not allow DND from insensitive icons.
   *
   * An icon should be set insensitive if the action that would trigger
   * when clicked is currently not available.
   *
   * Since: 2.16
   */
  g_object_class_install_property (bobject_class,
                                   PROP_SENSITIVE_SECONDARY,
                                   g_param_spec_boolean ("secondary-icon-sensitive",
                                                         P_("Secondary icon sensitive"),
                                                         P_("Whether the secondary icon is sensitive"),
                                                         TRUE,
                                                         BTK_PARAM_READWRITE));
  
  /**
   * BtkEntry:primary-icon-tooltip-text:
   * 
   * The contents of the tooltip on the primary icon.
   *
   * Also see btk_entry_set_icon_tooltip_text().
   *
   * Since: 2.16
   */
  g_object_class_install_property (bobject_class,
                                   PROP_TOOLTIP_TEXT_PRIMARY,
                                   g_param_spec_string ("primary-icon-tooltip-text",
                                                        P_("Primary icon tooltip text"),
                                                        P_("The contents of the tooltip on the primary icon"),                              
                                                        NULL,
                                                        BTK_PARAM_READWRITE));
  
  /**
   * BtkEntry:secondary-icon-tooltip-text:
   * 
   * The contents of the tooltip on the secondary icon.
   *
   * Also see btk_entry_set_icon_tooltip_text().
   *
   * Since: 2.16
   */
  g_object_class_install_property (bobject_class,
                                   PROP_TOOLTIP_TEXT_SECONDARY,
                                   g_param_spec_string ("secondary-icon-tooltip-text",
                                                        P_("Secondary icon tooltip text"),
                                                        P_("The contents of the tooltip on the secondary icon"),                              
                                                        NULL,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkEntry:primary-icon-tooltip-markup:
   * 
   * The contents of the tooltip on the primary icon, which is marked up
   * with the <link linkend="BangoMarkupFormat">Bango text markup 
   * language</link>.
   *
   * Also see btk_entry_set_icon_tooltip_markup().
   *
   * Since: 2.16
   */
  g_object_class_install_property (bobject_class,
                                   PROP_TOOLTIP_MARKUP_PRIMARY,
                                   g_param_spec_string ("primary-icon-tooltip-markup",
                                                        P_("Primary icon tooltip markup"),
                                                        P_("The contents of the tooltip on the primary icon"),                              
                                                        NULL,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkEntry:secondary-icon-tooltip-markup:
   * 
   * The contents of the tooltip on the secondary icon, which is marked up
   * with the <link linkend="BangoMarkupFormat">Bango text markup 
   * language</link>.
   *
   * Also see btk_entry_set_icon_tooltip_markup().
   *
   * Since: 2.16
   */
  g_object_class_install_property (bobject_class,
                                   PROP_TOOLTIP_MARKUP_SECONDARY,
                                   g_param_spec_string ("secondary-icon-tooltip-markup",
                                                        P_("Secondary icon tooltip markup"),
                                                        P_("The contents of the tooltip on the secondary icon"),                              
                                                        NULL,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkEntry:im-module:
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

  /**
   * BtkEntry:icon-prelight:
   *
   * The prelight style property determines whether activatable
   * icons prelight on mouseover.
   *
   * Since: 2.16
   */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boolean ("icon-prelight",
                                                                 P_("Icon Prelight"),
                                                                 P_("Whether activatable icons should prelight when hovered"),
                                                                 TRUE,
                                                                 BTK_PARAM_READABLE));

  /**
   * BtkEntry:progress-border:
   *
   * The border around the progress bar in the entry.
   *
   * Since: 2.16
   */
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_boxed ("progress-border",
                                                               P_("Progress Border"),
                                                               P_("Border around the progress bar"),
                                                               BTK_TYPE_BORDER,
                                                               BTK_PARAM_READABLE));
  
  /**
   * BtkEntry:invisible-char:
   *
   * The invisible character is used when masking entry contents (in
   * \"password mode\")"). When it is not explicitly set with the
   * #BtkEntry::invisible-char property, BTK+ determines the character
   * to use from a list of possible candidates, depending on availability
   * in the current font.
   *
   * This style property allows the theme to prepend a character
   * to the list of candidates.
   *
   * Since: 2.18
   */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_unichar ("invisible-char",
					    		         P_("Invisible character"),
							         P_("The character to use when masking entry contents (in \"password mode\")"),
							         0,
							         BTK_PARAM_READABLE));
  
  /**
   * BtkEntry::populate-popup:
   * @entry: The entry on which the signal is emitted
   * @menu: the menu that is being populated
   *
   * The ::populate-popup signal gets emitted before showing the 
   * context menu of the entry. 
   *
   * If you need to add items to the context menu, connect
   * to this signal and append your menuitems to the @menu.
   */
  signals[POPULATE_POPUP] =
    g_signal_new (I_("populate-popup"),
		  G_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkEntryClass, populate_popup),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  BTK_TYPE_MENU);
  
 /* Action signals */
  
  /**
   * BtkEntry::activate:
   * @entry: The entry on which the signal is emitted
   *
   * The ::activate signal is emitted when the the user hits
   * the Enter key.
   *
   * While this signal is used as a <link linkend="keybinding-signals">keybinding signal</link>,
   * it is also commonly used by applications to intercept
   * activation of entries.
   *
   * The default bindings for this signal are all forms of the Enter key.
   */
  signals[ACTIVATE] =
    g_signal_new (I_("activate"),
		  G_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkEntryClass, activate),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  widget_class->activate_signal = signals[ACTIVATE];

  /**
   * BtkEntry::move-cursor:
   * @entry: the object which received the signal
   * @step: the granularity of the move, as a #BtkMovementStep
   * @count: the number of @step units to move
   * @extend_selection: %TRUE if the move should extend the selection
   *
   * The ::move-cursor signal is a
   * <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted when the user initiates a cursor movement.
   * If the cursor is not visible in @entry, this signal causes
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
   * </itemizedlist>
   */
  signals[MOVE_CURSOR] = 
    g_signal_new (I_("move-cursor"),
		  G_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkEntryClass, move_cursor),
		  NULL, NULL,
		  _btk_marshal_VOID__ENUM_INT_BOOLEAN,
		  G_TYPE_NONE, 3,
		  BTK_TYPE_MOVEMENT_STEP,
		  G_TYPE_INT,
		  G_TYPE_BOOLEAN);

  /**
   * BtkEntry::insert-at-cursor:
   * @entry: the object which received the signal
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
		  G_STRUCT_OFFSET (BtkEntryClass, insert_at_cursor),
		  NULL, NULL,
		  _btk_marshal_VOID__STRING,
		  G_TYPE_NONE, 1,
		  G_TYPE_STRING);

  /**
   * BtkEntry::delete-from-cursor:
   * @entry: the object which received the signal
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
   * Delete for deleting a character and Ctrl-Delete for
   * deleting a word.
   */
  signals[DELETE_FROM_CURSOR] = 
    g_signal_new (I_("delete-from-cursor"),
		  G_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkEntryClass, delete_from_cursor),
		  NULL, NULL,
		  _btk_marshal_VOID__ENUM_INT,
		  G_TYPE_NONE, 2,
		  BTK_TYPE_DELETE_TYPE,
		  G_TYPE_INT);

  /**
   * BtkEntry::backspace:
   * @entry: the object which received the signal
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
		  G_STRUCT_OFFSET (BtkEntryClass, backspace),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * BtkEntry::cut-clipboard:
   * @entry: the object which received the signal
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
		  G_STRUCT_OFFSET (BtkEntryClass, cut_clipboard),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * BtkEntry::copy-clipboard:
   * @entry: the object which received the signal
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
		  G_STRUCT_OFFSET (BtkEntryClass, copy_clipboard),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * BtkEntry::paste-clipboard:
   * @entry: the object which received the signal
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
		  G_STRUCT_OFFSET (BtkEntryClass, paste_clipboard),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * BtkEntry::toggle-overwrite:
   * @entry: the object which received the signal
   *
   * The ::toggle-overwrite signal is a
   * <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted to toggle the overwrite mode of the entry.
   *
   * The default bindings for this signal is Insert.
   */
  signals[TOGGLE_OVERWRITE] =
    g_signal_new (I_("toggle-overwrite"),
		  G_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkEntryClass, toggle_overwrite),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * BtkEntry::icon-press:
   * @entry: The entry on which the signal is emitted
   * @icon_pos: The position of the clicked icon
   * @event: the button press event
   *
   * The ::icon-press signal is emitted when an activatable icon
   * is clicked.
   *
   * Since: 2.16
   */
  signals[ICON_PRESS] =
    g_signal_new (I_("icon-press"),
                  G_TYPE_FROM_CLASS (bobject_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  _btk_marshal_VOID__ENUM_BOXED,
                  G_TYPE_NONE, 2,
                  BTK_TYPE_ENTRY_ICON_POSITION,
                  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  
  /**
   * BtkEntry::icon-release:
   * @entry: The entry on which the signal is emitted
   * @icon_pos: The position of the clicked icon
   * @event: the button release event
   *
   * The ::icon-release signal is emitted on the button release from a
   * mouse click over an activatable icon.
   *
   * Since: 2.16
   */
  signals[ICON_RELEASE] =
    g_signal_new (I_("icon-release"),
                  G_TYPE_FROM_CLASS (bobject_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  _btk_marshal_VOID__ENUM_BOXED,
                  G_TYPE_NONE, 2,
                  BTK_TYPE_ENTRY_ICON_POSITION,
                  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkEntry::preedit-changed:
   * @entry: the object which received the signal
   * @preedit: the current preedit string
   *
   * If an input method is used, the typed text will not immediately
   * be committed to the buffer. So if you are interested in the text,
   * connect to this signal.
   *
   * Since: 2.20
   */
  signals[PREEDIT_CHANGED] =
    g_signal_new_class_handler (I_("preedit-changed"),
                                G_OBJECT_CLASS_TYPE (bobject_class),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                NULL,
                                NULL, NULL,
                                _btk_marshal_VOID__STRING,
                                G_TYPE_NONE, 1,
                                G_TYPE_STRING);


  /*
   * Key bindings
   */

  binding_set = btk_binding_set_by_class (class);

  /* Moving the insertion point */
  add_move_binding (binding_set, BDK_Right, 0,
		    BTK_MOVEMENT_VISUAL_POSITIONS, 1);
  
  add_move_binding (binding_set, BDK_Left, 0,
		    BTK_MOVEMENT_VISUAL_POSITIONS, -1);

  add_move_binding (binding_set, BDK_KP_Right, 0,
		    BTK_MOVEMENT_VISUAL_POSITIONS, 1);
  
  add_move_binding (binding_set, BDK_KP_Left, 0,
		    BTK_MOVEMENT_VISUAL_POSITIONS, -1);
  
  add_move_binding (binding_set, BDK_Right, BDK_CONTROL_MASK,
		    BTK_MOVEMENT_WORDS, 1);

  add_move_binding (binding_set, BDK_Left, BDK_CONTROL_MASK,
		    BTK_MOVEMENT_WORDS, -1);

  add_move_binding (binding_set, BDK_KP_Right, BDK_CONTROL_MASK,
		    BTK_MOVEMENT_WORDS, 1);

  add_move_binding (binding_set, BDK_KP_Left, BDK_CONTROL_MASK,
		    BTK_MOVEMENT_WORDS, -1);
  
  add_move_binding (binding_set, BDK_Home, 0,
		    BTK_MOVEMENT_DISPLAY_LINE_ENDS, -1);

  add_move_binding (binding_set, BDK_End, 0,
		    BTK_MOVEMENT_DISPLAY_LINE_ENDS, 1);

  add_move_binding (binding_set, BDK_KP_Home, 0,
		    BTK_MOVEMENT_DISPLAY_LINE_ENDS, -1);

  add_move_binding (binding_set, BDK_KP_End, 0,
		    BTK_MOVEMENT_DISPLAY_LINE_ENDS, 1);
  
  add_move_binding (binding_set, BDK_Home, BDK_CONTROL_MASK,
		    BTK_MOVEMENT_BUFFER_ENDS, -1);

  add_move_binding (binding_set, BDK_End, BDK_CONTROL_MASK,
		    BTK_MOVEMENT_BUFFER_ENDS, 1);

  add_move_binding (binding_set, BDK_KP_Home, BDK_CONTROL_MASK,
		    BTK_MOVEMENT_BUFFER_ENDS, -1);

  add_move_binding (binding_set, BDK_KP_End, BDK_CONTROL_MASK,
		    BTK_MOVEMENT_BUFFER_ENDS, 1);

  /* Select all
   */
  btk_binding_entry_add_signal (binding_set, BDK_a, BDK_CONTROL_MASK,
                                "move-cursor", 3,
                                BTK_TYPE_MOVEMENT_STEP, BTK_MOVEMENT_BUFFER_ENDS,
                                G_TYPE_INT, -1,
				G_TYPE_BOOLEAN, FALSE);
  btk_binding_entry_add_signal (binding_set, BDK_a, BDK_CONTROL_MASK,
                                "move-cursor", 3,
                                BTK_TYPE_MOVEMENT_STEP, BTK_MOVEMENT_BUFFER_ENDS,
                                G_TYPE_INT, 1,
				G_TYPE_BOOLEAN, TRUE);  

  btk_binding_entry_add_signal (binding_set, BDK_slash, BDK_CONTROL_MASK,
                                "move-cursor", 3,
                                BTK_TYPE_MOVEMENT_STEP, BTK_MOVEMENT_BUFFER_ENDS,
                                G_TYPE_INT, -1,
				G_TYPE_BOOLEAN, FALSE);
  btk_binding_entry_add_signal (binding_set, BDK_slash, BDK_CONTROL_MASK,
                                "move-cursor", 3,
                                BTK_TYPE_MOVEMENT_STEP, BTK_MOVEMENT_BUFFER_ENDS,
                                G_TYPE_INT, 1,
				G_TYPE_BOOLEAN, TRUE);  
  /* Unselect all 
   */
  btk_binding_entry_add_signal (binding_set, BDK_backslash, BDK_CONTROL_MASK,
                                "move-cursor", 3,
                                BTK_TYPE_MOVEMENT_STEP, BTK_MOVEMENT_VISUAL_POSITIONS,
                                G_TYPE_INT, 0,
				G_TYPE_BOOLEAN, FALSE);
  btk_binding_entry_add_signal (binding_set, BDK_a, BDK_SHIFT_MASK | BDK_CONTROL_MASK,
                                "move-cursor", 3,
                                BTK_TYPE_MOVEMENT_STEP, BTK_MOVEMENT_VISUAL_POSITIONS,
                                G_TYPE_INT, 0,
				G_TYPE_BOOLEAN, FALSE);

  /* Activate
   */
  btk_binding_entry_add_signal (binding_set, BDK_Return, 0,
				"activate", 0);
  btk_binding_entry_add_signal (binding_set, BDK_ISO_Enter, 0,
				"activate", 0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Enter, 0,
				"activate", 0);
  
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

  /**
   * BtkEntry:inner-border:
   *
   * Sets the text area's border between the text and the frame.
   *
   * Since: 2.10
   */
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_boxed ("inner-border",
                                                               P_("Inner Border"),
                                                               P_("Border between text and frame."),
                                                               BTK_TYPE_BORDER,
                                                               BTK_PARAM_READABLE));

   /**
    * BtkEntry:state-hint:
    *
    * Indicates whether to pass a proper widget state when
    * drawing the shadow and the widget background.
    *
    * Since: 2.16
    *
    * Deprecated: 2.22: This style property will be removed in BTK+ 3
    */
   btk_widget_class_install_style_property (widget_class,
                                            g_param_spec_boolean ("state-hint",
                                                                  P_("State Hint"),
                                                                  P_("Whether to pass a proper state when drawing shadow or background"),
                                                                  FALSE,
                                                                  BTK_PARAM_READABLE));

  g_type_class_add_private (bobject_class, sizeof (BtkEntryPrivate));
}

static void
btk_entry_editable_init (BtkEditableClass *iface)
{
  iface->do_insert_text = btk_entry_insert_text;
  iface->do_delete_text = btk_entry_delete_text;
  iface->insert_text = btk_entry_real_insert_text;
  iface->delete_text = btk_entry_real_delete_text;
  iface->get_chars = btk_entry_get_chars;
  iface->set_selection_bounds = btk_entry_set_selection_bounds;
  iface->get_selection_bounds = btk_entry_get_selection_bounds;
  iface->set_position = btk_entry_real_set_position;
  iface->get_position = btk_entry_get_position;
}

static void
btk_entry_cell_editable_init (BtkCellEditableIface *iface)
{
  iface->start_editing = btk_entry_start_editing;
}

static void
btk_entry_set_property (GObject         *object,
                        guint            prop_id,
                        const GValue    *value,
                        GParamSpec      *pspec)
{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (object);
  BtkEntry *entry = BTK_ENTRY (object);
  BtkWidget *widget;

  switch (prop_id)
    {
    case PROP_BUFFER:
      btk_entry_set_buffer (entry, g_value_get_object (value));
      break;

    case PROP_EDITABLE:
      {
        gboolean new_value = g_value_get_boolean (value);

      	if (new_value != entry->editable)
	  {
            widget = BTK_WIDGET (entry);
	    if (!new_value)
	      {
		_btk_entry_reset_im_context (entry);
		if (btk_widget_has_focus (widget))
		  btk_im_context_focus_out (entry->im_context);

		entry->preedit_length = 0;
		entry->preedit_cursor = 0;
	      }

	    entry->editable = new_value;

	    if (new_value && btk_widget_has_focus (widget))
	      btk_im_context_focus_in (entry->im_context);
	    
	    btk_entry_queue_draw (entry);
	  }
      }
      break;

    case PROP_MAX_LENGTH:
      btk_entry_set_max_length (entry, g_value_get_int (value));
      break;
      
    case PROP_VISIBILITY:
      btk_entry_set_visibility (entry, g_value_get_boolean (value));
      break;

    case PROP_HAS_FRAME:
      btk_entry_set_has_frame (entry, g_value_get_boolean (value));
      break;

    case PROP_INNER_BORDER:
      btk_entry_set_inner_border (entry, g_value_get_boxed (value));
      break;

    case PROP_INVISIBLE_CHAR:
      btk_entry_set_invisible_char (entry, g_value_get_uint (value));
      break;

    case PROP_ACTIVATES_DEFAULT:
      btk_entry_set_activates_default (entry, g_value_get_boolean (value));
      break;

    case PROP_WIDTH_CHARS:
      btk_entry_set_width_chars (entry, g_value_get_int (value));
      break;

    case PROP_TEXT:
      btk_entry_set_text (entry, g_value_get_string (value));
      break;

    case PROP_XALIGN:
      btk_entry_set_alignment (entry, g_value_get_float (value));
      break;

    case PROP_TRUNCATE_MULTILINE:
      entry->truncate_multiline = g_value_get_boolean (value);
      break;

    case PROP_SHADOW_TYPE:
      priv->shadow_type = g_value_get_enum (value);
      break;

    case PROP_OVERWRITE_MODE:
      btk_entry_set_overwrite_mode (entry, g_value_get_boolean (value));
      break;

    case PROP_INVISIBLE_CHAR_SET:
      if (g_value_get_boolean (value))
        priv->invisible_char_set = TRUE;
      else
        btk_entry_unset_invisible_char (entry);
      break;

    case PROP_CAPS_LOCK_WARNING:
      priv->caps_lock_warning = g_value_get_boolean (value);
      break;

    case PROP_PROGRESS_FRACTION:
      btk_entry_set_progress_fraction (entry, g_value_get_double (value));
      break;

    case PROP_PROGRESS_PULSE_STEP:
      btk_entry_set_progress_pulse_step (entry, g_value_get_double (value));
      break;

    case PROP_PIXBUF_PRIMARY:
      btk_entry_set_icon_from_pixbuf (entry,
                                      BTK_ENTRY_ICON_PRIMARY,
                                      g_value_get_object (value));
      break;

    case PROP_PIXBUF_SECONDARY:
      btk_entry_set_icon_from_pixbuf (entry,
                                      BTK_ENTRY_ICON_SECONDARY,
                                      g_value_get_object (value));
      break;

    case PROP_STOCK_PRIMARY:
      btk_entry_set_icon_from_stock (entry,
                                     BTK_ENTRY_ICON_PRIMARY,
                                     g_value_get_string (value));
      break;

    case PROP_STOCK_SECONDARY:
      btk_entry_set_icon_from_stock (entry,
                                     BTK_ENTRY_ICON_SECONDARY,
                                     g_value_get_string (value));
      break;

    case PROP_ICON_NAME_PRIMARY:
      btk_entry_set_icon_from_icon_name (entry,
                                         BTK_ENTRY_ICON_PRIMARY,
                                         g_value_get_string (value));
      break;

    case PROP_ICON_NAME_SECONDARY:
      btk_entry_set_icon_from_icon_name (entry,
                                         BTK_ENTRY_ICON_SECONDARY,
                                         g_value_get_string (value));
      break;

    case PROP_GICON_PRIMARY:
      btk_entry_set_icon_from_gicon (entry,
                                     BTK_ENTRY_ICON_PRIMARY,
                                     g_value_get_object (value));
      break;

    case PROP_GICON_SECONDARY:
      btk_entry_set_icon_from_gicon (entry,
                                     BTK_ENTRY_ICON_SECONDARY,
                                     g_value_get_object (value));
      break;

    case PROP_ACTIVATABLE_PRIMARY:
      btk_entry_set_icon_activatable (entry,
                                      BTK_ENTRY_ICON_PRIMARY,
                                      g_value_get_boolean (value));
      break;

    case PROP_ACTIVATABLE_SECONDARY:
      btk_entry_set_icon_activatable (entry,
                                      BTK_ENTRY_ICON_SECONDARY,
                                      g_value_get_boolean (value));
      break;

    case PROP_SENSITIVE_PRIMARY:
      btk_entry_set_icon_sensitive (entry,
                                    BTK_ENTRY_ICON_PRIMARY,
                                    g_value_get_boolean (value));
      break;

    case PROP_SENSITIVE_SECONDARY:
      btk_entry_set_icon_sensitive (entry,
                                    BTK_ENTRY_ICON_SECONDARY,
                                    g_value_get_boolean (value));
      break;

    case PROP_TOOLTIP_TEXT_PRIMARY:
      btk_entry_set_icon_tooltip_text (entry,
                                       BTK_ENTRY_ICON_PRIMARY,
                                       g_value_get_string (value));
      break;

    case PROP_TOOLTIP_TEXT_SECONDARY:
      btk_entry_set_icon_tooltip_text (entry,
                                       BTK_ENTRY_ICON_SECONDARY,
                                       g_value_get_string (value));
      break;

    case PROP_TOOLTIP_MARKUP_PRIMARY:
      btk_entry_set_icon_tooltip_markup (entry,
                                         BTK_ENTRY_ICON_PRIMARY,
                                         g_value_get_string (value));
      break;

    case PROP_TOOLTIP_MARKUP_SECONDARY:
      btk_entry_set_icon_tooltip_markup (entry,
                                         BTK_ENTRY_ICON_SECONDARY,
                                         g_value_get_string (value));
      break;

    case PROP_IM_MODULE:
      g_free (priv->im_module);
      priv->im_module = g_value_dup_string (value);
      if (BTK_IS_IM_MULTICONTEXT (entry->im_context))
        btk_im_multicontext_set_context_id (BTK_IM_MULTICONTEXT (entry->im_context), priv->im_module);
      break;

    case PROP_EDITING_CANCELED:
      entry->editing_canceled = g_value_get_boolean (value);
      break;

    case PROP_SCROLL_OFFSET:
    case PROP_CURSOR_POSITION:
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_entry_get_property (GObject         *object,
                        guint            prop_id,
                        GValue          *value,
                        GParamSpec      *pspec)
{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (object);
  BtkEntry *entry = BTK_ENTRY (object);

  switch (prop_id)
    {
    case PROP_BUFFER:
      g_value_set_object (value, btk_entry_get_buffer (entry));
      break;

    case PROP_CURSOR_POSITION:
      g_value_set_int (value, entry->current_pos);
      break;

    case PROP_SELECTION_BOUND:
      g_value_set_int (value, entry->selection_bound);
      break;

    case PROP_EDITABLE:
      g_value_set_boolean (value, entry->editable);
      break;

    case PROP_MAX_LENGTH:
      g_value_set_int (value, btk_entry_buffer_get_max_length (get_buffer (entry)));
      break;

    case PROP_VISIBILITY:
      g_value_set_boolean (value, entry->visible);
      break;

    case PROP_HAS_FRAME:
      g_value_set_boolean (value, entry->has_frame);
      break;

    case PROP_INNER_BORDER:
      g_value_set_boxed (value, btk_entry_get_inner_border (entry));
      break;

    case PROP_INVISIBLE_CHAR:
      g_value_set_uint (value, entry->invisible_char);
      break;

    case PROP_ACTIVATES_DEFAULT:
      g_value_set_boolean (value, entry->activates_default);
      break;

    case PROP_WIDTH_CHARS:
      g_value_set_int (value, entry->width_chars);
      break;

    case PROP_SCROLL_OFFSET:
      g_value_set_int (value, entry->scroll_offset);
      break;

    case PROP_TEXT:
      g_value_set_string (value, btk_entry_get_text (entry));
      break;

    case PROP_XALIGN:
      g_value_set_float (value, btk_entry_get_alignment (entry));
      break;

    case PROP_TRUNCATE_MULTILINE:
      g_value_set_boolean (value, entry->truncate_multiline);
      break;

    case PROP_SHADOW_TYPE:
      g_value_set_enum (value, priv->shadow_type);
      break;

    case PROP_OVERWRITE_MODE:
      g_value_set_boolean (value, entry->overwrite_mode);
      break;

    case PROP_TEXT_LENGTH:
      g_value_set_uint (value, btk_entry_buffer_get_length (get_buffer (entry)));
      break;

    case PROP_INVISIBLE_CHAR_SET:
      g_value_set_boolean (value, priv->invisible_char_set);
      break;

    case PROP_IM_MODULE:
      g_value_set_string (value, priv->im_module);
      break;

    case PROP_CAPS_LOCK_WARNING:
      g_value_set_boolean (value, priv->caps_lock_warning);
      break;

    case PROP_PROGRESS_FRACTION:
      g_value_set_double (value, priv->progress_fraction);
      break;

    case PROP_PROGRESS_PULSE_STEP:
      g_value_set_double (value, priv->progress_pulse_fraction);
      break;

    case PROP_PIXBUF_PRIMARY:
      g_value_set_object (value,
                          btk_entry_get_icon_pixbuf (entry,
                                                     BTK_ENTRY_ICON_PRIMARY));
      break;

    case PROP_PIXBUF_SECONDARY:
      g_value_set_object (value,
                          btk_entry_get_icon_pixbuf (entry,
                                                     BTK_ENTRY_ICON_SECONDARY));
      break;

    case PROP_STOCK_PRIMARY:
      g_value_set_string (value,
                          btk_entry_get_icon_stock (entry,
                                                    BTK_ENTRY_ICON_PRIMARY));
      break;

    case PROP_STOCK_SECONDARY:
      g_value_set_string (value,
                          btk_entry_get_icon_stock (entry,
                                                    BTK_ENTRY_ICON_SECONDARY));
      break;

    case PROP_ICON_NAME_PRIMARY:
      g_value_set_string (value,
                          btk_entry_get_icon_name (entry,
                                                   BTK_ENTRY_ICON_PRIMARY));
      break;

    case PROP_ICON_NAME_SECONDARY:
      g_value_set_string (value,
                          btk_entry_get_icon_name (entry,
                                                   BTK_ENTRY_ICON_SECONDARY));
      break;

    case PROP_GICON_PRIMARY:
      g_value_set_object (value,
                          btk_entry_get_icon_gicon (entry,
                                                    BTK_ENTRY_ICON_PRIMARY));
      break;

    case PROP_GICON_SECONDARY:
      g_value_set_object (value,
                          btk_entry_get_icon_gicon (entry,
                                                    BTK_ENTRY_ICON_SECONDARY));
      break;

    case PROP_STORAGE_TYPE_PRIMARY:
      g_value_set_enum (value,
                        btk_entry_get_icon_storage_type (entry, 
                                                         BTK_ENTRY_ICON_PRIMARY));
      break;

    case PROP_STORAGE_TYPE_SECONDARY:
      g_value_set_enum (value,
                        btk_entry_get_icon_storage_type (entry, 
                                                         BTK_ENTRY_ICON_SECONDARY));
      break;

    case PROP_ACTIVATABLE_PRIMARY:
      g_value_set_boolean (value,
                           btk_entry_get_icon_activatable (entry, BTK_ENTRY_ICON_PRIMARY));
      break;

    case PROP_ACTIVATABLE_SECONDARY:
      g_value_set_boolean (value,
                           btk_entry_get_icon_activatable (entry, BTK_ENTRY_ICON_SECONDARY));
      break;

    case PROP_SENSITIVE_PRIMARY:
      g_value_set_boolean (value,
                           btk_entry_get_icon_sensitive (entry, BTK_ENTRY_ICON_PRIMARY));
      break;

    case PROP_SENSITIVE_SECONDARY:
      g_value_set_boolean (value,
                           btk_entry_get_icon_sensitive (entry, BTK_ENTRY_ICON_SECONDARY));
      break;

    case PROP_TOOLTIP_TEXT_PRIMARY:
      g_value_take_string (value,
                           btk_entry_get_icon_tooltip_text (entry, BTK_ENTRY_ICON_PRIMARY));
      break;

    case PROP_TOOLTIP_TEXT_SECONDARY:
      g_value_take_string (value,
                           btk_entry_get_icon_tooltip_text (entry, BTK_ENTRY_ICON_SECONDARY));
      break;

    case PROP_TOOLTIP_MARKUP_PRIMARY:
      g_value_take_string (value,
                           btk_entry_get_icon_tooltip_markup (entry, BTK_ENTRY_ICON_PRIMARY));
      break;

    case PROP_TOOLTIP_MARKUP_SECONDARY:
      g_value_take_string (value,
                           btk_entry_get_icon_tooltip_markup (entry, BTK_ENTRY_ICON_SECONDARY));
      break;

    case PROP_EDITING_CANCELED:
      g_value_set_boolean (value,
                           entry->editing_canceled);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gunichar
find_invisible_char (BtkWidget *widget)
{
  BangoLayout *layout;
  BangoAttrList *attr_list;
  gint i;
  gunichar invisible_chars [] = {
    0,
    0x25cf, /* BLACK CIRCLE */
    0x2022, /* BULLET */
    0x2731, /* HEAVY ASTERISK */
    0x273a  /* SIXTEEN POINTED ASTERISK */
  };

  if (widget->style)
    btk_widget_style_get (widget,
                          "invisible-char", &invisible_chars[0],
                          NULL);

  layout = btk_widget_create_bango_layout (widget, NULL);

  attr_list = bango_attr_list_new ();
  bango_attr_list_insert (attr_list, bango_attr_fallback_new (FALSE));

  bango_layout_set_attributes (layout, attr_list);
  bango_attr_list_unref (attr_list);

  for (i = (invisible_chars[0] != 0 ? 0 : 1); i < G_N_ELEMENTS (invisible_chars); i++)
    {
      gchar text[7] = { 0, };
      gint len, count;

      len = g_unichar_to_utf8 (invisible_chars[i], text);
      bango_layout_set_text (layout, text, len);

      count = bango_layout_get_unknown_glyphs_count (layout);

      if (count == 0)
        {
          g_object_unref (layout);
          return invisible_chars[i];
        }
    }

  g_object_unref (layout);

  return '*';
}

static void
btk_entry_init (BtkEntry *entry)
{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);

  btk_widget_set_can_focus (BTK_WIDGET (entry), TRUE);

  entry->editable = TRUE;
  entry->visible = TRUE;
  entry->invisible_char = find_invisible_char (BTK_WIDGET (entry));
  entry->dnd_position = -1;
  entry->width_chars = -1;
  entry->is_cell_renderer = FALSE;
  entry->editing_canceled = FALSE;
  entry->has_frame = TRUE;
  entry->truncate_multiline = FALSE;
  priv->shadow_type = BTK_SHADOW_IN;
  priv->xalign = 0.0;
  priv->caps_lock_warning = TRUE;
  priv->caps_lock_warning_shown = FALSE;
  priv->progress_fraction = 0.0;
  priv->progress_pulse_fraction = 0.1;

  btk_drag_dest_set (BTK_WIDGET (entry),
                     BTK_DEST_DEFAULT_HIGHLIGHT,
                     NULL, 0,
                     BDK_ACTION_COPY | BDK_ACTION_MOVE);
  btk_drag_dest_add_text_targets (BTK_WIDGET (entry));

  /* This object is completely private. No external entity can gain a reference
   * to it; so we create it here and destroy it in finalize().
   */
  entry->im_context = btk_im_multicontext_new ();
  
  g_signal_connect (entry->im_context, "commit",
		    G_CALLBACK (btk_entry_commit_cb), entry);
  g_signal_connect (entry->im_context, "preedit-changed",
		    G_CALLBACK (btk_entry_preedit_changed_cb), entry);
  g_signal_connect (entry->im_context, "retrieve-surrounding",
		    G_CALLBACK (btk_entry_retrieve_surrounding_cb), entry);
  g_signal_connect (entry->im_context, "delete-surrounding",
		    G_CALLBACK (btk_entry_delete_surrounding_cb), entry);

}

static gint
get_icon_width (BtkEntry             *entry,
                BtkEntryIconPosition  icon_pos)
{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);
  EntryIconInfo *icon_info = priv->icons[icon_pos];
  BdkScreen *screen;
  BtkSettings *settings;
  gint menu_icon_width;

  if (!icon_info || icon_info->pixbuf == NULL)
    return 0;

  screen = btk_widget_get_screen (BTK_WIDGET (entry));
  settings = btk_settings_get_for_screen (screen);

  btk_icon_size_lookup_for_settings (settings, BTK_ICON_SIZE_MENU,
                                     &menu_icon_width, NULL);

  return MAX (bdk_pixbuf_get_width (icon_info->pixbuf), menu_icon_width);
}

static void
get_icon_allocations (BtkEntry      *entry,
                      BtkAllocation *primary,
                      BtkAllocation *secondary)

{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);
  gint x, y, width, height;

  get_text_area_size (entry, &x, &y, &width, &height);

  if (btk_widget_has_focus (BTK_WIDGET (entry)) && !priv->interior_focus)
    y += priv->focus_width;

  primary->y = y;
  primary->height = height;
  primary->width = get_icon_width (entry, BTK_ENTRY_ICON_PRIMARY);
  if (primary->width > 0)
    primary->width += 2 * priv->icon_margin;

  secondary->y = y;
  secondary->height = height;
  secondary->width = get_icon_width (entry, BTK_ENTRY_ICON_SECONDARY);
  if (secondary->width > 0)
    secondary->width += 2 * priv->icon_margin;

  if (btk_widget_get_direction (BTK_WIDGET (entry)) == BTK_TEXT_DIR_RTL)
    {
      primary->x = x + width - primary->width;
      secondary->x = x;
    }
  else
    {
      primary->x = x;
      secondary->x = x + width - secondary->width;
    }
}


static void
begin_change (BtkEntry *entry)
{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);

  priv->change_count++;

  g_object_freeze_notify (G_OBJECT (entry));
}

static void
end_change (BtkEntry *entry)
{
  BtkEditable *editable = BTK_EDITABLE (entry);
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);
 
  g_return_if_fail (priv->change_count > 0);

  g_object_thaw_notify (G_OBJECT (entry));

  priv->change_count--;

  if (priv->change_count == 0)
    {
       if (priv->real_changed) 
         {
           g_signal_emit_by_name (editable, "changed");
           priv->real_changed = FALSE;
         }
    } 
}

static void
emit_changed (BtkEntry *entry)
{
  BtkEditable *editable = BTK_EDITABLE (entry);
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);

  if (priv->change_count == 0)
    g_signal_emit_by_name (editable, "changed");
  else 
    priv->real_changed = TRUE;
}

static void
btk_entry_destroy (BtkObject *object)
{
  BtkEntry *entry = BTK_ENTRY (object);

  entry->current_pos = entry->selection_bound = 0;
  _btk_entry_reset_im_context (entry);
  btk_entry_reset_layout (entry);

  if (entry->blink_timeout)
    {
      g_source_remove (entry->blink_timeout);
      entry->blink_timeout = 0;
    }

  if (entry->recompute_idle)
    {
      g_source_remove (entry->recompute_idle);
      entry->recompute_idle = 0;
    }

  BTK_OBJECT_CLASS (btk_entry_parent_class)->destroy (object);
}

static void
btk_entry_dispose (GObject *object)
{
  BtkEntry *entry = BTK_ENTRY (object);
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);
  BdkKeymap *keymap;

  btk_entry_set_icon_from_pixbuf (entry, BTK_ENTRY_ICON_PRIMARY, NULL);
  btk_entry_set_icon_tooltip_markup (entry, BTK_ENTRY_ICON_PRIMARY, NULL);
  btk_entry_set_icon_from_pixbuf (entry, BTK_ENTRY_ICON_SECONDARY, NULL);
  btk_entry_set_icon_tooltip_markup (entry, BTK_ENTRY_ICON_SECONDARY, NULL);
  btk_entry_set_completion (entry, NULL);

  if (priv->buffer)
    {
      buffer_disconnect_signals (entry);
      g_object_unref (priv->buffer);
      priv->buffer = NULL;
    }

  keymap = bdk_keymap_get_for_display (btk_widget_get_display (BTK_WIDGET (object)));
  g_signal_handlers_disconnect_by_func (keymap, keymap_state_changed, entry);
  g_signal_handlers_disconnect_by_func (keymap, keymap_direction_changed, entry);

  G_OBJECT_CLASS (btk_entry_parent_class)->dispose (object);
}

static void
btk_entry_finalize (GObject *object)
{
  BtkEntry *entry = BTK_ENTRY (object);
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);
  EntryIconInfo *icon_info = NULL;
  gint i;

  for (i = 0; i < MAX_ICONS; i++)
    {
      if ((icon_info = priv->icons[i]) != NULL)
        {
          if (icon_info->target_list != NULL)
            {
              btk_target_list_unref (icon_info->target_list);
              icon_info->target_list = NULL;
            }

          g_slice_free (EntryIconInfo, icon_info);
          priv->icons[i] = NULL;
        }
    }

  if (entry->cached_layout)
    g_object_unref (entry->cached_layout);

  g_object_unref (entry->im_context);

  if (entry->blink_timeout)
    g_source_remove (entry->blink_timeout);

  if (entry->recompute_idle)
    g_source_remove (entry->recompute_idle);

  g_free (priv->im_module);

  G_OBJECT_CLASS (btk_entry_parent_class)->finalize (object);
}

static DisplayMode
btk_entry_get_display_mode (BtkEntry *entry)
{
  BtkEntryPrivate *priv;
  if (entry->visible)
    return DISPLAY_NORMAL;
  priv = BTK_ENTRY_GET_PRIVATE (entry);
  if (entry->invisible_char == 0 && priv->invisible_char_set)
    return DISPLAY_BLANK;
  return DISPLAY_INVISIBLE;
}

static gchar*
btk_entry_get_display_text (BtkEntry *entry,
                            gint      start_pos,
                            gint      end_pos)
{
  BtkEntryPasswordHint *password_hint;
  BtkEntryPrivate *priv;
  gunichar invisible_char;
  const gchar *start;
  const gchar *end;
  const gchar *text;
  gchar char_str[7];
  gint char_len;
  GString *str;
  guint length;
  gint i;

  priv = BTK_ENTRY_GET_PRIVATE (entry);
  text = btk_entry_buffer_get_text (get_buffer (entry));
  length = btk_entry_buffer_get_length (get_buffer (entry));

  if (end_pos < 0)
    end_pos = length;
  if (start_pos > length)
    start_pos = length;

  if (end_pos <= start_pos)
      return g_strdup ("");
  else if (entry->visible)
    {
      start = g_utf8_offset_to_pointer (text, start_pos);
      end = g_utf8_offset_to_pointer (start, end_pos - start_pos);
      return g_strndup (start, end - start);
    }
  else
    {
      str = g_string_sized_new (length * 2);

      /* Figure out what our invisible char is and encode it */
      if (!entry->invisible_char)
          invisible_char = priv->invisible_char_set ? ' ' : '*';
      else
          invisible_char = entry->invisible_char;
      char_len = g_unichar_to_utf8 (invisible_char, char_str);

      /*
       * Add hidden characters for each character in the text
       * buffer. If there is a password hint, then keep that
       * character visible.
       */

      password_hint = g_object_get_qdata (G_OBJECT (entry), quark_password_hint);
      for (i = start_pos; i < end_pos; ++i)
        {
          if (password_hint && i == password_hint->position)
            {
              start = g_utf8_offset_to_pointer (text, i);
              g_string_append_len (str, start, g_utf8_next_char (start) - start);
            }
          else
            {
              g_string_append_len (str, char_str, char_len);
            }
        }

      return g_string_free (str, FALSE);
    }

}

static void
update_cursors (BtkWidget *widget)
{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (widget);
  EntryIconInfo *icon_info = NULL;
  BdkDisplay *display;
  BdkCursor *cursor;
  gint i;

  for (i = 0; i < MAX_ICONS; i++)
    {
      if ((icon_info = priv->icons[i]) != NULL)
        {
          if (icon_info->pixbuf != NULL && icon_info->window != NULL)
            bdk_window_show_unraised (icon_info->window);

          /* The icon windows are not children of the visible entry window,
           * thus we can't just inherit the xterm cursor. Slight complication 
           * here is that for the entry, insensitive => arrow cursor, but for 
           * an icon in a sensitive entry, insensitive => xterm cursor.
           */
          if (btk_widget_is_sensitive (widget) &&
              (icon_info->insensitive || 
               (icon_info->nonactivatable && icon_info->target_list == NULL)))
            {
              display = btk_widget_get_display (widget);
              cursor = bdk_cursor_new_for_display (display, BDK_XTERM);
              bdk_window_set_cursor (icon_info->window, cursor);
              bdk_cursor_unref (cursor);
            }
          else
            {
              bdk_window_set_cursor (icon_info->window, NULL);
            }
        }
    }
}

static void
realize_icon_info (BtkWidget            *widget, 
                   BtkEntryIconPosition  icon_pos)
{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (widget);
  EntryIconInfo *icon_info = priv->icons[icon_pos];
  BdkWindowAttr attributes;
  gint attributes_mask;

  g_return_if_fail (icon_info != NULL);

  attributes.x = 0;
  attributes.y = 0;
  attributes.width = 1;
  attributes.height = 1;
  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);
  attributes.event_mask = btk_widget_get_events (widget);
  attributes.event_mask |= (BDK_EXPOSURE_MASK |
                                BDK_BUTTON_PRESS_MASK |
                                BDK_BUTTON_RELEASE_MASK |
                                BDK_BUTTON1_MOTION_MASK |
                                BDK_BUTTON3_MOTION_MASK |
                                BDK_POINTER_MOTION_HINT_MASK |
                                BDK_POINTER_MOTION_MASK |
                                BDK_ENTER_NOTIFY_MASK |
                            BDK_LEAVE_NOTIFY_MASK);
  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;

  icon_info->window = bdk_window_new (widget->window,
                                      &attributes,
                                      attributes_mask);
  bdk_window_set_user_data (icon_info->window, widget);
  bdk_window_set_background (icon_info->window,
                             &widget->style->base[btk_widget_get_state (widget)]);

  btk_widget_queue_resize (widget);
}

static EntryIconInfo*
construct_icon_info (BtkWidget            *widget, 
                     BtkEntryIconPosition  icon_pos)
{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (widget);
  EntryIconInfo *icon_info;

  g_return_val_if_fail (priv->icons[icon_pos] == NULL, NULL);

  icon_info = g_slice_new0 (EntryIconInfo);
  priv->icons[icon_pos] = icon_info;

  if (btk_widget_get_realized (widget))
    realize_icon_info (widget, icon_pos);

  return icon_info;
}

static void
btk_entry_map (BtkWidget *widget)
{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (widget);
  EntryIconInfo *icon_info = NULL;
  gint i;

  if (btk_widget_get_realized (widget) && !btk_widget_get_mapped (widget))
    {
      BTK_WIDGET_CLASS (btk_entry_parent_class)->map (widget);

      for (i = 0; i < MAX_ICONS; i++)
        {
          if ((icon_info = priv->icons[i]) != NULL)
            {
              if (icon_info->pixbuf != NULL && icon_info->window != NULL)
                bdk_window_show (icon_info->window);
            }
        }

      update_cursors (widget);
    }
}

static void
btk_entry_unmap (BtkWidget *widget)
{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (widget);
  EntryIconInfo *icon_info = NULL;
  gint i;

  if (btk_widget_get_mapped (widget))
    {
      for (i = 0; i < MAX_ICONS; i++)
        {
          if ((icon_info = priv->icons[i]) != NULL)
            {
              if (icon_info->pixbuf != NULL && icon_info->window != NULL)
                bdk_window_hide (icon_info->window);
            }
        }

      BTK_WIDGET_CLASS (btk_entry_parent_class)->unmap (widget);
    }
}

static void
btk_entry_realize (BtkWidget *widget)
{
  BtkEntry *entry;
  BtkEntryPrivate *priv;
  EntryIconInfo *icon_info;
  BdkWindowAttr attributes;
  gint attributes_mask;
  int i;

  btk_widget_set_realized (widget, TRUE);
  entry = BTK_ENTRY (widget);
  priv = BTK_ENTRY_GET_PRIVATE (entry);

  attributes.window_type = BDK_WINDOW_CHILD;
  
  get_widget_window_size (entry, &attributes.x, &attributes.y, &attributes.width, &attributes.height);

  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);
  attributes.event_mask = btk_widget_get_events (widget);
  attributes.event_mask |= (BDK_EXPOSURE_MASK |
			    BDK_BUTTON_PRESS_MASK |
			    BDK_BUTTON_RELEASE_MASK |
			    BDK_BUTTON1_MOTION_MASK |
			    BDK_BUTTON3_MOTION_MASK |
			    BDK_POINTER_MOTION_HINT_MASK |
			    BDK_POINTER_MOTION_MASK |
                            BDK_ENTER_NOTIFY_MASK |
			    BDK_LEAVE_NOTIFY_MASK);
  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;

  widget->window = bdk_window_new (btk_widget_get_parent_window (widget), &attributes, attributes_mask);
  bdk_window_set_user_data (widget->window, entry);

  get_text_area_size (entry, &attributes.x, &attributes.y, &attributes.width, &attributes.height);
 
  if (btk_widget_is_sensitive (widget))
    {
      attributes.cursor = bdk_cursor_new_for_display (btk_widget_get_display (widget), BDK_XTERM);
      attributes_mask |= BDK_WA_CURSOR;
    }

  entry->text_area = bdk_window_new (widget->window, &attributes, attributes_mask);

  bdk_window_set_user_data (entry->text_area, entry);

  if (attributes_mask & BDK_WA_CURSOR)
    bdk_cursor_unref (attributes.cursor);

  widget->style = btk_style_attach (widget->style, widget->window);

  bdk_window_set_background (widget->window, &widget->style->base[btk_widget_get_state (widget)]);
  bdk_window_set_background (entry->text_area, &widget->style->base[btk_widget_get_state (widget)]);

  bdk_window_show (entry->text_area);

  btk_im_context_set_client_window (entry->im_context, entry->text_area);

  btk_entry_adjust_scroll (entry);
  btk_entry_update_primary_selection (entry);


  /* If the icon positions are already setup, create their windows.
   * Otherwise if they don't exist yet, then construct_icon_info()
   * will create the windows once the widget is already realized.
   */
  for (i = 0; i < MAX_ICONS; i++)
    {
      if ((icon_info = priv->icons[i]) != NULL)
        {
          if (icon_info->window == NULL)
            realize_icon_info (widget, i);
        }
    }
}

static void
btk_entry_unrealize (BtkWidget *widget)
{
  BtkEntry *entry = BTK_ENTRY (widget);
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);
  BtkClipboard *clipboard;
  EntryIconInfo *icon_info;
  gint i;

  btk_entry_reset_layout (entry);
  
  btk_im_context_set_client_window (entry->im_context, NULL);

  clipboard = btk_widget_get_clipboard (widget, BDK_SELECTION_PRIMARY);
  if (btk_clipboard_get_owner (clipboard) == G_OBJECT (entry))
    btk_clipboard_clear (clipboard);
  
  if (entry->text_area)
    {
      bdk_window_set_user_data (entry->text_area, NULL);
      bdk_window_destroy (entry->text_area);
      entry->text_area = NULL;
    }

  if (entry->popup_menu)
    {
      btk_widget_destroy (entry->popup_menu);
      entry->popup_menu = NULL;
    }

  BTK_WIDGET_CLASS (btk_entry_parent_class)->unrealize (widget);

  for (i = 0; i < MAX_ICONS; i++)
    {
      if ((icon_info = priv->icons[i]) != NULL)
        {
          if (icon_info->window != NULL)
            {
              bdk_window_destroy (icon_info->window);
              icon_info->window = NULL;
            }
        }
    }
}

void
_btk_entry_get_borders (BtkEntry *entry,
			gint     *xborder,
			gint     *yborder)
{
  BtkWidget *widget = BTK_WIDGET (entry);
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (widget);

  if (entry->has_frame)
    {
      *xborder = widget->style->xthickness;
      *yborder = widget->style->ythickness;
    }
  else
    {
      *xborder = 0;
      *yborder = 0;
    }

  if (!priv->interior_focus)
    {
      *xborder += priv->focus_width;
      *yborder += priv->focus_width;
    }
}

static void
btk_entry_size_request (BtkWidget      *widget,
			BtkRequisition *requisition)
{
  BtkEntry *entry = BTK_ENTRY (widget);
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);
  BangoFontMetrics *metrics;
  gint xborder, yborder;
  BtkBorder inner_border;
  BangoContext *context;
  int icon_widths = 0;
  int icon_width, i;
  
  btk_widget_ensure_style (widget);
  context = btk_widget_get_bango_context (widget);
  metrics = bango_context_get_metrics (context,
				       widget->style->font_desc,
				       bango_context_get_language (context));

  entry->ascent = bango_font_metrics_get_ascent (metrics);
  entry->descent = bango_font_metrics_get_descent (metrics);
  
  _btk_entry_get_borders (entry, &xborder, &yborder);
  _btk_entry_effective_inner_border (entry, &inner_border);

  if (entry->width_chars < 0)
    requisition->width = MIN_ENTRY_WIDTH + xborder * 2 + inner_border.left + inner_border.right;
  else
    {
      gint char_width = bango_font_metrics_get_approximate_char_width (metrics);
      gint digit_width = bango_font_metrics_get_approximate_digit_width (metrics);
      gint char_pixels = (MAX (char_width, digit_width) + BANGO_SCALE - 1) / BANGO_SCALE;
      
      requisition->width = char_pixels * entry->width_chars + xborder * 2 + inner_border.left + inner_border.right;
    }
    
  requisition->height = BANGO_PIXELS (entry->ascent + entry->descent) + yborder * 2 + inner_border.top + inner_border.bottom;

  for (i = 0; i < MAX_ICONS; i++)
    {
      icon_width = get_icon_width (entry, i);
      if (icon_width > 0)
        icon_widths += icon_width + 2 * priv->icon_margin;
    }

  if (icon_widths > requisition->width)
    requisition->width += icon_widths;

  bango_font_metrics_unref (metrics);
}

static void
place_windows (BtkEntry *entry)
{
  BtkWidget *widget = BTK_WIDGET (entry);
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);
  gint x, y, width, height;
  BtkAllocation primary;
  BtkAllocation secondary;
  EntryIconInfo *icon_info = NULL;

  get_text_area_size (entry, &x, &y, &width, &height);
  get_icon_allocations (entry, &primary, &secondary);

  if (btk_widget_has_focus (widget) && !priv->interior_focus)
    y += priv->focus_width;

  if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL)
    x += secondary.width;
  else
    x += primary.width;
  width -= primary.width + secondary.width;

  if ((icon_info = priv->icons[BTK_ENTRY_ICON_PRIMARY]) != NULL)
    bdk_window_move_resize (icon_info->window,
                            primary.x, primary.y,
                            primary.width, primary.height);

  if ((icon_info = priv->icons[BTK_ENTRY_ICON_SECONDARY]) != NULL)
    bdk_window_move_resize (icon_info->window,
                            secondary.x, secondary.y,
                            secondary.width, secondary.height);

  bdk_window_move_resize (entry->text_area, x, y, width, height);
}

static void
btk_entry_get_text_area_size (BtkEntry *entry,
                              gint     *x,
			      gint     *y,
			      gint     *width,
			      gint     *height)
{
  BtkWidget *widget = BTK_WIDGET (entry);
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (widget);
  gint frame_height;
  gint xborder, yborder;
  BtkRequisition requisition;

  btk_widget_get_child_requisition (widget, &requisition);
  _btk_entry_get_borders (entry, &xborder, &yborder);

  if (btk_widget_get_realized (widget))
    frame_height = bdk_window_get_height (widget->window);
  else
    frame_height = requisition.height;

  if (btk_widget_has_focus (widget) && !priv->interior_focus)
    frame_height -= 2 * priv->focus_width;

  if (x)
    *x = xborder;

  if (y)
    *y = frame_height / 2 - (requisition.height - yborder * 2) / 2;

  if (width)
    *width = BTK_WIDGET (entry)->allocation.width - xborder * 2;

  if (height)
    *height = requisition.height - yborder * 2;
}

static void
get_text_area_size (BtkEntry *entry,
                    gint     *x,
                    gint     *y,
                    gint     *width,
                    gint     *height)
{
  BtkEntryClass *class;

  g_return_if_fail (BTK_IS_ENTRY (entry));

  class = BTK_ENTRY_GET_CLASS (entry);

  if (class->get_text_area_size)
    class->get_text_area_size (entry, x, y, width, height);
}


static void
get_widget_window_size (BtkEntry *entry,
                        gint     *x,
                        gint     *y,
                        gint     *width,
                        gint     *height)
{
  BtkRequisition requisition;
  BtkWidget *widget = BTK_WIDGET (entry);
      
  btk_widget_get_child_requisition (widget, &requisition);

  if (x)
    *x = widget->allocation.x;

  if (y)
    {
      if (entry->is_cell_renderer)
	*y = widget->allocation.y;
      else
	*y = widget->allocation.y + (widget->allocation.height - requisition.height) / 2;
    }

  if (width)
    *width = widget->allocation.width;

  if (height)
    {
      if (entry->is_cell_renderer)
	*height = widget->allocation.height;
      else
	*height = requisition.height;
    }
}

void
_btk_entry_effective_inner_border (BtkEntry  *entry,
                                   BtkBorder *border)
{
  BtkBorder *tmp_border;

  tmp_border = g_object_get_qdata (G_OBJECT (entry), quark_inner_border);

  if (tmp_border)
    {
      *border = *tmp_border;
      return;
    }

  btk_widget_style_get (BTK_WIDGET (entry), "inner-border", &tmp_border, NULL);

  if (tmp_border)
    {
      *border = *tmp_border;
      btk_border_free (tmp_border);
      return;
    }

  *border = default_inner_border;
}

static void
btk_entry_size_allocate (BtkWidget     *widget,
			 BtkAllocation *allocation)
{
  BtkEntry *entry = BTK_ENTRY (widget);
  
  widget->allocation = *allocation;
  
  if (btk_widget_get_realized (widget))
    {
      /* We call btk_widget_get_child_requisition, since we want (for
       * backwards compatibility reasons) the realization here to
       * be affected by the usize of the entry, if set
       */
      gint x, y, width, height;
      BtkEntryCompletion* completion;

      get_widget_window_size (entry, &x, &y, &width, &height);
      bdk_window_move_resize (widget->window, x, y, width, height);

      place_windows (entry);
      btk_entry_recompute (entry);

      completion = btk_entry_get_completion (entry);
      if (completion && btk_widget_get_mapped (completion->priv->popup_window))
        _btk_entry_completion_resize_popup (completion);
    }
}

/* Kudos to the gnome-panel guys. */
static void
colorshift_pixbuf (BdkPixbuf *dest,
                   BdkPixbuf *src,
                   gint       shift)
{
  gint i, j;
  gint width, height, has_alpha, src_rowstride, dest_rowstride;
  guchar *target_pixels;
  guchar *original_pixels;
  guchar *pix_src;
  guchar *pix_dest;
  int val;
  guchar r, g, b;

  has_alpha       = bdk_pixbuf_get_has_alpha (src);
  width           = bdk_pixbuf_get_width (src);
  height          = bdk_pixbuf_get_height (src);
  src_rowstride   = bdk_pixbuf_get_rowstride (src);
  dest_rowstride  = bdk_pixbuf_get_rowstride (dest);
  original_pixels = bdk_pixbuf_get_pixels (src);
  target_pixels   = bdk_pixbuf_get_pixels (dest);

  for (i = 0; i < height; i++)
    {
      pix_dest = target_pixels   + i * dest_rowstride;
      pix_src  = original_pixels + i * src_rowstride;

      for (j = 0; j < width; j++)
        {
          r = *(pix_src++);
          g = *(pix_src++);
          b = *(pix_src++);

          val = r + shift;
          *(pix_dest++) = CLAMP (val, 0, 255);

          val = g + shift;
          *(pix_dest++) = CLAMP (val, 0, 255);

          val = b + shift;
          *(pix_dest++) = CLAMP (val, 0, 255);

          if (has_alpha)
            *(pix_dest++) = *(pix_src++);
        }
    }
}

static gboolean
should_prelight (BtkEntry             *entry,
                 BtkEntryIconPosition  icon_pos)
{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);
  EntryIconInfo *icon_info = priv->icons[icon_pos];
  gboolean prelight;

  if (!icon_info) 
    return FALSE;

  if (icon_info->nonactivatable && icon_info->target_list == NULL)
    return FALSE;

  if (icon_info->pressed)
    return FALSE;

  btk_widget_style_get (BTK_WIDGET (entry),
                        "icon-prelight", &prelight,
                        NULL);

  return prelight;
}

static void
draw_icon (BtkWidget            *widget,
           BtkEntryIconPosition  icon_pos)
{
  BtkEntry *entry = BTK_ENTRY (widget);
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);
  EntryIconInfo *icon_info = priv->icons[icon_pos];
  BdkPixbuf *pixbuf;
  gint x, y, width, height;
  bairo_t *cr;

  if (!icon_info)
    return;

  btk_entry_ensure_pixbuf (entry, icon_pos);

  if (icon_info->pixbuf == NULL)
    return;

  width = bdk_window_get_width (icon_info->window);
  height = bdk_window_get_height (icon_info->window);

  /* size_allocate hasn't been called yet. These are the default values.
   */
  if (width == 1 || height == 1)
    return;

  pixbuf = icon_info->pixbuf;
  g_object_ref (pixbuf);

  if (bdk_pixbuf_get_height (pixbuf) > height)
    {
      BdkPixbuf *temp_pixbuf;
      gint scale;

      scale = height - 2 * priv->icon_margin;
      temp_pixbuf = bdk_pixbuf_scale_simple (pixbuf, scale, scale,
                                             BDK_INTERP_BILINEAR);
      g_object_unref (pixbuf);
      pixbuf = temp_pixbuf;
    }

  x = (width  - bdk_pixbuf_get_width (pixbuf)) / 2;
  y = (height - bdk_pixbuf_get_height (pixbuf)) / 2;

  if (!btk_widget_is_sensitive (widget) ||
      icon_info->insensitive)
    {
      BdkPixbuf *temp_pixbuf;

      temp_pixbuf = bdk_pixbuf_copy (pixbuf);
      bdk_pixbuf_saturate_and_pixelate (pixbuf,
                                        temp_pixbuf,
                                        0.8f,
                                        TRUE);
      g_object_unref (pixbuf);
      pixbuf = temp_pixbuf;
    }
  else if (icon_info->prelight)
    {
      BdkPixbuf *temp_pixbuf;

      temp_pixbuf = bdk_pixbuf_copy (pixbuf);
      colorshift_pixbuf (temp_pixbuf, pixbuf, 30);
      g_object_unref (pixbuf);
      pixbuf = temp_pixbuf;
    }

  cr = bdk_bairo_create (icon_info->window);
  bdk_bairo_set_source_pixbuf (cr, pixbuf, x, y);
  bairo_paint (cr);
  bairo_destroy (cr);

  g_object_unref (pixbuf);
}


static void
btk_entry_draw_frame (BtkWidget      *widget,
                      BdkEventExpose *event)
{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (widget);
  gint x = 0, y = 0, width, height;
  gboolean state_hint;
  BtkStateType state;

  width = bdk_window_get_width (widget->window);
  height = bdk_window_get_height (widget->window);

  /* Fix a problem with some themes which assume that entry->text_area's
   * width equals widget->window's width */
  if (BTK_IS_SPIN_BUTTON (widget))
    {
      gint xborder, yborder;

      btk_entry_get_text_area_size (BTK_ENTRY (widget), &x, NULL, &width, NULL);
      _btk_entry_get_borders (BTK_ENTRY (widget), &xborder, &yborder);

      x -= xborder;
      width += xborder * 2;
    }

  if (btk_widget_has_focus (widget) && !priv->interior_focus)
    {
      x += priv->focus_width;
      y += priv->focus_width;
      width -= 2 * priv->focus_width;
      height -= 2 * priv->focus_width;
    }

  btk_widget_style_get (widget, "state-hint", &state_hint, NULL);
  if (state_hint)
      state = btk_widget_has_focus (widget) ?
        BTK_STATE_ACTIVE : btk_widget_get_state (widget);
  else
      state = BTK_STATE_NORMAL;

  btk_paint_shadow (widget->style, widget->window,
                    state, priv->shadow_type,
                    &event->area, widget, "entry", x, y, width, height);


  btk_entry_draw_progress (widget, event);

  if (btk_widget_has_focus (widget) && !priv->interior_focus)
    {
      x -= priv->focus_width;
      y -= priv->focus_width;
      width += 2 * priv->focus_width;
      height += 2 * priv->focus_width;
      
      btk_paint_focus (widget->style, widget->window,
                       btk_widget_get_state (widget),
		       &event->area, widget, "entry",
		       0, 0, width, height);
    }
}

static void
get_progress_area (BtkWidget *widget,
                   gint       *x,
                   gint       *y,
                   gint       *width,
                   gint       *height)
{
  BtkEntryPrivate *private = BTK_ENTRY_GET_PRIVATE (widget);
  BtkEntry *entry = BTK_ENTRY (widget);
  BtkBorder *progress_border;

  get_text_area_size (entry, x, y, width, height);

  if (!private->interior_focus)
    {
      *x -= private->focus_width;
      *y -= private->focus_width;
      *width += 2 * private->focus_width;
      *height += 2 * private->focus_width;
    }

  btk_widget_style_get (widget, "progress-border", &progress_border, NULL);

  if (progress_border)
    {
      *x += progress_border->left;
      *y += progress_border->top;
      *width -= progress_border->left + progress_border->right;
      *height -= progress_border->top + progress_border->bottom;

      btk_border_free (progress_border);
    }

  if (private->progress_pulse_mode)
    {
      gdouble value = private->progress_pulse_current;

      *x += (gint) floor(value * (*width));
      *width = (gint) ceil(private->progress_pulse_fraction * (*width));
    }
  else if (private->progress_fraction > 0)
    {
      gdouble value = private->progress_fraction;

      if (btk_widget_get_direction (BTK_WIDGET (entry)) == BTK_TEXT_DIR_RTL)
        {
          gint bar_width;

          bar_width = floor(value * (*width) + 0.5);
          *x += *width - bar_width;
          *width = bar_width;
        }
      else
        {
          *width = (gint) floor(value * (*width) + 0.5);
        }
    }
  else
    {
      *width = 0;
      *height = 0;
    }
}

static void
btk_entry_draw_progress (BtkWidget      *widget,
                         BdkEventExpose *event)
{
  gint x, y, width, height;
  BtkStateType state;

  get_progress_area (widget, &x, &y, &width, &height);

  if ((width <= 0) || (height <= 0))
    return;

  if (event->window != widget->window)
    {
      gint pos_x, pos_y;

      bdk_window_get_position (event->window, &pos_x, &pos_y);

      x -= pos_x;
      y -= pos_y;
    }

  state = BTK_STATE_SELECTED;
  if (!btk_widget_get_sensitive (widget))
    state = BTK_STATE_INSENSITIVE;

  btk_paint_box (widget->style, event->window,
                 state, BTK_SHADOW_OUT,
                 &event->area, widget, "entry-progress",
                 x, y,
                 width, height);
}

static gint
btk_entry_expose (BtkWidget      *widget,
		  BdkEventExpose *event)
{
  BtkEntry *entry = BTK_ENTRY (widget);
  gboolean state_hint;
  BtkStateType state;
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);

  btk_widget_style_get (widget, "state-hint", &state_hint, NULL);
  if (state_hint)
    state = btk_widget_has_focus (widget) ?
      BTK_STATE_ACTIVE : btk_widget_get_state (widget);
  else
    state = btk_widget_get_state(widget);

  if (widget->window == event->window)
    {
      btk_entry_draw_frame (widget, event);
    }
  else if (entry->text_area == event->window)
    {
      gint width, height;

      width = bdk_window_get_width (entry->text_area);
      height = bdk_window_get_height (entry->text_area);

      btk_paint_flat_box (widget->style, entry->text_area, 
			  state, BTK_SHADOW_NONE,
			  &event->area, widget, "entry_bg",
			  0, 0, width, height);

      btk_entry_draw_progress (widget, event);

      if (entry->dnd_position != -1)
	btk_entry_draw_cursor (BTK_ENTRY (widget), CURSOR_DND);
      
      btk_entry_draw_text (BTK_ENTRY (widget));

      /* When no text is being displayed at all, don't show the cursor */
      if (btk_entry_get_display_mode (entry) != DISPLAY_BLANK &&
	  btk_widget_has_focus (widget) &&
	  entry->selection_bound == entry->current_pos && entry->cursor_visible) 
        btk_entry_draw_cursor (BTK_ENTRY (widget), CURSOR_STANDARD);
    }
  else
    {
      int i;

      for (i = 0; i < MAX_ICONS; i++)
        {
          EntryIconInfo *icon_info = priv->icons[i];

          if (icon_info != NULL && event->window == icon_info->window)
            {
              gint width, height;

              width = bdk_window_get_width (icon_info->window);
              height = bdk_window_get_height (icon_info->window);

              btk_paint_flat_box (widget->style, icon_info->window,
                                  btk_widget_get_state (widget), BTK_SHADOW_NONE,
                                  NULL, widget, "entry_bg",
                                  0, 0, width, height);

              btk_entry_draw_progress (widget, event);
              draw_icon (widget, i);

              break;
            }
        }
    }

  return FALSE;
}

static gint
btk_entry_enter_notify (BtkWidget *widget,
                        BdkEventCrossing *event)
{
  BtkEntry *entry = BTK_ENTRY (widget);
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);
  gint i;

  for (i = 0; i < MAX_ICONS; i++)
    {
      EntryIconInfo *icon_info = priv->icons[i];

      if (icon_info != NULL && event->window == icon_info->window)
        {
          if (should_prelight (entry, i))
            {
              icon_info->prelight = TRUE;
              btk_widget_queue_draw (widget);
            }

          break;
        }
    }

    return FALSE;
}

static gint
btk_entry_leave_notify (BtkWidget        *widget,
                        BdkEventCrossing *event)
{
  BtkEntry *entry = BTK_ENTRY (widget);
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);
  gint i;

  for (i = 0; i < MAX_ICONS; i++)
    {
      EntryIconInfo *icon_info = priv->icons[i];

      if (icon_info != NULL && event->window == icon_info->window)
        {
          /* a grab means that we may never see the button release */
          if (event->mode == BDK_CROSSING_GRAB || event->mode == BDK_CROSSING_BTK_GRAB)
            icon_info->pressed = FALSE;

          if (should_prelight (entry, i))
            {
              icon_info->prelight = FALSE;
              btk_widget_queue_draw (widget);
            }

          break;
        }
    }

  return FALSE;
}

static void
btk_entry_get_pixel_ranges (BtkEntry  *entry,
			    gint     **ranges,
			    gint      *n_ranges)
{
  gint start_char, end_char;

  if (btk_editable_get_selection_bounds (BTK_EDITABLE (entry), &start_char, &end_char))
    {
      BangoLayout *layout = btk_entry_ensure_layout (entry, TRUE);
      BangoLayoutLine *line = bango_layout_get_lines_readonly (layout)->data;
      const char *text = bango_layout_get_text (layout);
      gint start_index = g_utf8_offset_to_pointer (text, start_char) - text;
      gint end_index = g_utf8_offset_to_pointer (text, end_char) - text;
      gint real_n_ranges, i;

      bango_layout_line_get_x_ranges (line, start_index, end_index, ranges, &real_n_ranges);

      if (ranges)
	{
	  gint *r = *ranges;
	  
	  for (i = 0; i < real_n_ranges; ++i)
	    {
	      r[2 * i + 1] = (r[2 * i + 1] - r[2 * i]) / BANGO_SCALE;
	      r[2 * i] = r[2 * i] / BANGO_SCALE;
	    }
	}
      
      if (n_ranges)
	*n_ranges = real_n_ranges;
    }
  else
    {
      if (n_ranges)
	*n_ranges = 0;
      if (ranges)
	*ranges = NULL;
    }
}

static gboolean
in_selection (BtkEntry *entry,
	      gint	x)
{
  gint *ranges;
  gint n_ranges, i;
  gint retval = FALSE;

  btk_entry_get_pixel_ranges (entry, &ranges, &n_ranges);

  for (i = 0; i < n_ranges; ++i)
    {
      if (x >= ranges[2 * i] && x < ranges[2 * i] + ranges[2 * i + 1])
	{
	  retval = TRUE;
	  break;
	}
    }

  g_free (ranges);
  return retval;
}
	      
static gint
btk_entry_button_press (BtkWidget      *widget,
			BdkEventButton *event)
{
  BtkEntry *entry = BTK_ENTRY (widget);
  BtkEditable *editable = BTK_EDITABLE (widget);
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);
  EntryIconInfo *icon_info = NULL;
  gint tmp_pos;
  gint sel_start, sel_end;
  gint i;

  for (i = 0; i < MAX_ICONS; i++)
    {
      icon_info = priv->icons[i];

      if (!icon_info || icon_info->insensitive)
        continue;

      if (event->window == icon_info->window)
        {
          if (should_prelight (entry, i))
            {
              icon_info->prelight = FALSE;
              btk_widget_queue_draw (widget);
            }

          priv->start_x = event->x;
          priv->start_y = event->y;
          icon_info->pressed = TRUE;

          if (!icon_info->nonactivatable)
            g_signal_emit (entry, signals[ICON_PRESS], 0, i, event);

          return TRUE;
        }
    }

  if (event->window != entry->text_area ||
      (entry->button && event->button != entry->button))
    return FALSE;

  btk_entry_reset_blink_time (entry);

  entry->button = event->button;
  
  if (!btk_widget_has_focus (widget))
    {
      entry->in_click = TRUE;
      btk_widget_grab_focus (widget);
      entry->in_click = FALSE;
    }
  
  tmp_pos = btk_entry_find_position (entry, event->x + entry->scroll_offset);

  if (_btk_button_event_triggers_context_menu (event))
    {
      btk_entry_do_popup (entry, event);
      entry->button = 0;	/* Don't wait for release, since the menu will btk_grab_add */

      return TRUE;
    }
  else if (event->button == 1)
    {
      gboolean have_selection = btk_editable_get_selection_bounds (editable, &sel_start, &sel_end);
      
      entry->select_words = FALSE;
      entry->select_lines = FALSE;

      if (event->state & BTK_EXTEND_SELECTION_MOD_MASK)
	{
	  _btk_entry_reset_im_context (entry);
	  
	  if (!have_selection) /* select from the current position to the clicked position */
	    sel_start = sel_end = entry->current_pos;
	  
	  if (tmp_pos > sel_start && tmp_pos < sel_end)
	    {
	      /* Truncate current selection, but keep it as big as possible */
	      if (tmp_pos - sel_start > sel_end - tmp_pos)
		btk_entry_set_positions (entry, sel_start, tmp_pos);
	      else
		btk_entry_set_positions (entry, tmp_pos, sel_end);
	    }
	  else
	    {
	      gboolean extend_to_left;
	      gint start, end;

	      /* Figure out what click selects and extend current selection */
	      switch (event->type)
		{
		case BDK_BUTTON_PRESS:
		  btk_entry_set_positions (entry, tmp_pos, tmp_pos);
		  break;
		  
		case BDK_2BUTTON_PRESS:
		  entry->select_words = TRUE;
		  btk_entry_select_word (entry);
		  break;
		  
		case BDK_3BUTTON_PRESS:
		  entry->select_lines = TRUE;
		  btk_entry_select_line (entry);
		  break;

		default:
		  break;
		}

	      start = MIN (entry->current_pos, entry->selection_bound);
	      start = MIN (sel_start, start);
	      
	      end = MAX (entry->current_pos, entry->selection_bound);
	      end = MAX (sel_end, end);

	      if (tmp_pos == sel_start || tmp_pos == sel_end)
		extend_to_left = (tmp_pos == start);
	      else
		extend_to_left = (end == sel_end);
	      
	      if (extend_to_left)
		btk_entry_set_positions (entry, start, end);
	      else
		btk_entry_set_positions (entry, end, start);
	    }
	}
      else /* no shift key */
	switch (event->type)
	{
	case BDK_BUTTON_PRESS:
	  if (in_selection (entry, event->x + entry->scroll_offset))
	    {
	      /* Click inside the selection - we'll either start a drag, or
	       * clear the selection
	       */
	      entry->in_drag = TRUE;
	      entry->drag_start_x = event->x + entry->scroll_offset;
	      entry->drag_start_y = event->y;
	    }
	  else
            btk_editable_set_position (editable, tmp_pos);
	  break;
 
	case BDK_2BUTTON_PRESS:
	  /* We ALWAYS receive a BDK_BUTTON_PRESS immediately before 
	   * receiving a BDK_2BUTTON_PRESS so we need to reset
 	   * entry->in_drag which may have been set above
           */
	  entry->in_drag = FALSE;
	  entry->select_words = TRUE;
	  btk_entry_select_word (entry);
	  break;
	
	case BDK_3BUTTON_PRESS:
	  /* We ALWAYS receive a BDK_BUTTON_PRESS immediately before
	   * receiving a BDK_3BUTTON_PRESS so we need to reset
	   * entry->in_drag which may have been set above
	   */
	  entry->in_drag = FALSE;
	  entry->select_lines = TRUE;
	  btk_entry_select_line (entry);
	  break;

	default:
	  break;
	}

      return TRUE;
    }
  else if (event->button == 2 && event->type == BDK_BUTTON_PRESS)
    {
      if (entry->editable)
        {
          priv->insert_pos = tmp_pos;
          btk_entry_paste (entry, BDK_SELECTION_PRIMARY);
          return TRUE;
        }
      else
        {
          btk_widget_error_bell (widget);
        }
    }

  return FALSE;
}

static gint
btk_entry_button_release (BtkWidget      *widget,
			  BdkEventButton *event)
{
  BtkEntry *entry = BTK_ENTRY (widget);
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (widget);
  EntryIconInfo *icon_info = NULL;
  gint i;

  for (i = 0; i < MAX_ICONS; i++)
    {
      icon_info = priv->icons[i];

      if (!icon_info || icon_info->insensitive)
        continue;

      if (event->window == icon_info->window)
        {
          gint width, height;

          width = bdk_window_get_width (icon_info->window);
          height = bdk_window_get_height (icon_info->window);

          icon_info->pressed = FALSE;

          if (should_prelight (entry, i) &&
              event->x >= 0 && event->y >= 0 &&
              event->x < width && event->y < height)
            {
              icon_info->prelight = TRUE;
              btk_widget_queue_draw (widget);
            }

          if (!icon_info->nonactivatable)
            g_signal_emit (entry, signals[ICON_RELEASE], 0, i, event);

          return TRUE;
        }
    }

  if (event->window != entry->text_area || entry->button != event->button)
    return FALSE;

  if (entry->in_drag)
    {
      gint tmp_pos = btk_entry_find_position (entry, entry->drag_start_x);

      btk_editable_set_position (BTK_EDITABLE (entry), tmp_pos);

      entry->in_drag = 0;
    }
  
  entry->button = 0;
  
  btk_entry_update_primary_selection (entry);
	      
  return TRUE;
}

static gchar *
_btk_entry_get_selected_text (BtkEntry *entry)
{
  BtkEditable *editable = BTK_EDITABLE (entry);
  gint         start_text, end_text;
  gchar       *text = NULL;

  if (btk_editable_get_selection_bounds (editable, &start_text, &end_text))
    text = btk_editable_get_chars (editable, start_text, end_text);

  return text;
}

static gint
btk_entry_motion_notify (BtkWidget      *widget,
			 BdkEventMotion *event)
{
  BtkEntry *entry = BTK_ENTRY (widget);
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);
  EntryIconInfo *icon_info = NULL;
  BdkDragContext *context;
  gint tmp_pos;
  gint i;

  for (i = 0; i < MAX_ICONS; i++)
    {
      icon_info = priv->icons[i];

      if (!icon_info || icon_info->insensitive)
        continue;

      if (event->window == icon_info->window)
        {
          if (icon_info->pressed &&
              icon_info->target_list != NULL &&
              btk_drag_check_threshold (widget,
                                        priv->start_x,
                                        priv->start_y,
                                        event->x,
                                        event->y))
            {
              icon_info->in_drag = TRUE;
              icon_info->pressed = FALSE;
              context = btk_drag_begin (widget,
                                        icon_info->target_list,
                                        icon_info->actions,
                                        1,
                                        (BdkEvent*)event);
            }

          return TRUE;
        }
    }

  if (entry->mouse_cursor_obscured)
    {
      BdkCursor *cursor;
      
      cursor = bdk_cursor_new_for_display (btk_widget_get_display (widget), BDK_XTERM);
      bdk_window_set_cursor (entry->text_area, cursor);
      bdk_cursor_unref (cursor);
      entry->mouse_cursor_obscured = FALSE;
    }

  if (event->window != entry->text_area || entry->button != 1)
    return FALSE;

  if (entry->select_lines)
    return TRUE;

  bdk_event_request_motions (event);

  if (entry->in_drag)
    {
      if (btk_entry_get_display_mode (entry) == DISPLAY_NORMAL &&
          btk_drag_check_threshold (widget,
                                    entry->drag_start_x, entry->drag_start_y,
                                    event->x + entry->scroll_offset, event->y))
        {
          BdkDragContext *context;
          BtkTargetList  *target_list = btk_target_list_new (NULL, 0);
          guint actions = entry->editable ? BDK_ACTION_COPY | BDK_ACTION_MOVE : BDK_ACTION_COPY;
          gchar *text = NULL;
          BdkPixmap *pixmap = NULL;

          btk_target_list_add_text_targets (target_list, 0);

          text = _btk_entry_get_selected_text (entry);
          pixmap = _btk_text_util_create_drag_icon (widget, text, -1);

          context = btk_drag_begin (widget, target_list, actions,
                                    entry->button, (BdkEvent *)event);
          
          if (pixmap)
            btk_drag_set_icon_pixmap (context,
                                      bdk_drawable_get_colormap (pixmap),
                                      pixmap,
                                      NULL,
                                      -2, -2);
          else
            btk_drag_set_icon_default (context);
          
          if (pixmap)
            g_object_unref (pixmap);
          g_free (text);

          entry->in_drag = FALSE;
          entry->button = 0;
	  
          btk_target_list_unref (target_list);
        }
    }
  else
    {
      gint height;

      height = bdk_window_get_height (entry->text_area);

      if (event->y < 0)
	tmp_pos = 0;
      else if (event->y >= height)
	tmp_pos = btk_entry_buffer_get_length (get_buffer (entry));
      else
	tmp_pos = btk_entry_find_position (entry, event->x + entry->scroll_offset);
      
      if (entry->select_words) 
	{
	  gint min, max;
	  gint old_min, old_max;
	  gint pos, bound;
	  
	  min = btk_entry_move_backward_word (entry, tmp_pos, TRUE);
	  max = btk_entry_move_forward_word (entry, tmp_pos, TRUE);
	  
	  pos = entry->current_pos;
	  bound = entry->selection_bound;

	  old_min = MIN(entry->current_pos, entry->selection_bound);
	  old_max = MAX(entry->current_pos, entry->selection_bound);
	  
	  if (min < old_min)
	    {
	      pos = min;
	      bound = old_max;
	    }
	  else if (old_max < max) 
	    {
	      pos = max;
	      bound = old_min;
	    }
	  else if (pos == old_min) 
	    {
	      if (entry->current_pos != min)
		pos = max;
	    }
	  else 
	    {
	      if (entry->current_pos != max)
		pos = min;
	    }
	
	  btk_entry_set_positions (entry, pos, bound);
	}
      else
        btk_entry_set_positions (entry, tmp_pos, -1);
    }
      
  return TRUE;
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
btk_entry_obscure_mouse_cursor (BtkEntry *entry)
{
  if (entry->mouse_cursor_obscured)
    return;

  set_invisible_cursor (entry->text_area);
  
  entry->mouse_cursor_obscured = TRUE;  
}

static gint
btk_entry_key_press (BtkWidget   *widget,
		     BdkEventKey *event)
{
  BtkEntry *entry = BTK_ENTRY (widget);

  btk_entry_reset_blink_time (entry);
  btk_entry_pend_cursor_blink (entry);

  if (entry->editable)
    {
      if (btk_im_context_filter_keypress (entry->im_context, event))
	{
	  btk_entry_obscure_mouse_cursor (entry);
	  entry->need_im_reset = TRUE;
	  return TRUE;
	}
    }

  if (event->keyval == BDK_Return || 
      event->keyval == BDK_KP_Enter || 
      event->keyval == BDK_ISO_Enter || 
      event->keyval == BDK_Escape)
    {
      BtkEntryCompletion *completion = btk_entry_get_completion (entry);
      
      if (completion && completion->priv->completion_timeout)
        {
          g_source_remove (completion->priv->completion_timeout);
          completion->priv->completion_timeout = 0;
        }

      _btk_entry_reset_im_context (entry);
    }

  if (BTK_WIDGET_CLASS (btk_entry_parent_class)->key_press_event (widget, event))
    /* Activate key bindings
     */
    return TRUE;

  if (!entry->editable && event->length)
    btk_widget_error_bell (widget);

  return FALSE;
}

static gint
btk_entry_key_release (BtkWidget   *widget,
		       BdkEventKey *event)
{
  BtkEntry *entry = BTK_ENTRY (widget);

  if (entry->editable)
    {
      if (btk_im_context_filter_keypress (entry->im_context, event))
	{
	  entry->need_im_reset = TRUE;
	  return TRUE;
	}
    }

  return BTK_WIDGET_CLASS (btk_entry_parent_class)->key_release_event (widget, event);
}

static gint
btk_entry_focus_in (BtkWidget     *widget,
		    BdkEventFocus *event)
{
  BtkEntry *entry = BTK_ENTRY (widget);
  BdkKeymap *keymap;

  btk_widget_queue_draw (widget);

  keymap = bdk_keymap_get_for_display (btk_widget_get_display (widget));

  if (entry->editable)
    {
      entry->need_im_reset = TRUE;
      btk_im_context_focus_in (entry->im_context);
      keymap_state_changed (keymap, entry);
      g_signal_connect (keymap, "state-changed", 
                        G_CALLBACK (keymap_state_changed), entry);
    }

  g_signal_connect (keymap, "direction-changed",
		    G_CALLBACK (keymap_direction_changed), entry);

  btk_entry_reset_blink_time (entry);
  btk_entry_check_cursor_blink (entry);

  return FALSE;
}

static gint
btk_entry_focus_out (BtkWidget     *widget,
		     BdkEventFocus *event)
{
  BtkEntry *entry = BTK_ENTRY (widget);
  BtkEntryCompletion *completion;
  BdkKeymap *keymap;

  btk_widget_queue_draw (widget);

  keymap = bdk_keymap_get_for_display (btk_widget_get_display (widget));

  if (entry->editable)
    {
      entry->need_im_reset = TRUE;
      btk_im_context_focus_out (entry->im_context);
      remove_capslock_feedback (entry);
    }

  btk_entry_check_cursor_blink (entry);
  
  g_signal_handlers_disconnect_by_func (keymap, keymap_state_changed, entry);
  g_signal_handlers_disconnect_by_func (keymap, keymap_direction_changed, entry);

  completion = btk_entry_get_completion (entry);
  if (completion)
    _btk_entry_completion_popdown (completion);

  return FALSE;
}

static void
btk_entry_grab_focus (BtkWidget *widget)
{
  BtkEntry *entry = BTK_ENTRY (widget);
  gboolean select_on_focus;

  BTK_WIDGET_CLASS (btk_entry_parent_class)->grab_focus (widget);

  if (entry->editable && !entry->in_click)
    {
      g_object_get (btk_widget_get_settings (widget),
                    "btk-entry-select-on-focus",
                    &select_on_focus,
                    NULL);

      if (select_on_focus)
        btk_editable_select_rebunnyion (BTK_EDITABLE (widget), 0, -1);
    }
}

static void
btk_entry_direction_changed (BtkWidget        *widget,
                             BtkTextDirection  previous_dir)
{
  BtkEntry *entry = BTK_ENTRY (widget);

  btk_entry_recompute (entry);

  BTK_WIDGET_CLASS (btk_entry_parent_class)->direction_changed (widget, previous_dir);
}

static void
btk_entry_state_changed (BtkWidget      *widget,
			 BtkStateType    previous_state)
{
  BtkEntry *entry = BTK_ENTRY (widget);
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (widget);
  BdkCursor *cursor;
  gint i;

  if (btk_widget_get_realized (widget))
    {
      bdk_window_set_background (widget->window, &widget->style->base[btk_widget_get_state (widget)]);
      bdk_window_set_background (entry->text_area, &widget->style->base[btk_widget_get_state (widget)]);
      for (i = 0; i < MAX_ICONS; i++)
        {
          EntryIconInfo *icon_info = priv->icons[i];
          if (icon_info && icon_info->window)
            bdk_window_set_background (icon_info->window, &widget->style->base[btk_widget_get_state (widget)]);
        }

      if (btk_widget_is_sensitive (widget))
        cursor = bdk_cursor_new_for_display (btk_widget_get_display (widget), BDK_XTERM);
      else
        cursor = NULL;

      bdk_window_set_cursor (entry->text_area, cursor);

      if (cursor)
        bdk_cursor_unref (cursor);

      entry->mouse_cursor_obscured = FALSE;

      update_cursors (widget);
    }

  if (!btk_widget_is_sensitive (widget))
    {
      /* Clear any selection */
      btk_editable_select_rebunnyion (BTK_EDITABLE (entry), entry->current_pos, entry->current_pos);
    }

  btk_widget_queue_draw (widget);
}

static void
btk_entry_screen_changed (BtkWidget *widget,
			  BdkScreen *old_screen)
{
  btk_entry_recompute (BTK_ENTRY (widget));
}

/* BtkEditable method implementations
 */
static void
btk_entry_insert_text (BtkEditable *editable,
		       const gchar *new_text,
		       gint         new_text_length,
		       gint        *position)
{
  g_object_ref (editable);

  /*
   * The incoming text may a password or other secret. We make sure
   * not to copy it into temporary buffers.
   */

  g_signal_emit_by_name (editable, "insert-text", new_text, new_text_length, position);

  g_object_unref (editable);
}

static void
btk_entry_delete_text (BtkEditable *editable,
		       gint         start_pos,
		       gint         end_pos)
{
  g_object_ref (editable);

  g_signal_emit_by_name (editable, "delete-text", start_pos, end_pos);

  g_object_unref (editable);
}

static gchar *    
btk_entry_get_chars      (BtkEditable   *editable,
			  gint           start_pos,
			  gint           end_pos)
{
  BtkEntry *entry = BTK_ENTRY (editable);
  const gchar *text;
  gint text_length;
  gint start_index, end_index;

  text = btk_entry_buffer_get_text (get_buffer (entry));
  text_length = btk_entry_buffer_get_length (get_buffer (entry));

  if (end_pos < 0)
    end_pos = text_length;

  start_pos = MIN (text_length, start_pos);
  end_pos = MIN (text_length, end_pos);

  start_index = g_utf8_offset_to_pointer (text, start_pos) - entry->text;
  end_index = g_utf8_offset_to_pointer (text, end_pos) - entry->text;

  return g_strndup (text + start_index, end_index - start_index);
}

static void
btk_entry_real_set_position (BtkEditable *editable,
			     gint         position)
{
  BtkEntry *entry = BTK_ENTRY (editable);

  guint length;

  length = btk_entry_buffer_get_length (get_buffer (entry));
  if (position < 0 || position > length)
    position = length;

  if (position != entry->current_pos ||
      position != entry->selection_bound)
    {
      _btk_entry_reset_im_context (entry);
      btk_entry_set_positions (entry, position, position);
    }
}

static gint
btk_entry_get_position (BtkEditable *editable)
{
  return BTK_ENTRY (editable)->current_pos;
}

static void
btk_entry_set_selection_bounds (BtkEditable *editable,
				gint         start,
				gint         end)
{
  BtkEntry *entry = BTK_ENTRY (editable);
  guint length;

  length = btk_entry_buffer_get_length (get_buffer (entry));
  if (start < 0)
    start = length;
  if (end < 0)
    end = length;
  
  _btk_entry_reset_im_context (entry);

  btk_entry_set_positions (entry,
			   MIN (end, length),
			   MIN (start, length));

  btk_entry_update_primary_selection (entry);
}

static gboolean
btk_entry_get_selection_bounds (BtkEditable *editable,
				gint        *start,
				gint        *end)
{
  BtkEntry *entry = BTK_ENTRY (editable);

  *start = entry->selection_bound;
  *end = entry->current_pos;

  return (entry->selection_bound != entry->current_pos);
}

static void
icon_theme_changed (BtkEntry *entry)
{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);
  gint i;

  for (i = 0; i < MAX_ICONS; i++)
    {
      EntryIconInfo *icon_info = priv->icons[i];
      if (icon_info != NULL) 
        {
          if (icon_info->storage_type == BTK_IMAGE_ICON_NAME)
            btk_entry_set_icon_from_icon_name (entry, i, icon_info->icon_name);
          else if (icon_info->storage_type == BTK_IMAGE_STOCK)
            btk_entry_set_icon_from_stock (entry, i, icon_info->stock_id);
          else if (icon_info->storage_type == BTK_IMAGE_GICON)
            btk_entry_set_icon_from_gicon (entry, i, icon_info->gicon);
        }
    }

  btk_widget_queue_draw (BTK_WIDGET (entry));
}

static void
icon_margin_changed (BtkEntry *entry)
{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);
  BtkBorder border;

  _btk_entry_effective_inner_border (BTK_ENTRY (entry), &border);

  priv->icon_margin = border.left;
}

static void 
btk_entry_style_set (BtkWidget *widget,
                     BtkStyle  *previous_style)
{
  BtkEntry *entry = BTK_ENTRY (widget);
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);
  gint focus_width;
  gboolean interior_focus;
  gint i;

  btk_widget_style_get (widget,
			"focus-line-width", &focus_width,
			"interior-focus", &interior_focus,
			NULL);

  priv->focus_width = focus_width;
  priv->interior_focus = interior_focus;

  if (!priv->invisible_char_set)
    entry->invisible_char = find_invisible_char (BTK_WIDGET (entry));

  btk_entry_recompute (entry);

  if (previous_style && btk_widget_get_realized (widget))
    {
      bdk_window_set_background (widget->window, &widget->style->base[btk_widget_get_state (widget)]);
      bdk_window_set_background (entry->text_area, &widget->style->base[btk_widget_get_state (widget)]);
      for (i = 0; i < MAX_ICONS; i++) 
        {
          EntryIconInfo *icon_info = priv->icons[i];
          if (icon_info && icon_info->window)
            bdk_window_set_background (icon_info->window, &widget->style->base[btk_widget_get_state (widget)]);
        }
    }

  icon_theme_changed (entry);
  icon_margin_changed (entry);
}

/* BtkCellEditable method implementations
 */
static void
btk_cell_editable_entry_activated (BtkEntry *entry, gpointer data)
{
  btk_cell_editable_editing_done (BTK_CELL_EDITABLE (entry));
  btk_cell_editable_remove_widget (BTK_CELL_EDITABLE (entry));
}

static gboolean
btk_cell_editable_key_press_event (BtkEntry    *entry,
				   BdkEventKey *key_event,
				   gpointer     data)
{
  if (key_event->keyval == BDK_Escape)
    {
      entry->editing_canceled = TRUE;
      btk_cell_editable_editing_done (BTK_CELL_EDITABLE (entry));
      btk_cell_editable_remove_widget (BTK_CELL_EDITABLE (entry));

      return TRUE;
    }

  /* override focus */
  if (key_event->keyval == BDK_Up || key_event->keyval == BDK_Down)
    {
      btk_cell_editable_editing_done (BTK_CELL_EDITABLE (entry));
      btk_cell_editable_remove_widget (BTK_CELL_EDITABLE (entry));

      return TRUE;
    }

  return FALSE;
}

static void
btk_entry_start_editing (BtkCellEditable *cell_editable,
			 BdkEvent        *event)
{
  BTK_ENTRY (cell_editable)->is_cell_renderer = TRUE;

  g_signal_connect (cell_editable, "activate",
		    G_CALLBACK (btk_cell_editable_entry_activated), NULL);
  g_signal_connect (cell_editable, "key-press-event",
		    G_CALLBACK (btk_cell_editable_key_press_event), NULL);
}

static void
btk_entry_password_hint_free (BtkEntryPasswordHint *password_hint)
{
  if (password_hint->source_id)
    g_source_remove (password_hint->source_id);

  g_slice_free (BtkEntryPasswordHint, password_hint);
}


static gboolean
btk_entry_remove_password_hint (gpointer data)
{
  BtkEntryPasswordHint *password_hint = g_object_get_qdata (data, quark_password_hint);
  password_hint->position = -1;

  /* Force the string to be redrawn, but now without a visible character */
  btk_entry_recompute (BTK_ENTRY (data));
  return FALSE;
}

/* Default signal handlers
 */
static void
btk_entry_real_insert_text (BtkEditable *editable,
			    const gchar *new_text,
			    gint         new_text_length,
			    gint        *position)
{
  guint n_inserted;
  gint n_chars;

  n_chars = g_utf8_strlen (new_text, new_text_length);

  /*
   * The actual insertion into the buffer. This will end up firing the
   * following signal handlers: buffer_inserted_text(), buffer_notify_display_text(),
   * buffer_notify_text(), buffer_notify_length()
   */
  begin_change (BTK_ENTRY (editable));

  n_inserted = btk_entry_buffer_insert_text (get_buffer (BTK_ENTRY (editable)), *position, new_text, n_chars);

  end_change (BTK_ENTRY (editable));

  if (n_inserted != n_chars)
      btk_widget_error_bell (BTK_WIDGET (editable));

  *position += n_inserted;
}

static void
btk_entry_real_delete_text (BtkEditable *editable,
                            gint         start_pos,
                            gint         end_pos)
{
  /*
   * The actual deletion from the buffer. This will end up firing the
   * following signal handlers: buffer_deleted_text(), buffer_notify_display_text(),
   * buffer_notify_text(), buffer_notify_length()
   */

  begin_change (BTK_ENTRY (editable));

  btk_entry_buffer_delete_text (get_buffer (BTK_ENTRY (editable)), start_pos, end_pos - start_pos);

  end_change (BTK_ENTRY (editable));
}

/* BtkEntryBuffer signal handlers
 */
static void
buffer_inserted_text (BtkEntryBuffer *buffer,
                      guint           position,
                      const gchar    *chars,
                      guint           n_chars,
                      BtkEntry       *entry)
{
  guint password_hint_timeout;
  guint current_pos;
  gint selection_bound;

  current_pos = entry->current_pos;
  if (current_pos > position)
    current_pos += n_chars;

  selection_bound = entry->selection_bound;
  if (selection_bound > position)
    selection_bound += n_chars;

  btk_entry_set_positions (entry, current_pos, selection_bound);

  /* Calculate the password hint if it needs to be displayed. */
  if (n_chars == 1 && !entry->visible)
    {
      g_object_get (btk_widget_get_settings (BTK_WIDGET (entry)),
                    "btk-entry-password-hint-timeout", &password_hint_timeout,
                    NULL);

      if (password_hint_timeout > 0)
        {
          BtkEntryPasswordHint *password_hint = g_object_get_qdata (G_OBJECT (entry),
                                                                    quark_password_hint);
          if (!password_hint)
            {
              password_hint = g_slice_new0 (BtkEntryPasswordHint);
              g_object_set_qdata_full (G_OBJECT (entry), quark_password_hint, password_hint,
                                       (GDestroyNotify)btk_entry_password_hint_free);
            }

          password_hint->position = position;
          if (password_hint->source_id)
            g_source_remove (password_hint->source_id);
          password_hint->source_id = bdk_threads_add_timeout (password_hint_timeout,
                                                              (GSourceFunc)btk_entry_remove_password_hint, entry);
        }
    }
}

static void
buffer_deleted_text (BtkEntryBuffer *buffer,
                     guint           position,
                     guint           n_chars,
                     BtkEntry       *entry)
{
  guint end_pos = position + n_chars;
  gint selection_bound;
  guint current_pos;

  current_pos = entry->current_pos;
  if (current_pos > position)
    current_pos -= MIN (current_pos, end_pos) - position;

  selection_bound = entry->selection_bound;
  if (selection_bound > position)
    selection_bound -= MIN (selection_bound, end_pos) - position;

  btk_entry_set_positions (entry, current_pos, selection_bound);

  /* We might have deleted the selection */
  btk_entry_update_primary_selection (entry);

  /* Disable the password hint if one exists. */
  if (!entry->visible)
    {
      BtkEntryPasswordHint *password_hint = g_object_get_qdata (G_OBJECT (entry),
                                                                quark_password_hint);
      if (password_hint)
        {
          if (password_hint->source_id)
            g_source_remove (password_hint->source_id);
          password_hint->source_id = 0;
          password_hint->position = -1;
        }
    }
}

static void
buffer_notify_text (BtkEntryBuffer *buffer,
                    GParamSpec     *spec,
                    BtkEntry       *entry)
{
  /* COMPAT: Deprecated, not used. This struct field will be removed in BTK+ 3.x */
  entry->text = (gchar*)btk_entry_buffer_get_text (buffer);

  btk_entry_recompute (entry);
  emit_changed (entry);
  g_object_notify (G_OBJECT (entry), "text");
}

static void
buffer_notify_length (BtkEntryBuffer *buffer,
                      GParamSpec     *spec,
                      BtkEntry       *entry)
{
  /* COMPAT: Deprecated, not used. This struct field will be removed in BTK+ 3.x */
  entry->text_length = btk_entry_buffer_get_length (buffer);

  g_object_notify (G_OBJECT (entry), "text-length");
}

static void
buffer_notify_max_length (BtkEntryBuffer *buffer,
                          GParamSpec     *spec,
                          BtkEntry       *entry)
{
  /* COMPAT: Deprecated, not used. This struct field will be removed in BTK+ 3.x */
  entry->text_max_length = btk_entry_buffer_get_max_length (buffer);

  g_object_notify (G_OBJECT (entry), "max-length");
}

static void
buffer_connect_signals (BtkEntry *entry)
{
  g_signal_connect (get_buffer (entry), "inserted-text", G_CALLBACK (buffer_inserted_text), entry);
  g_signal_connect (get_buffer (entry), "deleted-text", G_CALLBACK (buffer_deleted_text), entry);
  g_signal_connect (get_buffer (entry), "notify::text", G_CALLBACK (buffer_notify_text), entry);
  g_signal_connect (get_buffer (entry), "notify::length", G_CALLBACK (buffer_notify_length), entry);
  g_signal_connect (get_buffer (entry), "notify::max-length", G_CALLBACK (buffer_notify_max_length), entry);
}

static void
buffer_disconnect_signals (BtkEntry *entry)
{
  g_signal_handlers_disconnect_by_func (get_buffer (entry), buffer_inserted_text, entry);
  g_signal_handlers_disconnect_by_func (get_buffer (entry), buffer_deleted_text, entry);
  g_signal_handlers_disconnect_by_func (get_buffer (entry), buffer_notify_text, entry);
  g_signal_handlers_disconnect_by_func (get_buffer (entry), buffer_notify_length, entry);
  g_signal_handlers_disconnect_by_func (get_buffer (entry), buffer_notify_max_length, entry);
}

/* Compute the X position for an offset that corresponds to the "more important
 * cursor position for that offset. We use this when trying to guess to which
 * end of the selection we should go to when the user hits the left or
 * right arrow key.
 */
static gint
get_better_cursor_x (BtkEntry *entry,
		     gint      offset)
{
  BdkKeymap *keymap = bdk_keymap_get_for_display (btk_widget_get_display (BTK_WIDGET (entry)));
  BangoDirection keymap_direction = bdk_keymap_get_direction (keymap);
  gboolean split_cursor;
  
  BangoLayout *layout = btk_entry_ensure_layout (entry, TRUE);
  const gchar *text = bango_layout_get_text (layout);
  gint index = g_utf8_offset_to_pointer (text, offset) - text;
  
  BangoRectangle strong_pos, weak_pos;
  
  g_object_get (btk_widget_get_settings (BTK_WIDGET (entry)),
		"btk-split-cursor", &split_cursor,
		NULL);

  bango_layout_get_cursor_pos (layout, index, &strong_pos, &weak_pos);

  if (split_cursor)
    return strong_pos.x / BANGO_SCALE;
  else
    return (keymap_direction == entry->resolved_dir) ? strong_pos.x / BANGO_SCALE : weak_pos.x / BANGO_SCALE;
}

static void
btk_entry_move_cursor (BtkEntry       *entry,
		       BtkMovementStep step,
		       gint            count,
		       gboolean        extend_selection)
{
  gint new_pos = entry->current_pos;
  BtkEntryPrivate *priv;

  _btk_entry_reset_im_context (entry);

  if (entry->current_pos != entry->selection_bound && !extend_selection)
    {
      /* If we have a current selection and aren't extending it, move to the
       * start/or end of the selection as appropriate
       */
      switch (step)
	{
	case BTK_MOVEMENT_VISUAL_POSITIONS:
	  {
	    gint current_x = get_better_cursor_x (entry, entry->current_pos);
	    gint bound_x = get_better_cursor_x (entry, entry->selection_bound);

	    if (count <= 0)
	      new_pos = current_x < bound_x ? entry->current_pos : entry->selection_bound;
	    else 
	      new_pos = current_x > bound_x ? entry->current_pos : entry->selection_bound;
	  }
	  break;

	case BTK_MOVEMENT_WORDS:
          if (entry->resolved_dir == BANGO_DIRECTION_RTL)
            count *= -1;
          /* Fall through */

	case BTK_MOVEMENT_LOGICAL_POSITIONS:
	  if (count < 0)
	    new_pos = MIN (entry->current_pos, entry->selection_bound);
	  else
	    new_pos = MAX (entry->current_pos, entry->selection_bound);

	  break;

	case BTK_MOVEMENT_DISPLAY_LINE_ENDS:
	case BTK_MOVEMENT_PARAGRAPH_ENDS:
	case BTK_MOVEMENT_BUFFER_ENDS:
	  priv = BTK_ENTRY_GET_PRIVATE (entry);
	  new_pos = count < 0 ? 0 : btk_entry_buffer_get_length (get_buffer (entry));
	  break;

	case BTK_MOVEMENT_DISPLAY_LINES:
	case BTK_MOVEMENT_PARAGRAPHS:
	case BTK_MOVEMENT_PAGES:
	case BTK_MOVEMENT_HORIZONTAL_PAGES:
	  break;
	}
    }
  else
    {
      switch (step)
	{
	case BTK_MOVEMENT_LOGICAL_POSITIONS:
	  new_pos = btk_entry_move_logically (entry, new_pos, count);
	  break;

	case BTK_MOVEMENT_VISUAL_POSITIONS:
	  new_pos = btk_entry_move_visually (entry, new_pos, count);

          if (entry->current_pos == new_pos)
            {
              if (!extend_selection)
                {
                  if (!btk_widget_keynav_failed (BTK_WIDGET (entry),
                                                 count > 0 ?
                                                 BTK_DIR_RIGHT : BTK_DIR_LEFT))
                    {
                      BtkWidget *toplevel = btk_widget_get_toplevel (BTK_WIDGET (entry));

                      if (toplevel)
                        btk_widget_child_focus (toplevel,
                                                count > 0 ?
                                                BTK_DIR_RIGHT : BTK_DIR_LEFT);
                    }
                }
              else
                {
                  btk_widget_error_bell (BTK_WIDGET (entry));
                }
            }
	  break;

	case BTK_MOVEMENT_WORDS:
          if (entry->resolved_dir == BANGO_DIRECTION_RTL)
            count *= -1;

	  while (count > 0)
	    {
	      new_pos = btk_entry_move_forward_word (entry, new_pos, FALSE);
	      count--;
	    }

	  while (count < 0)
	    {
	      new_pos = btk_entry_move_backward_word (entry, new_pos, FALSE);
	      count++;
	    }
          if (entry->current_pos == new_pos)
            btk_widget_error_bell (BTK_WIDGET (entry));

	  break;

	case BTK_MOVEMENT_DISPLAY_LINE_ENDS:
	case BTK_MOVEMENT_PARAGRAPH_ENDS:
	case BTK_MOVEMENT_BUFFER_ENDS:
	  priv = BTK_ENTRY_GET_PRIVATE (entry);
	  new_pos = count < 0 ? 0 : btk_entry_buffer_get_length (get_buffer (entry));
          if (entry->current_pos == new_pos)
            btk_widget_error_bell (BTK_WIDGET (entry));

	  break;

	case BTK_MOVEMENT_DISPLAY_LINES:
	case BTK_MOVEMENT_PARAGRAPHS:
	case BTK_MOVEMENT_PAGES:
	case BTK_MOVEMENT_HORIZONTAL_PAGES:
	  break;
	}
    }

  if (extend_selection)
    btk_editable_select_rebunnyion (BTK_EDITABLE (entry), entry->selection_bound, new_pos);
  else
    btk_editable_set_position (BTK_EDITABLE (entry), new_pos);
  
  btk_entry_pend_cursor_blink (entry);
}

static void
btk_entry_insert_at_cursor (BtkEntry    *entry,
			    const gchar *str)
{
  BtkEditable *editable = BTK_EDITABLE (entry);
  gint pos = entry->current_pos;

  if (entry->editable)
    {
      _btk_entry_reset_im_context (entry);

      btk_editable_insert_text (editable, str, -1, &pos);
      btk_editable_set_position (editable, pos);
    }
}

static void
btk_entry_delete_from_cursor (BtkEntry       *entry,
			      BtkDeleteType   type,
			      gint            count)
{
  BtkEditable *editable = BTK_EDITABLE (entry);
  gint start_pos = entry->current_pos;
  gint end_pos = entry->current_pos;
  gint old_n_bytes = btk_entry_buffer_get_bytes (get_buffer (entry));
  
  _btk_entry_reset_im_context (entry);

  if (!entry->editable)
    {
      btk_widget_error_bell (BTK_WIDGET (entry));
      return;
    }

  if (entry->selection_bound != entry->current_pos)
    {
      btk_editable_delete_selection (editable);
      return;
    }
  
  switch (type)
    {
    case BTK_DELETE_CHARS:
      end_pos = btk_entry_move_logically (entry, entry->current_pos, count);
      btk_editable_delete_text (editable, MIN (start_pos, end_pos), MAX (start_pos, end_pos));
      break;

    case BTK_DELETE_WORDS:
      if (count < 0)
	{
	  /* Move to end of current word, or if not on a word, end of previous word */
	  end_pos = btk_entry_move_backward_word (entry, end_pos, FALSE);
	  end_pos = btk_entry_move_forward_word (entry, end_pos, FALSE);
	}
      else if (count > 0)
	{
	  /* Move to beginning of current word, or if not on a word, begining of next word */
	  start_pos = btk_entry_move_forward_word (entry, start_pos, FALSE);
	  start_pos = btk_entry_move_backward_word (entry, start_pos, FALSE);
	}
	
      /* Fall through */
    case BTK_DELETE_WORD_ENDS:
      while (count < 0)
	{
	  start_pos = btk_entry_move_backward_word (entry, start_pos, FALSE);
	  count++;
	}

      while (count > 0)
	{
	  end_pos = btk_entry_move_forward_word (entry, end_pos, FALSE);
	  count--;
	}

      btk_editable_delete_text (editable, start_pos, end_pos);
      break;

    case BTK_DELETE_DISPLAY_LINE_ENDS:
    case BTK_DELETE_PARAGRAPH_ENDS:
      if (count < 0)
	btk_editable_delete_text (editable, 0, entry->current_pos);
      else
	btk_editable_delete_text (editable, entry->current_pos, -1);

      break;

    case BTK_DELETE_DISPLAY_LINES:
    case BTK_DELETE_PARAGRAPHS:
      btk_editable_delete_text (editable, 0, -1);  
      break;

    case BTK_DELETE_WHITESPACE:
      btk_entry_delete_whitespace (entry);
      break;
    }

  if (btk_entry_buffer_get_bytes (get_buffer (entry)) == old_n_bytes)
    btk_widget_error_bell (BTK_WIDGET (entry));

  btk_entry_pend_cursor_blink (entry);
}

static void
btk_entry_backspace (BtkEntry *entry)
{
  BtkEditable *editable = BTK_EDITABLE (entry);
  gint prev_pos;

  _btk_entry_reset_im_context (entry);

  if (!entry->editable)
    {
      btk_widget_error_bell (BTK_WIDGET (entry));
      return;
    }

  if (entry->selection_bound != entry->current_pos)
    {
      btk_editable_delete_selection (editable);
      return;
    }

  prev_pos = btk_entry_move_logically (entry, entry->current_pos, -1);

  if (prev_pos < entry->current_pos)
    {
      BangoLayout *layout = btk_entry_ensure_layout (entry, FALSE);
      BangoLogAttr *log_attrs;
      gint n_attrs;

      bango_layout_get_log_attrs (layout, &log_attrs, &n_attrs);

      /* Deleting parts of characters */
      if (log_attrs[entry->current_pos].backspace_deletes_character)
	{
	  gchar *cluster_text;
	  gchar *normalized_text;
          glong  len;

	  cluster_text = btk_entry_get_display_text (entry, prev_pos,
	                                             entry->current_pos);
	  normalized_text = g_utf8_normalize (cluster_text,
			  		      strlen (cluster_text),
					      G_NORMALIZE_NFD);
          len = g_utf8_strlen (normalized_text, -1);

          btk_editable_delete_text (editable, prev_pos, entry->current_pos);
	  if (len > 1)
	    {
	      gint pos = entry->current_pos;

	      btk_editable_insert_text (editable, normalized_text,
					g_utf8_offset_to_pointer (normalized_text, len - 1) - normalized_text,
					&pos);
	      btk_editable_set_position (editable, pos);
	    }

	  g_free (normalized_text);
	  g_free (cluster_text);
	}
      else
	{
          btk_editable_delete_text (editable, prev_pos, entry->current_pos);
	}
      
      g_free (log_attrs);
    }
  else
    {
      btk_widget_error_bell (BTK_WIDGET (entry));
    }

  btk_entry_pend_cursor_blink (entry);
}

static void
btk_entry_copy_clipboard (BtkEntry *entry)
{
  BtkEditable *editable = BTK_EDITABLE (entry);
  gint start, end;
  gchar *str;

  if (btk_editable_get_selection_bounds (editable, &start, &end))
    {
      if (!entry->visible)
        {
          btk_widget_error_bell (BTK_WIDGET (entry));
          return;
        }

      str = btk_entry_get_display_text (entry, start, end);
      btk_clipboard_set_text (btk_widget_get_clipboard (BTK_WIDGET (entry),
							BDK_SELECTION_CLIPBOARD),
			      str, -1);
      g_free (str);
    }
}

static void
btk_entry_cut_clipboard (BtkEntry *entry)
{
  BtkEditable *editable = BTK_EDITABLE (entry);
  gint start, end;

  if (!entry->visible)
    {
      btk_widget_error_bell (BTK_WIDGET (entry));
      return;
    }

  btk_entry_copy_clipboard (entry);

  if (entry->editable)
    {
      if (btk_editable_get_selection_bounds (editable, &start, &end))
	btk_editable_delete_text (editable, start, end);
    }
  else
    {
      btk_widget_error_bell (BTK_WIDGET (entry));
    }
}

static void
btk_entry_paste_clipboard (BtkEntry *entry)
{
  if (entry->editable)
    btk_entry_paste (entry, BDK_SELECTION_CLIPBOARD);
  else
    btk_widget_error_bell (BTK_WIDGET (entry));
}

static void
btk_entry_delete_cb (BtkEntry *entry)
{
  BtkEditable *editable = BTK_EDITABLE (entry);
  gint start, end;

  if (entry->editable)
    {
      if (btk_editable_get_selection_bounds (editable, &start, &end))
	btk_editable_delete_text (editable, start, end);
    }
}

static void
btk_entry_toggle_overwrite (BtkEntry *entry)
{
  entry->overwrite_mode = !entry->overwrite_mode;
  btk_entry_pend_cursor_blink (entry);
  btk_widget_queue_draw (BTK_WIDGET (entry));
}

static void
btk_entry_select_all (BtkEntry *entry)
{
  btk_entry_select_line (entry);
}

static void
btk_entry_real_activate (BtkEntry *entry)
{
  BtkWindow *window;
  BtkWidget *toplevel;
  BtkWidget *widget;

  widget = BTK_WIDGET (entry);

  if (entry->activates_default)
    {
      toplevel = btk_widget_get_toplevel (widget);
      if (BTK_IS_WINDOW (toplevel))
	{
	  window = BTK_WINDOW (toplevel);
      
	  if (window &&
	      widget != window->default_widget &&
	      !(widget == window->focus_widget &&
		(!window->default_widget || !btk_widget_get_sensitive (window->default_widget))))
	    btk_window_activate_default (window);
	}
    }
}

static void
keymap_direction_changed (BdkKeymap *keymap,
                          BtkEntry  *entry)
{
  btk_entry_recompute (entry);
}

/* IM Context Callbacks
 */

static void
btk_entry_commit_cb (BtkIMContext *context,
		     const gchar  *str,
		     BtkEntry     *entry)
{
  if (entry->editable)
    btk_entry_enter_text (entry, str);
}

static void 
btk_entry_preedit_changed_cb (BtkIMContext *context,
			      BtkEntry     *entry)
{
  if (entry->editable)
    {
      gchar *preedit_string;
      gint cursor_pos;
      
      btk_im_context_get_preedit_string (entry->im_context,
                                         &preedit_string, NULL,
                                         &cursor_pos);
      g_signal_emit (entry, signals[PREEDIT_CHANGED], 0, preedit_string);
      entry->preedit_length = strlen (preedit_string);
      cursor_pos = CLAMP (cursor_pos, 0, g_utf8_strlen (preedit_string, -1));
      entry->preedit_cursor = cursor_pos;
      g_free (preedit_string);
    
      btk_entry_recompute (entry);
    }
}

static gboolean
btk_entry_retrieve_surrounding_cb (BtkIMContext *context,
			       BtkEntry         *entry)
{
  gchar *text;

  /* XXXX ??? does this even make sense when text is not visible? Should we return FALSE? */
  text = btk_entry_get_display_text (entry, 0, -1);
  btk_im_context_set_surrounding (context, text, strlen (text), /* Length in bytes */
				  g_utf8_offset_to_pointer (text, entry->current_pos) - text);
  g_free (text);

  return TRUE;
}

static gboolean
btk_entry_delete_surrounding_cb (BtkIMContext *slave,
				 gint          offset,
				 gint          n_chars,
				 BtkEntry     *entry)
{
  if (entry->editable)
    btk_editable_delete_text (BTK_EDITABLE (entry),
                              entry->current_pos + offset,
                              entry->current_pos + offset + n_chars);

  return TRUE;
}

/* Internal functions
 */

/* Used for im_commit_cb and inserting Unicode chars */
static void
btk_entry_enter_text (BtkEntry       *entry,
                      const gchar    *str)
{
  BtkEditable *editable = BTK_EDITABLE (entry);
  gint tmp_pos;
  gboolean old_need_im_reset;
  guint text_length;

  old_need_im_reset = entry->need_im_reset;
  entry->need_im_reset = FALSE;

  if (btk_editable_get_selection_bounds (editable, NULL, NULL))
    btk_editable_delete_selection (editable);
  else
    {
      if (entry->overwrite_mode)
        {
          text_length = btk_entry_buffer_get_length (get_buffer (entry));
          if (entry->current_pos < text_length)
            btk_entry_delete_from_cursor (entry, BTK_DELETE_CHARS, 1);
        }
    }

  tmp_pos = entry->current_pos;
  btk_editable_insert_text (editable, str, strlen (str), &tmp_pos);
  btk_editable_set_position (editable, tmp_pos);

  entry->need_im_reset = old_need_im_reset;
}

/* All changes to entry->current_pos and entry->selection_bound
 * should go through this function.
 */
static void
btk_entry_set_positions (BtkEntry *entry,
			 gint      current_pos,
			 gint      selection_bound)
{
  gboolean changed = FALSE;

  g_object_freeze_notify (G_OBJECT (entry));
  
  if (current_pos != -1 &&
      entry->current_pos != current_pos)
    {
      entry->current_pos = current_pos;
      changed = TRUE;

      g_object_notify (G_OBJECT (entry), "cursor-position");
    }

  if (selection_bound != -1 &&
      entry->selection_bound != selection_bound)
    {
      entry->selection_bound = selection_bound;
      changed = TRUE;
      
      g_object_notify (G_OBJECT (entry), "selection-bound");
    }

  g_object_thaw_notify (G_OBJECT (entry));

  if (changed) 
    {
      btk_entry_move_adjustments (entry);
      btk_entry_recompute (entry);
    }
}

static void
btk_entry_reset_layout (BtkEntry *entry)
{
  if (entry->cached_layout)
    {
      g_object_unref (entry->cached_layout);
      entry->cached_layout = NULL;
    }
}

static void
update_im_cursor_location (BtkEntry *entry)
{
  BdkRectangle area;
  gint strong_x;
  gint strong_xoffset;
  gint area_width, area_height;

  btk_entry_get_cursor_locations (entry, CURSOR_STANDARD, &strong_x, NULL);
  btk_entry_get_text_area_size (entry, NULL, NULL, &area_width, &area_height);

  strong_xoffset = strong_x - entry->scroll_offset;
  if (strong_xoffset < 0)
    {
      strong_xoffset = 0;
    }
  else if (strong_xoffset > area_width)
    {
      strong_xoffset = area_width;
    }
  area.x = strong_xoffset;
  area.y = 0;
  area.width = 0;
  area.height = area_height;

  btk_im_context_set_cursor_location (entry->im_context, &area);
}

static gboolean
recompute_idle_func (gpointer data)
{
  BtkEntry *entry;

  entry = BTK_ENTRY (data);

  entry->recompute_idle = 0;
  
  if (btk_widget_has_screen (BTK_WIDGET (entry)))
    {
      btk_entry_adjust_scroll (entry);
      btk_entry_queue_draw (entry);
      
      update_im_cursor_location (entry);
    }

  return FALSE;
}

static void
btk_entry_recompute (BtkEntry *entry)
{
  btk_entry_reset_layout (entry);
  btk_entry_check_cursor_blink (entry);
  
  if (!entry->recompute_idle)
    {
      entry->recompute_idle = bdk_threads_add_idle_full (G_PRIORITY_HIGH_IDLE + 15, /* between resize and redraw */
					       recompute_idle_func, entry, NULL); 
    }
}

static BangoLayout *
btk_entry_create_layout (BtkEntry *entry,
			 gboolean  include_preedit)
{
  BtkWidget *widget = BTK_WIDGET (entry);
  BangoLayout *layout = btk_widget_create_bango_layout (widget, NULL);
  BangoAttrList *tmp_attrs = bango_attr_list_new ();

  gchar *preedit_string = NULL;
  gint preedit_length = 0;
  BangoAttrList *preedit_attrs = NULL;

  gchar *display;
  guint n_bytes;

  bango_layout_set_single_paragraph_mode (layout, TRUE);

  display = btk_entry_get_display_text (entry, 0, -1);
  n_bytes = strlen (display);

  if (include_preedit)
    {
      btk_im_context_get_preedit_string (entry->im_context,
					 &preedit_string, &preedit_attrs, NULL);
      preedit_length = entry->preedit_length;
    }

  if (preedit_length)
    {
      GString *tmp_string = g_string_new (display);
      gint cursor_index = g_utf8_offset_to_pointer (display, entry->current_pos) - display;
      
      g_string_insert (tmp_string, cursor_index, preedit_string);
      
      bango_layout_set_text (layout, tmp_string->str, tmp_string->len);
      
      bango_attr_list_splice (tmp_attrs, preedit_attrs,
			      cursor_index, preedit_length);
      
      g_string_free (tmp_string, TRUE);
    }
  else
    {
      BangoDirection bango_dir;
      
      if (btk_entry_get_display_mode (entry) == DISPLAY_NORMAL)
	bango_dir = bango_find_base_dir (display, n_bytes);
      else
	bango_dir = BANGO_DIRECTION_NEUTRAL;

      if (bango_dir == BANGO_DIRECTION_NEUTRAL)
        {
          if (btk_widget_has_focus (widget))
	    {
	      BdkDisplay *display = btk_widget_get_display (widget);
	      BdkKeymap *keymap = bdk_keymap_get_for_display (display);
	      if (bdk_keymap_get_direction (keymap) == BANGO_DIRECTION_RTL)
		bango_dir = BANGO_DIRECTION_RTL;
	      else
		bango_dir = BANGO_DIRECTION_LTR;
	    }
          else
	    {
	      if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL)
		bango_dir = BANGO_DIRECTION_RTL;
	      else
		bango_dir = BANGO_DIRECTION_LTR;
	    }
        }

      bango_context_set_base_dir (btk_widget_get_bango_context (widget),
				  bango_dir);

      entry->resolved_dir = bango_dir;

      bango_layout_set_text (layout, display, n_bytes);
    }
      
  bango_layout_set_attributes (layout, tmp_attrs);

  g_free (preedit_string);
  g_free (display);

  if (preedit_attrs)
    bango_attr_list_unref (preedit_attrs);
      
  bango_attr_list_unref (tmp_attrs);

  return layout;
}

static BangoLayout *
btk_entry_ensure_layout (BtkEntry *entry,
                         gboolean  include_preedit)
{
  if (entry->preedit_length > 0 &&
      !include_preedit != !entry->cache_includes_preedit)
    btk_entry_reset_layout (entry);

  if (!entry->cached_layout)
    {
      entry->cached_layout = btk_entry_create_layout (entry, include_preedit);
      entry->cache_includes_preedit = include_preedit;
    }
  
  return entry->cached_layout;
}

static void
get_layout_position (BtkEntry *entry,
                     gint     *x,
                     gint     *y)
{
  BangoLayout *layout;
  BangoRectangle logical_rect;
  gint area_width, area_height;
  BtkBorder inner_border;
  gint y_pos;
  BangoLayoutLine *line;
  
  layout = btk_entry_ensure_layout (entry, TRUE);

  btk_entry_get_text_area_size (entry, NULL, NULL, &area_width, &area_height);
  _btk_entry_effective_inner_border (entry, &inner_border);

  area_height = BANGO_SCALE * (area_height - inner_border.top - inner_border.bottom);

  line = bango_layout_get_lines_readonly (layout)->data;
  bango_layout_line_get_extents (line, NULL, &logical_rect);
  
  /* Align primarily for locale's ascent/descent */
  y_pos = ((area_height - entry->ascent - entry->descent) / 2 + 
           entry->ascent + logical_rect.y);
  
  /* Now see if we need to adjust to fit in actual drawn string */
  if (logical_rect.height > area_height)
    y_pos = (area_height - logical_rect.height) / 2;
  else if (y_pos < 0)
    y_pos = 0;
  else if (y_pos + logical_rect.height > area_height)
    y_pos = area_height - logical_rect.height;
  
  y_pos = inner_border.top + y_pos / BANGO_SCALE;

  if (x)
    *x = inner_border.left - entry->scroll_offset;

  if (y)
    *y = y_pos;
}

static void
draw_text_with_color (BtkEntry *entry, bairo_t *cr, BdkColor *default_color)
{
  BangoLayout *layout = btk_entry_ensure_layout (entry, TRUE);
  BtkWidget *widget;
  gint x, y;
  gint start_pos, end_pos;

  widget = BTK_WIDGET (entry);

  bairo_save (cr);

  get_layout_position (entry, &x, &y);

  bairo_move_to (cr, x, y);
  bdk_bairo_set_source_color (cr, default_color);
  bango_bairo_show_layout (cr, layout);

  if (btk_editable_get_selection_bounds (BTK_EDITABLE (entry), &start_pos, &end_pos))
    {
      gint *ranges;
      gint n_ranges, i;
      BangoRectangle logical_rect;
      BdkColor *selection_color, *text_color;
      BtkBorder inner_border;

      bango_layout_get_pixel_extents (layout, NULL, &logical_rect);
      btk_entry_get_pixel_ranges (entry, &ranges, &n_ranges);

      if (btk_widget_has_focus (widget))
        {
          selection_color = &widget->style->base [BTK_STATE_SELECTED];
          text_color = &widget->style->text [BTK_STATE_SELECTED];
        }
      else
        {
          selection_color = &widget->style->base [BTK_STATE_ACTIVE];
	  text_color = &widget->style->text [BTK_STATE_ACTIVE];
        }

      _btk_entry_effective_inner_border (entry, &inner_border);

      for (i = 0; i < n_ranges; ++i)
        bairo_rectangle (cr,
        	         inner_border.left - entry->scroll_offset + ranges[2 * i],
			 y,
			 ranges[2 * i + 1],
			 logical_rect.height);

      bairo_clip (cr);
	  
      bdk_bairo_set_source_color (cr, selection_color);
      bairo_paint (cr);

      bairo_move_to (cr, x, y);
      bdk_bairo_set_source_color (cr, text_color);
      bango_bairo_show_layout (cr, layout);
  
      g_free (ranges);
    }
  bairo_restore (cr);
}

static void
btk_entry_draw_text (BtkEntry *entry)
{
  BtkWidget *widget = BTK_WIDGET (entry);
  bairo_t *cr;

  /* Nothing to display at all */
  if (btk_entry_get_display_mode (entry) == DISPLAY_BLANK)
    return;
  
  if (btk_widget_is_drawable (widget))
    {
      BdkColor text_color, bar_text_color;
      gint pos_x, pos_y;
      gint width, height;
      gint progress_x, progress_y, progress_width, progress_height;
      BtkStateType state;

      state = BTK_STATE_SELECTED;
      if (!btk_widget_get_sensitive (widget))
        state = BTK_STATE_INSENSITIVE;
      text_color = widget->style->text[widget->state];
      bar_text_color = widget->style->fg[state];

      get_progress_area (widget,
                         &progress_x, &progress_y,
                         &progress_width, &progress_height);

      cr = bdk_bairo_create (entry->text_area);

      /* If the color is the same, or the progress area has a zero
       * size, then we only need to draw once. */
      if ((text_color.pixel == bar_text_color.pixel) ||
          ((progress_width == 0) || (progress_height == 0)))
        {
          draw_text_with_color (entry, cr, &text_color);
        }
      else
        {
          width = bdk_window_get_width (entry->text_area);
          height = bdk_window_get_height (entry->text_area);

          bairo_rectangle (cr, 0, 0, width, height);
          bairo_clip (cr);
          bairo_save (cr);

          bairo_set_fill_rule (cr, BAIRO_FILL_RULE_EVEN_ODD);
          bairo_rectangle (cr, 0, 0, width, height);

          bdk_window_get_position (entry->text_area, &pos_x, &pos_y);
          progress_x -= pos_x;
          progress_y -= pos_y;

          bairo_rectangle (cr, progress_x, progress_y,
                           progress_width, progress_height);
          bairo_clip (cr);
          bairo_set_fill_rule (cr, BAIRO_FILL_RULE_WINDING);
      
          draw_text_with_color (entry, cr, &text_color);
          bairo_restore (cr);

          bairo_rectangle (cr, progress_x, progress_y,
                           progress_width, progress_height);
          bairo_clip (cr);

          draw_text_with_color (entry, cr, &bar_text_color);
        }

      bairo_destroy (cr);
    }
}

static void
draw_insertion_cursor (BtkEntry      *entry,
		       BdkRectangle  *cursor_location,
		       gboolean       is_primary,
		       BangoDirection direction,
		       gboolean       draw_arrow)
{
  BtkWidget *widget = BTK_WIDGET (entry);
  BtkTextDirection text_dir;

  if (direction == BANGO_DIRECTION_LTR)
    text_dir = BTK_TEXT_DIR_LTR;
  else
    text_dir = BTK_TEXT_DIR_RTL;

  btk_draw_insertion_cursor (widget, entry->text_area, NULL,
			     cursor_location,
			     is_primary, text_dir, draw_arrow);
}

static void
btk_entry_draw_cursor (BtkEntry  *entry,
		       CursorType type)
{
  BtkWidget *widget = BTK_WIDGET (entry);
  BdkKeymap *keymap = bdk_keymap_get_for_display (btk_widget_get_display (BTK_WIDGET (entry)));
  BangoDirection keymap_direction = bdk_keymap_get_direction (keymap);
  
  if (btk_widget_is_drawable (widget))
    {
      BdkRectangle cursor_location;
      gboolean split_cursor;
      BangoRectangle cursor_rect;
      BtkBorder inner_border;
      gint xoffset;
      gint text_area_height;
      gint cursor_index;
      gboolean block;
      gboolean block_at_line_end;
      BangoLayout *layout;
      const char *text;

      _btk_entry_effective_inner_border (entry, &inner_border);

      xoffset = inner_border.left - entry->scroll_offset;

      text_area_height = bdk_window_get_height (entry->text_area);

      layout = btk_entry_ensure_layout (entry, TRUE);
      text = bango_layout_get_text (layout);
      cursor_index = g_utf8_offset_to_pointer (text, entry->current_pos + entry->preedit_cursor) - text;
      if (!entry->overwrite_mode)
        block = FALSE;
      else
        block = _btk_text_util_get_block_cursor_location (layout,
                                                          cursor_index, &cursor_rect, &block_at_line_end);

      if (!block)
        {
          gint strong_x, weak_x;
          BangoDirection dir1 = BANGO_DIRECTION_NEUTRAL;
          BangoDirection dir2 = BANGO_DIRECTION_NEUTRAL;
          gint x1 = 0;
          gint x2 = 0;

          btk_entry_get_cursor_locations (entry, type, &strong_x, &weak_x);

          g_object_get (btk_widget_get_settings (widget),
                        "btk-split-cursor", &split_cursor,
                        NULL);

          dir1 = entry->resolved_dir;
      
          if (split_cursor)
            {
              x1 = strong_x;

              if (weak_x != strong_x)
                {
                  dir2 = (entry->resolved_dir == BANGO_DIRECTION_LTR) ? BANGO_DIRECTION_RTL : BANGO_DIRECTION_LTR;
                  x2 = weak_x;
                }
            }
          else
            {
              if (keymap_direction == entry->resolved_dir)
                x1 = strong_x;
              else
                x1 = weak_x;
            }

          cursor_location.x = xoffset + x1;
          cursor_location.y = inner_border.top;
          cursor_location.width = 0;
          cursor_location.height = text_area_height - inner_border.top - inner_border.bottom;

          draw_insertion_cursor (entry,
                                 &cursor_location, TRUE, dir1,
                                 dir2 != BANGO_DIRECTION_NEUTRAL);
      
          if (dir2 != BANGO_DIRECTION_NEUTRAL)
            {
              cursor_location.x = xoffset + x2;
              draw_insertion_cursor (entry,
                                     &cursor_location, FALSE, dir2,
                                     TRUE);
            }
        }
      else /* overwrite_mode */
        {
          BdkColor cursor_color;
          BdkRectangle rect;
          bairo_t *cr;
          gint x, y;

          get_layout_position (entry, &x, &y);

          rect.x = BANGO_PIXELS (cursor_rect.x) + x;
          rect.y = BANGO_PIXELS (cursor_rect.y) + y;
          rect.width = BANGO_PIXELS (cursor_rect.width);
          rect.height = BANGO_PIXELS (cursor_rect.height);

          cr = bdk_bairo_create (entry->text_area);

          _btk_widget_get_cursor_color (widget, &cursor_color);
          bdk_bairo_set_source_color (cr, &cursor_color);
          bdk_bairo_rectangle (cr, &rect);
          bairo_fill (cr);

          if (!block_at_line_end)
            {
              bdk_bairo_rectangle (cr, &rect);
              bairo_clip (cr);
              bairo_move_to (cr, x, y);
              bdk_bairo_set_source_color (cr, &widget->style->base[widget->state]);
              bango_bairo_show_layout (cr, layout);
            }

          bairo_destroy (cr);
        }
    }
}

static void
btk_entry_queue_draw (BtkEntry *entry)
{
  if (btk_widget_is_drawable (BTK_WIDGET (entry)))
    bdk_window_invalidate_rect (entry->text_area, NULL, FALSE);
}

void
_btk_entry_reset_im_context (BtkEntry *entry)
{
  if (entry->need_im_reset)
    {
      entry->need_im_reset = FALSE;
      btk_im_context_reset (entry->im_context);
    }
}

/**
 * btk_entry_reset_im_context:
 * @entry: a #BtkEntry
 *
 * Reset the input method context of the entry if needed.
 *
 * This can be necessary in the case where modifying the buffer
 * would confuse on-going input method behavior.
 *
 * Since: 2.22
 */
void
btk_entry_reset_im_context (BtkEntry *entry)
{
  g_return_if_fail (BTK_IS_ENTRY (entry));

  _btk_entry_reset_im_context (entry);
}

/**
 * btk_entry_im_context_filter_keypress:
 * @entry: a #BtkEntry
 * @event: the key event
 *
 * Allow the #BtkEntry input method to internally handle key press
 * and release events. If this function returns %TRUE, then no further
 * processing should be done for this key event. See
 * btk_im_context_filter_keypress().
 *
 * Note that you are expected to call this function from your handler
 * when overriding key event handling. This is needed in the case when
 * you need to insert your own key handling between the input method
 * and the default key event handling of the #BtkEntry.
 * See btk_text_view_reset_im_context() for an example of use.
 *
 * Return value: %TRUE if the input method handled the key event.
 *
 * Since: 2.22
 */
gboolean
btk_entry_im_context_filter_keypress (BtkEntry    *entry,
                                      BdkEventKey *event)
{
  g_return_val_if_fail (BTK_IS_ENTRY (entry), FALSE);

  return btk_im_context_filter_keypress (entry->im_context, event);
}


static gint
btk_entry_find_position (BtkEntry *entry,
			 gint      x)
{
  BangoLayout *layout;
  BangoLayoutLine *line;
  gint index;
  gint pos;
  gint trailing;
  const gchar *text;
  gint cursor_index;
  
  layout = btk_entry_ensure_layout (entry, TRUE);
  text = bango_layout_get_text (layout);
  cursor_index = g_utf8_offset_to_pointer (text, entry->current_pos) - text;
  
  line = bango_layout_get_lines_readonly (layout)->data;
  bango_layout_line_x_to_index (line, x * BANGO_SCALE, &index, &trailing);

  if (index >= cursor_index && entry->preedit_length)
    {
      if (index >= cursor_index + entry->preedit_length)
	index -= entry->preedit_length;
      else
	{
	  index = cursor_index;
	  trailing = 0;
	}
    }

  pos = g_utf8_pointer_to_offset (text, text + index);
  pos += trailing;

  return pos;
}

static void
btk_entry_get_cursor_locations (BtkEntry   *entry,
				CursorType  type,
				gint       *strong_x,
				gint       *weak_x)
{
  DisplayMode mode = btk_entry_get_display_mode (entry);

  /* Nothing to display at all, so no cursor is relevant */
  if (mode == DISPLAY_BLANK)
    {
      if (strong_x)
	*strong_x = 0;
      
      if (weak_x)
	*weak_x = 0;
    }
  else
    {
      BangoLayout *layout = btk_entry_ensure_layout (entry, TRUE);
      const gchar *text = bango_layout_get_text (layout);
      BangoRectangle strong_pos, weak_pos;
      gint index;
  
      if (type == CURSOR_STANDARD)
	{
	  index = g_utf8_offset_to_pointer (text, entry->current_pos + entry->preedit_cursor) - text;
	}
      else /* type == CURSOR_DND */
	{
	  index = g_utf8_offset_to_pointer (text, entry->dnd_position) - text;

	  if (entry->dnd_position > entry->current_pos)
	    {
	      if (mode == DISPLAY_NORMAL)
		index += entry->preedit_length;
	      else
		{
		  gint preedit_len_chars = g_utf8_strlen (text, -1) - btk_entry_buffer_get_length (get_buffer (entry));
		  index += preedit_len_chars * g_unichar_to_utf8 (entry->invisible_char, NULL);
		}
	    }
	}
      
      bango_layout_get_cursor_pos (layout, index, &strong_pos, &weak_pos);
      
      if (strong_x)
	*strong_x = strong_pos.x / BANGO_SCALE;
      
      if (weak_x)
	*weak_x = weak_pos.x / BANGO_SCALE;
    }
}

static void
btk_entry_adjust_scroll (BtkEntry *entry)
{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);
  gint min_offset, max_offset;
  gint text_area_width, text_width;
  BtkBorder inner_border;
  gint strong_x, weak_x;
  gint strong_xoffset, weak_xoffset;
  gfloat xalign;
  BangoLayout *layout;
  BangoLayoutLine *line;
  BangoRectangle logical_rect;

  if (!btk_widget_get_realized (BTK_WIDGET (entry)))
    return;

  _btk_entry_effective_inner_border (entry, &inner_border);

  text_area_width = bdk_window_get_width (entry->text_area);
  text_area_width -= inner_border.left + inner_border.right;
  if (text_area_width < 0)
    text_area_width = 0;

  layout = btk_entry_ensure_layout (entry, TRUE);
  line = bango_layout_get_lines_readonly (layout)->data;

  bango_layout_line_get_extents (line, NULL, &logical_rect);

  /* Display as much text as we can */

  if (entry->resolved_dir == BANGO_DIRECTION_LTR)
      xalign = priv->xalign;
  else
      xalign = 1.0 - priv->xalign;

  text_width = BANGO_PIXELS(logical_rect.width);

  if (text_width > text_area_width)
    {
      min_offset = 0;
      max_offset = text_width - text_area_width;
    }
  else
    {
      min_offset = (text_width - text_area_width) * xalign;
      max_offset = min_offset;
    }

  entry->scroll_offset = CLAMP (entry->scroll_offset, min_offset, max_offset);

  /* And make sure cursors are on screen. Note that the cursor is
   * actually drawn one pixel into the INNER_BORDER space on
   * the right, when the scroll is at the utmost right. This
   * looks better to to me than confining the cursor inside the
   * border entirely, though it means that the cursor gets one
   * pixel closer to the edge of the widget on the right than
   * on the left. This might need changing if one changed
   * INNER_BORDER from 2 to 1, as one would do on a
   * small-screen-real-estate display.
   *
   * We always make sure that the strong cursor is on screen, and
   * put the weak cursor on screen if possible.
   */

  btk_entry_get_cursor_locations (entry, CURSOR_STANDARD, &strong_x, &weak_x);
  
  strong_xoffset = strong_x - entry->scroll_offset;

  if (strong_xoffset < 0)
    {
      entry->scroll_offset += strong_xoffset;
      strong_xoffset = 0;
    }
  else if (strong_xoffset > text_area_width)
    {
      entry->scroll_offset += strong_xoffset - text_area_width;
      strong_xoffset = text_area_width;
    }

  weak_xoffset = weak_x - entry->scroll_offset;

  if (weak_xoffset < 0 && strong_xoffset - weak_xoffset <= text_area_width)
    {
      entry->scroll_offset += weak_xoffset;
    }
  else if (weak_xoffset > text_area_width &&
	   strong_xoffset - (weak_xoffset - text_area_width) >= 0)
    {
      entry->scroll_offset += weak_xoffset - text_area_width;
    }

  g_object_notify (G_OBJECT (entry), "scroll-offset");
}

static void
btk_entry_move_adjustments (BtkEntry *entry)
{
  BangoContext *context;
  BangoFontMetrics *metrics;
  gint x, layout_x, border_x, border_y;
  gint char_width;
  BtkAdjustment *adjustment;

  adjustment = g_object_get_qdata (G_OBJECT (entry), quark_cursor_hadjustment);
  if (!adjustment)
    return;

  /* Cursor position, layout offset, border width, and widget allocation */
  btk_entry_get_cursor_locations (entry, CURSOR_STANDARD, &x, NULL);
  get_layout_position (entry, &layout_x, NULL);
  _btk_entry_get_borders (entry, &border_x, &border_y);
  x += entry->widget.allocation.x + layout_x + border_x;

  /* Approximate width of a char, so user can see what is ahead/behind */
  context = btk_widget_get_bango_context (BTK_WIDGET (entry));
  metrics = bango_context_get_metrics (context, 
                                       entry->widget.style->font_desc,
				       bango_context_get_language (context));
  char_width = bango_font_metrics_get_approximate_char_width (metrics) / BANGO_SCALE;

  /* Scroll it */
  btk_adjustment_clamp_page (adjustment, 
  			     x - (char_width + 1),   /* one char + one pixel before */
			     x + (char_width + 2));  /* one char + cursor + one pixel after */
}

static gint
btk_entry_move_visually (BtkEntry *entry,
			 gint      start,
			 gint      count)
{
  gint index;
  BangoLayout *layout = btk_entry_ensure_layout (entry, FALSE);
  const gchar *text;

  text = bango_layout_get_text (layout);
  
  index = g_utf8_offset_to_pointer (text, start) - text;

  while (count != 0)
    {
      int new_index, new_trailing;
      gboolean split_cursor;
      gboolean strong;

      g_object_get (btk_widget_get_settings (BTK_WIDGET (entry)),
		    "btk-split-cursor", &split_cursor,
		    NULL);

      if (split_cursor)
	strong = TRUE;
      else
	{
	  BdkKeymap *keymap = bdk_keymap_get_for_display (btk_widget_get_display (BTK_WIDGET (entry)));
	  BangoDirection keymap_direction = bdk_keymap_get_direction (keymap);

	  strong = keymap_direction == entry->resolved_dir;
	}
      
      if (count > 0)
	{
	  bango_layout_move_cursor_visually (layout, strong, index, 0, 1, &new_index, &new_trailing);
	  count--;
	}
      else
	{
	  bango_layout_move_cursor_visually (layout, strong, index, 0, -1, &new_index, &new_trailing);
	  count++;
	}

      if (new_index < 0)
        index = 0;
      else if (new_index != G_MAXINT)
        index = new_index;
      
      while (new_trailing--)
	index = g_utf8_next_char (text + index) - text;
    }
  
  return g_utf8_pointer_to_offset (text, text + index);
}

static gint
btk_entry_move_logically (BtkEntry *entry,
			  gint      start,
			  gint      count)
{
  gint new_pos = start;
  guint length;

  length = btk_entry_buffer_get_length (get_buffer (entry));

  /* Prevent any leak of information */
  if (btk_entry_get_display_mode (entry) != DISPLAY_NORMAL)
    {
      new_pos = CLAMP (start + count, 0, length);
    }
  else
    {
      BangoLayout *layout = btk_entry_ensure_layout (entry, FALSE);
      BangoLogAttr *log_attrs;
      gint n_attrs;

      bango_layout_get_log_attrs (layout, &log_attrs, &n_attrs);

      while (count > 0 && new_pos < length)
	{
	  do
	    new_pos++;
	  while (new_pos < length && !log_attrs[new_pos].is_cursor_position);
	  
	  count--;
	}
      while (count < 0 && new_pos > 0)
	{
	  do
	    new_pos--;
	  while (new_pos > 0 && !log_attrs[new_pos].is_cursor_position);
	  
	  count++;
	}
      
      g_free (log_attrs);
    }

  return new_pos;
}

static gint
btk_entry_move_forward_word (BtkEntry *entry,
			     gint      start,
                             gboolean  allow_whitespace)
{
  gint new_pos = start;
  guint length;

  length = btk_entry_buffer_get_length (get_buffer (entry));

  /* Prevent any leak of information */
  if (btk_entry_get_display_mode (entry) != DISPLAY_NORMAL)
    {
      new_pos = length;
    }
  else if (new_pos < length)
    {
      BangoLayout *layout = btk_entry_ensure_layout (entry, FALSE);
      BangoLogAttr *log_attrs;
      gint n_attrs;

      bango_layout_get_log_attrs (layout, &log_attrs, &n_attrs);
      
      /* Find the next word boundary */
      new_pos++;
      while (new_pos < n_attrs - 1 && !(log_attrs[new_pos].is_word_end ||
                                        (log_attrs[new_pos].is_word_start && allow_whitespace)))
	new_pos++;

      g_free (log_attrs);
    }

  return new_pos;
}


static gint
btk_entry_move_backward_word (BtkEntry *entry,
			      gint      start,
                              gboolean  allow_whitespace)
{
  gint new_pos = start;

  /* Prevent any leak of information */
  if (btk_entry_get_display_mode (entry) != DISPLAY_NORMAL)
    {
      new_pos = 0;
    }
  else if (start > 0)
    {
      BangoLayout *layout = btk_entry_ensure_layout (entry, FALSE);
      BangoLogAttr *log_attrs;
      gint n_attrs;

      bango_layout_get_log_attrs (layout, &log_attrs, &n_attrs);

      new_pos = start - 1;

      /* Find the previous word boundary */
      while (new_pos > 0 && !(log_attrs[new_pos].is_word_start || 
                              (log_attrs[new_pos].is_word_end && allow_whitespace)))
	new_pos--;

      g_free (log_attrs);
    }

  return new_pos;
}

static void
btk_entry_delete_whitespace (BtkEntry *entry)
{
  BangoLayout *layout = btk_entry_ensure_layout (entry, FALSE);
  BangoLogAttr *log_attrs;
  gint n_attrs;
  gint start, end;

  bango_layout_get_log_attrs (layout, &log_attrs, &n_attrs);

  start = end = entry->current_pos;
  
  while (start > 0 && log_attrs[start-1].is_white)
    start--;

  while (end < n_attrs && log_attrs[end].is_white)
    end++;

  g_free (log_attrs);

  if (start != end)
    btk_editable_delete_text (BTK_EDITABLE (entry), start, end);
}


static void
btk_entry_select_word (BtkEntry *entry)
{
  gint start_pos = btk_entry_move_backward_word (entry, entry->current_pos, TRUE);
  gint end_pos = btk_entry_move_forward_word (entry, entry->current_pos, TRUE);

  btk_editable_select_rebunnyion (BTK_EDITABLE (entry), start_pos, end_pos);
}

static void
btk_entry_select_line (BtkEntry *entry)
{
  btk_editable_select_rebunnyion (BTK_EDITABLE (entry), 0, -1);
}

static gint
truncate_multiline (const gchar *text)
{
  gint length;

  for (length = 0;
       text[length] && text[length] != '\n' && text[length] != '\r';
       length++);

  return length;
}

static void
paste_received (BtkClipboard *clipboard,
		const gchar  *text,
		gpointer      data)
{
  BtkEntry *entry = BTK_ENTRY (data);
  BtkEditable *editable = BTK_EDITABLE (entry);
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);
      
  if (entry->button == 2)
    {
      gint pos, start, end;
      pos = priv->insert_pos;
      btk_editable_get_selection_bounds (editable, &start, &end);
      if (!((start <= pos && pos <= end) || (end <= pos && pos <= start)))
	btk_editable_select_rebunnyion (editable, pos, pos);
    }
      
  if (text)
    {
      gint pos, start, end;
      gint length = -1;
      gboolean popup_completion;
      BtkEntryCompletion *completion;

      completion = btk_entry_get_completion (entry);

      if (entry->truncate_multiline)
        length = truncate_multiline (text);

      /* only complete if the selection is at the end */
      popup_completion = (btk_entry_buffer_get_length (get_buffer (entry)) ==
                          MAX (entry->current_pos, entry->selection_bound));

      if (completion)
	{
	  if (btk_widget_get_mapped (completion->priv->popup_window))
	    _btk_entry_completion_popdown (completion);

          if (!popup_completion && completion->priv->changed_id > 0)
            g_signal_handler_block (entry, completion->priv->changed_id);
	}

      begin_change (entry);
      if (btk_editable_get_selection_bounds (editable, &start, &end))
        btk_editable_delete_text (editable, start, end);

      pos = entry->current_pos;
      btk_editable_insert_text (editable, text, length, &pos);
      btk_editable_set_position (editable, pos);
      end_change (entry);

      if (completion &&
          !popup_completion && completion->priv->changed_id > 0)
	g_signal_handler_unblock (entry, completion->priv->changed_id);
    }

  g_object_unref (entry);
}

static void
btk_entry_paste (BtkEntry *entry,
		 BdkAtom   selection)
{
  g_object_ref (entry);
  btk_clipboard_request_text (btk_widget_get_clipboard (BTK_WIDGET (entry), selection),
			      paste_received, entry);
}

static void
primary_get_cb (BtkClipboard     *clipboard,
		BtkSelectionData *selection_data,
		guint             info,
		gpointer          data)
{
  BtkEntry *entry = BTK_ENTRY (data);
  gint start, end;
  
  if (btk_editable_get_selection_bounds (BTK_EDITABLE (entry), &start, &end))
    {
      gchar *str = btk_entry_get_display_text (entry, start, end);
      btk_selection_data_set_text (selection_data, str, -1);
      g_free (str);
    }
}

static void
primary_clear_cb (BtkClipboard *clipboard,
		  gpointer      data)
{
  BtkEntry *entry = BTK_ENTRY (data);

  btk_editable_select_rebunnyion (BTK_EDITABLE (entry), entry->current_pos, entry->current_pos);
}

static void
btk_entry_update_primary_selection (BtkEntry *entry)
{
  BtkTargetList *list;
  BtkTargetEntry *targets;
  BtkClipboard *clipboard;
  gint start, end;
  gint n_targets;

  if (!btk_widget_get_realized (BTK_WIDGET (entry)))
    return;

  list = btk_target_list_new (NULL, 0);
  btk_target_list_add_text_targets (list, 0);

  targets = btk_target_table_new_from_list (list, &n_targets);

  clipboard = btk_widget_get_clipboard (BTK_WIDGET (entry), BDK_SELECTION_PRIMARY);
  
  if (btk_editable_get_selection_bounds (BTK_EDITABLE (entry), &start, &end))
    {
      if (!btk_clipboard_set_with_owner (clipboard, targets, n_targets,
					 primary_get_cb, primary_clear_cb, G_OBJECT (entry)))
	primary_clear_cb (clipboard, entry);
    }
  else
    {
      if (btk_clipboard_get_owner (clipboard) == G_OBJECT (entry))
	btk_clipboard_clear (clipboard);
    }

  btk_target_table_free (targets, n_targets);
  btk_target_list_unref (list);
}

static void
btk_entry_clear (BtkEntry             *entry,
                 BtkEntryIconPosition  icon_pos)
{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);
  EntryIconInfo *icon_info = priv->icons[icon_pos];

  if (!icon_info || icon_info->storage_type == BTK_IMAGE_EMPTY)
    return;

  g_object_freeze_notify (G_OBJECT (entry));

  /* Explicitly check, as the pointer may become invalidated
   * during destruction.
   */
  if (BDK_IS_WINDOW (icon_info->window))
    bdk_window_hide (icon_info->window);

  if (icon_info->pixbuf)
    {
      g_object_unref (icon_info->pixbuf);
      icon_info->pixbuf = NULL;
    }

  switch (icon_info->storage_type)
    {
    case BTK_IMAGE_PIXBUF:
      g_object_notify (G_OBJECT (entry),
                       icon_pos == BTK_ENTRY_ICON_PRIMARY ? "primary-icon-pixbuf" : "secondary-icon-pixbuf");
      break;

    case BTK_IMAGE_STOCK:
      g_free (icon_info->stock_id);
      icon_info->stock_id = NULL;
      g_object_notify (G_OBJECT (entry),
                       icon_pos == BTK_ENTRY_ICON_PRIMARY ? "primary-icon-stock" : "secondary-icon-stock");
      break;

    case BTK_IMAGE_ICON_NAME:
      g_free (icon_info->icon_name);
      icon_info->icon_name = NULL;
      g_object_notify (G_OBJECT (entry),
                       icon_pos == BTK_ENTRY_ICON_PRIMARY ? "primary-icon-name" : "secondary-icon-name");
      break;

    case BTK_IMAGE_GICON:
      if (icon_info->gicon)
        {
          g_object_unref (icon_info->gicon);
          icon_info->gicon = NULL;
        }
      g_object_notify (G_OBJECT (entry),
                       icon_pos == BTK_ENTRY_ICON_PRIMARY ? "primary-icon-gicon" : "secondary-icon-gicon");
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  icon_info->storage_type = BTK_IMAGE_EMPTY;
  g_object_notify (G_OBJECT (entry),
                   icon_pos == BTK_ENTRY_ICON_PRIMARY ? "primary-icon-storage-type" : "secondary-icon-storage-type");

  g_object_thaw_notify (G_OBJECT (entry));
}

static void
btk_entry_ensure_pixbuf (BtkEntry             *entry,
                         BtkEntryIconPosition  icon_pos)
{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);
  EntryIconInfo *icon_info = priv->icons[icon_pos];
  BtkIconInfo *info;
  BtkIconTheme *icon_theme;
  BtkSettings *settings;
  BtkStateType state;
  BtkWidget *widget;
  BdkScreen *screen;
  gint width, height;

  if (!icon_info || icon_info->pixbuf)
    return;

  widget = BTK_WIDGET (entry);

  switch (icon_info->storage_type)
    {
    case BTK_IMAGE_EMPTY:
    case BTK_IMAGE_PIXBUF:
      break;
    case BTK_IMAGE_STOCK:
      state = btk_widget_get_state (widget);
      btk_widget_set_state (widget, BTK_STATE_NORMAL);
      icon_info->pixbuf = btk_widget_render_icon (widget,
                                                  icon_info->stock_id,
                                                  BTK_ICON_SIZE_MENU,
                                                  NULL);
      if (!icon_info->pixbuf)
        icon_info->pixbuf = btk_widget_render_icon (widget,
                                                    BTK_STOCK_MISSING_IMAGE,
                                                    BTK_ICON_SIZE_MENU,
                                                    NULL);
      btk_widget_set_state (widget, state);
      break;

    case BTK_IMAGE_ICON_NAME:
      screen = btk_widget_get_screen (widget);
      if (screen)
        {
          icon_theme = btk_icon_theme_get_for_screen (screen);
          settings = btk_settings_get_for_screen (screen);
          
          btk_icon_size_lookup_for_settings (settings,
                                             BTK_ICON_SIZE_MENU,
                                             &width, &height);

          icon_info->pixbuf = btk_icon_theme_load_icon (icon_theme,
                                                        icon_info->icon_name,
                                                        MIN (width, height),
                                                        0, NULL);

          if (icon_info->pixbuf == NULL)
            {
              state = btk_widget_get_state (widget);
              btk_widget_set_state (widget, BTK_STATE_NORMAL);
              icon_info->pixbuf = btk_widget_render_icon (widget,
                                                          BTK_STOCK_MISSING_IMAGE,
                                                          BTK_ICON_SIZE_MENU,
                                                          NULL);
              btk_widget_set_state (widget, state);
            }
        }
      break;

    case BTK_IMAGE_GICON:
      screen = btk_widget_get_screen (widget);
      if (screen)
        {
          icon_theme = btk_icon_theme_get_for_screen (screen);
          settings = btk_settings_get_for_screen (screen);

          btk_icon_size_lookup_for_settings (settings,
                                             BTK_ICON_SIZE_MENU,
                                             &width, &height);

          info = btk_icon_theme_lookup_by_gicon (icon_theme,
                                                 icon_info->gicon,
                                                 MIN (width, height), 
                                                 BTK_ICON_LOOKUP_USE_BUILTIN);
          if (info)
            {
              icon_info->pixbuf = btk_icon_info_load_icon (info, NULL);
              btk_icon_info_free (info);
            }

          if (icon_info->pixbuf == NULL)
            {
              state = btk_widget_get_state (widget);
              btk_widget_set_state (widget, BTK_STATE_NORMAL);
              icon_info->pixbuf = btk_widget_render_icon (widget,
                                                          BTK_STOCK_MISSING_IMAGE,
                                                          BTK_ICON_SIZE_MENU,
                                                          NULL);
              btk_widget_set_state (widget, state);
            }
        }
      break;

    default:
      g_assert_not_reached ();
      break;
    }
    
  if (icon_info->pixbuf != NULL && icon_info->window != NULL)
    bdk_window_show_unraised (icon_info->window);
}


/* Public API
 */

/**
 * btk_entry_new:
 *
 * Creates a new entry.
 *
 * Return value: a new #BtkEntry.
 */
BtkWidget*
btk_entry_new (void)
{
  return g_object_new (BTK_TYPE_ENTRY, NULL);
}

/**
 * btk_entry_new_with_buffer:
 * @buffer: The buffer to use for the new #BtkEntry.
 *
 * Creates a new entry with the specified text buffer.
 *
 * Return value: a new #BtkEntry
 *
 * Since: 2.18
 */
BtkWidget*
btk_entry_new_with_buffer (BtkEntryBuffer *buffer)
{
  g_return_val_if_fail (BTK_IS_ENTRY_BUFFER (buffer), NULL);
  return g_object_new (BTK_TYPE_ENTRY, "buffer", buffer, NULL);
}

/**
 * btk_entry_new_with_max_length:
 * @max: the maximum length of the entry, or 0 for no maximum.
 *   (other than the maximum length of entries.) The value passed in will
 *   be clamped to the range 0-65536.
 *
 * Creates a new #BtkEntry widget with the given maximum length.
 * 
 * Return value: a new #BtkEntry
 *
 * Deprecated: 2.0: Use btk_entry_set_max_length() instead.
 **/
BtkWidget*
btk_entry_new_with_max_length (gint max)
{
  BtkEntry *entry;

  max = CLAMP (max, 0, BTK_ENTRY_BUFFER_MAX_SIZE);

  entry = g_object_new (BTK_TYPE_ENTRY, NULL);
  btk_entry_buffer_set_max_length (get_buffer (entry), max);

  return BTK_WIDGET (entry);
}


static BtkEntryBuffer*
get_buffer (BtkEntry *entry)
{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);

  if (priv->buffer == NULL)
    {
      BtkEntryBuffer *buffer;
      buffer = btk_entry_buffer_new (NULL, 0);
      btk_entry_set_buffer (entry, buffer);
      g_object_unref (buffer);
    }

  return priv->buffer;
}

/**
 * btk_entry_get_buffer:
 * @entry: a #BtkEntry
 *
 * Get the #BtkEntryBuffer object which holds the text for
 * this widget.
 *
 * Since: 2.18
 *
 * Returns: (transfer none): A #BtkEntryBuffer object.
 */
BtkEntryBuffer*
btk_entry_get_buffer (BtkEntry *entry)
{
  g_return_val_if_fail (BTK_IS_ENTRY (entry), NULL);

  return get_buffer (entry);
}

/**
 * btk_entry_set_buffer:
 * @entry: a #BtkEntry
 * @buffer: a #BtkEntryBuffer
 *
 * Set the #BtkEntryBuffer object which holds the text for
 * this widget.
 *
 * Since: 2.18
 */
void
btk_entry_set_buffer (BtkEntry       *entry,
                      BtkEntryBuffer *buffer)
{
  BtkEntryPrivate *priv;
  GObject *obj;

  g_return_if_fail (BTK_IS_ENTRY (entry));
  priv = BTK_ENTRY_GET_PRIVATE (entry);

  if (buffer)
    {
      g_return_if_fail (BTK_IS_ENTRY_BUFFER (buffer));
      g_object_ref (buffer);
    }

  if (priv->buffer)
    {
      buffer_disconnect_signals (entry);
      g_object_unref (priv->buffer);

      /* COMPAT: Deprecated. Not used. Setting these fields no longer necessary in BTK 3.x */
      entry->text = NULL;
      entry->text_length = 0;
      entry->text_max_length = 0;
    }

  priv->buffer = buffer;

  if (priv->buffer)
    {
       buffer_connect_signals (entry);

      /* COMPAT: Deprecated. Not used. Setting these fields no longer necessary in BTK 3.x */
      entry->text = (char*)btk_entry_buffer_get_text (priv->buffer);
      entry->text_length = btk_entry_buffer_get_length (priv->buffer);
      entry->text_max_length = btk_entry_buffer_get_max_length (priv->buffer);
    }

  obj = G_OBJECT (entry);
  g_object_freeze_notify (obj);
  g_object_notify (obj, "buffer");
  g_object_notify (obj, "text");
  g_object_notify (obj, "text-length");
  g_object_notify (obj, "max-length");
  g_object_notify (obj, "visibility");
  g_object_notify (obj, "invisible-char");
  g_object_notify (obj, "invisible-char-set");
  g_object_thaw_notify (obj);

  btk_editable_set_position (BTK_EDITABLE (entry), 0);
  btk_entry_recompute (entry);
}

/**
 * btk_entry_get_text_window:
 * @entry: a #BtkEntry
 *
 * Returns the #BdkWindow which contains the text. This function is
 * useful when drawing something to the entry in an expose-event
 * callback because it enables the callback to distinguish between
 * the text window and entry's icon windows.
 *
 * See also btk_entry_get_icon_window().
 *
 * Note that BTK+ 3 does not have this function anymore; it has
 * been replaced by btk_entry_get_text_area().
 *
 * Return value: (transfer none): the entry's text window.
 *
 * Since: 2.20
 **/
BdkWindow *
btk_entry_get_text_window (BtkEntry *entry)
{
  g_return_val_if_fail (BTK_IS_ENTRY (entry), NULL);

  return entry->text_area;
}


/**
 * btk_entry_set_text:
 * @entry: a #BtkEntry
 * @text: the new text
 *
 * Sets the text in the widget to the given
 * value, replacing the current contents.
 *
 * See btk_entry_buffer_set_text().
 */
void
btk_entry_set_text (BtkEntry    *entry,
		    const gchar *text)
{
  gint tmp_pos;
  BtkEntryCompletion *completion;
  BtkEntryPrivate *priv;

  g_return_if_fail (BTK_IS_ENTRY (entry));
  g_return_if_fail (text != NULL);
  priv = BTK_ENTRY_GET_PRIVATE (entry);

  /* Actually setting the text will affect the cursor and selection;
   * if the contents don't actually change, this will look odd to the user.
   */
  if (strcmp (btk_entry_buffer_get_text (get_buffer (entry)), text) == 0)
    return;

  completion = btk_entry_get_completion (entry);
  if (completion && completion->priv->changed_id > 0)
    g_signal_handler_block (entry, completion->priv->changed_id);

  begin_change (entry);
  btk_editable_delete_text (BTK_EDITABLE (entry), 0, -1);
  tmp_pos = 0;
  btk_editable_insert_text (BTK_EDITABLE (entry), text, strlen (text), &tmp_pos);
  end_change (entry);

  if (completion && completion->priv->changed_id > 0)
    g_signal_handler_unblock (entry, completion->priv->changed_id);
}

/**
 * btk_entry_append_text:
 * @entry: a #BtkEntry
 * @text: the text to append
 *
 * Appends the given text to the contents of the widget.
 *
 * Deprecated: 2.0: Use btk_editable_insert_text() instead.
 */
void
btk_entry_append_text (BtkEntry *entry,
		       const gchar *text)
{
  BtkEntryPrivate *priv;
  gint tmp_pos;

  g_return_if_fail (BTK_IS_ENTRY (entry));
  g_return_if_fail (text != NULL);
  priv = BTK_ENTRY_GET_PRIVATE (entry);

  tmp_pos = btk_entry_buffer_get_length (get_buffer (entry));
  btk_editable_insert_text (BTK_EDITABLE (entry), text, -1, &tmp_pos);
}

/**
 * btk_entry_prepend_text:
 * @entry: a #BtkEntry
 * @text: the text to prepend
 *
 * Prepends the given text to the contents of the widget.
 *
 * Deprecated: 2.0: Use btk_editable_insert_text() instead.
 */
void
btk_entry_prepend_text (BtkEntry *entry,
			const gchar *text)
{
  gint tmp_pos;

  g_return_if_fail (BTK_IS_ENTRY (entry));
  g_return_if_fail (text != NULL);

  tmp_pos = 0;
  btk_editable_insert_text (BTK_EDITABLE (entry), text, -1, &tmp_pos);
}

/**
 * btk_entry_set_position:
 * @entry: a #BtkEntry
 * @position: the position of the cursor. The cursor is displayed
 *    before the character with the given (base 0) index in the widget. 
 *    The value must be less than or equal to the number of characters 
 *    in the widget. A value of -1 indicates that the position should
 *    be set after the last character in the entry. Note that this 
 *    position is in characters, not in bytes.
 *
 * Sets the cursor position in an entry to the given value. 
 *
 * Deprecated: 2.0: Use btk_editable_set_position() instead.
 */
void
btk_entry_set_position (BtkEntry *entry,
			gint      position)
{
  g_return_if_fail (BTK_IS_ENTRY (entry));

  btk_editable_set_position (BTK_EDITABLE (entry), position);
}

/**
 * btk_entry_set_visibility:
 * @entry: a #BtkEntry
 * @visible: %TRUE if the contents of the entry are displayed
 *           as plaintext
 *
 * Sets whether the contents of the entry are visible or not. 
 * When visibility is set to %FALSE, characters are displayed 
 * as the invisible char, and will also appear that way when 
 * the text in the entry widget is copied elsewhere.
 *
 * By default, BTK+ picks the best invisible character available
 * in the current font, but it can be changed with
 * btk_entry_set_invisible_char().
 */
void
btk_entry_set_visibility (BtkEntry *entry,
			  gboolean visible)
{
  g_return_if_fail (BTK_IS_ENTRY (entry));

  visible = visible != FALSE;

  if (entry->visible != visible)
    {
      entry->visible = visible;

      g_object_notify (G_OBJECT (entry), "visibility");
      btk_entry_recompute (entry);
    }
}

/**
 * btk_entry_get_visibility:
 * @entry: a #BtkEntry
 *
 * Retrieves whether the text in @entry is visible. See
 * btk_entry_set_visibility().
 *
 * Return value: %TRUE if the text is currently visible
 **/
gboolean
btk_entry_get_visibility (BtkEntry *entry)
{
  g_return_val_if_fail (BTK_IS_ENTRY (entry), FALSE);

  return entry->visible;
}

/**
 * btk_entry_set_invisible_char:
 * @entry: a #BtkEntry
 * @ch: a Unicode character
 * 
 * Sets the character to use in place of the actual text when
 * btk_entry_set_visibility() has been called to set text visibility
 * to %FALSE. i.e. this is the character used in "password mode" to
 * show the user how many characters have been typed. By default, BTK+
 * picks the best invisible char available in the current font. If you
 * set the invisible char to 0, then the user will get no feedback
 * at all; there will be no text on the screen as they type.
 **/
void
btk_entry_set_invisible_char (BtkEntry *entry,
                              gunichar  ch)
{
  BtkEntryPrivate *priv;

  g_return_if_fail (BTK_IS_ENTRY (entry));

  priv = BTK_ENTRY_GET_PRIVATE (entry);

  if (!priv->invisible_char_set)
    {
      priv->invisible_char_set = TRUE;
      g_object_notify (G_OBJECT (entry), "invisible-char-set");
    }

  if (ch == entry->invisible_char)
    return;

  entry->invisible_char = ch;
  g_object_notify (G_OBJECT (entry), "invisible-char");
  btk_entry_recompute (entry);  
}

/**
 * btk_entry_get_invisible_char:
 * @entry: a #BtkEntry
 *
 * Retrieves the character displayed in place of the real characters
 * for entries with visibility set to false. See btk_entry_set_invisible_char().
 *
 * Return value: the current invisible char, or 0, if the entry does not
 *               show invisible text at all. 
 **/
gunichar
btk_entry_get_invisible_char (BtkEntry *entry)
{
  g_return_val_if_fail (BTK_IS_ENTRY (entry), 0);

  return entry->invisible_char;
}

/**
 * btk_entry_unset_invisible_char:
 * @entry: a #BtkEntry
 *
 * Unsets the invisible char previously set with
 * btk_entry_set_invisible_char(). So that the
 * default invisible char is used again.
 *
 * Since: 2.16
 **/
void
btk_entry_unset_invisible_char (BtkEntry *entry)
{
  BtkEntryPrivate *priv;
  gunichar ch;

  g_return_if_fail (BTK_IS_ENTRY (entry));

  priv = BTK_ENTRY_GET_PRIVATE (entry);

  if (!priv->invisible_char_set)
    return;

  priv->invisible_char_set = FALSE;
  ch = find_invisible_char (BTK_WIDGET (entry));

  if (entry->invisible_char != ch)
    {
      entry->invisible_char = ch;
      g_object_notify (G_OBJECT (entry), "invisible-char");
    }

  g_object_notify (G_OBJECT (entry), "invisible-char-set");
  btk_entry_recompute (entry);
}

/**
 * btk_entry_set_editable:
 * @entry: a #BtkEntry
 * @editable: %TRUE if the user is allowed to edit the text
 *   in the widget
 *
 * Determines if the user can edit the text in the editable
 * widget or not. 
 *
 * Deprecated: 2.0: Use btk_editable_set_editable() instead.
 */
void
btk_entry_set_editable (BtkEntry *entry,
			gboolean  editable)
{
  g_return_if_fail (BTK_IS_ENTRY (entry));

  btk_editable_set_editable (BTK_EDITABLE (entry), editable);
}

/**
 * btk_entry_set_overwrite_mode:
 * @entry: a #BtkEntry
 * @overwrite: new value
 * 
 * Sets whether the text is overwritten when typing in the #BtkEntry.
 *
 * Since: 2.14
 **/
void
btk_entry_set_overwrite_mode (BtkEntry *entry,
                              gboolean  overwrite)
{
  g_return_if_fail (BTK_IS_ENTRY (entry));
  
  if (entry->overwrite_mode == overwrite) 
    return;
  
  btk_entry_toggle_overwrite (entry);

  g_object_notify (G_OBJECT (entry), "overwrite-mode");
}

/**
 * btk_entry_get_overwrite_mode:
 * @entry: a #BtkEntry
 * 
 * Gets the value set by btk_entry_set_overwrite_mode().
 * 
 * Return value: whether the text is overwritten when typing.
 *
 * Since: 2.14
 **/
gboolean
btk_entry_get_overwrite_mode (BtkEntry *entry)
{
  g_return_val_if_fail (BTK_IS_ENTRY (entry), FALSE);

  return entry->overwrite_mode;
}

/**
 * btk_entry_get_text:
 * @entry: a #BtkEntry
 *
 * Retrieves the contents of the entry widget.
 * See also btk_editable_get_chars().
 *
 * This is equivalent to:
 *
 * <informalexample><programlisting>
 * btk_entry_buffer_get_text (btk_entry_get_buffer (entry));
 * </programlisting></informalexample>
 *
 * Return value: a pointer to the contents of the widget as a
 *      string. This string points to internally allocated
 *      storage in the widget and must not be freed, modified or
 *      stored.
 **/
const gchar*
btk_entry_get_text (BtkEntry *entry)
{
  g_return_val_if_fail (BTK_IS_ENTRY (entry), NULL);
  return btk_entry_buffer_get_text (get_buffer (entry));
}

/**
 * btk_entry_select_rebunnyion:
 * @entry: a #BtkEntry
 * @start: the starting position
 * @end: the end position
 *
 * Selects a rebunnyion of text. The characters that are selected are 
 * those characters at positions from @start_pos up to, but not 
 * including @end_pos. If @end_pos is negative, then the the characters 
 * selected will be those characters from @start_pos to the end of 
 * the text. 
 *
 * Deprecated: 2.0: Use btk_editable_select_rebunnyion() instead.
 */
void       
btk_entry_select_rebunnyion  (BtkEntry       *entry,
			  gint            start,
			  gint            end)
{
  btk_editable_select_rebunnyion (BTK_EDITABLE (entry), start, end);
}

/**
 * btk_entry_set_max_length:
 * @entry: a #BtkEntry
 * @max: the maximum length of the entry, or 0 for no maximum.
 *   (other than the maximum length of entries.) The value passed in will
 *   be clamped to the range 0-65536.
 * 
 * Sets the maximum allowed length of the contents of the widget. If
 * the current contents are longer than the given length, then they
 * will be truncated to fit.
 *
 * This is equivalent to:
 *
 * <informalexample><programlisting>
 * btk_entry_buffer_set_max_length (btk_entry_get_buffer (entry), max);
 * </programlisting></informalexample>
 **/
void
btk_entry_set_max_length (BtkEntry     *entry,
                          gint          max)
{
  g_return_if_fail (BTK_IS_ENTRY (entry));
  btk_entry_buffer_set_max_length (get_buffer (entry), max);
}

/**
 * btk_entry_get_max_length:
 * @entry: a #BtkEntry
 *
 * Retrieves the maximum allowed length of the text in
 * @entry. See btk_entry_set_max_length().
 *
 * This is equivalent to:
 *
 * <informalexample><programlisting>
 * btk_entry_buffer_get_max_length (btk_entry_get_buffer (entry));
 * </programlisting></informalexample>
 *
 * Return value: the maximum allowed number of characters
 *               in #BtkEntry, or 0 if there is no maximum.
 **/
gint
btk_entry_get_max_length (BtkEntry *entry)
{
  g_return_val_if_fail (BTK_IS_ENTRY (entry), 0);
  return btk_entry_buffer_get_max_length (get_buffer (entry));
}

/**
 * btk_entry_get_text_length:
 * @entry: a #BtkEntry
 *
 * Retrieves the current length of the text in
 * @entry. 
 *
 * This is equivalent to:
 *
 * <informalexample><programlisting>
 * btk_entry_buffer_get_length (btk_entry_get_buffer (entry));
 * </programlisting></informalexample>
 *
 * Return value: the current number of characters
 *               in #BtkEntry, or 0 if there are none.
 *
 * Since: 2.14
 **/
guint16
btk_entry_get_text_length (BtkEntry *entry)
{
  g_return_val_if_fail (BTK_IS_ENTRY (entry), 0);
  return btk_entry_buffer_get_length (get_buffer (entry));
}

/**
 * btk_entry_set_activates_default:
 * @entry: a #BtkEntry
 * @setting: %TRUE to activate window's default widget on Enter keypress
 *
 * If @setting is %TRUE, pressing Enter in the @entry will activate the default
 * widget for the window containing the entry. This usually means that
 * the dialog box containing the entry will be closed, since the default
 * widget is usually one of the dialog buttons.
 *
 * (For experts: if @setting is %TRUE, the entry calls
 * btk_window_activate_default() on the window containing the entry, in
 * the default handler for the #BtkWidget::activate signal.)
 **/
void
btk_entry_set_activates_default (BtkEntry *entry,
                                 gboolean  setting)
{
  g_return_if_fail (BTK_IS_ENTRY (entry));
  setting = setting != FALSE;

  if (setting != entry->activates_default)
    {
      entry->activates_default = setting;
      g_object_notify (G_OBJECT (entry), "activates-default");
    }
}

/**
 * btk_entry_get_activates_default:
 * @entry: a #BtkEntry
 * 
 * Retrieves the value set by btk_entry_set_activates_default().
 * 
 * Return value: %TRUE if the entry will activate the default widget
 **/
gboolean
btk_entry_get_activates_default (BtkEntry *entry)
{
  g_return_val_if_fail (BTK_IS_ENTRY (entry), FALSE);

  return entry->activates_default;
}

/**
 * btk_entry_set_width_chars:
 * @entry: a #BtkEntry
 * @n_chars: width in chars
 *
 * Changes the size request of the entry to be about the right size
 * for @n_chars characters. Note that it changes the size
 * <emphasis>request</emphasis>, the size can still be affected by
 * how you pack the widget into containers. If @n_chars is -1, the
 * size reverts to the default entry size.
 **/
void
btk_entry_set_width_chars (BtkEntry *entry,
                           gint      n_chars)
{
  g_return_if_fail (BTK_IS_ENTRY (entry));

  if (entry->width_chars != n_chars)
    {
      entry->width_chars = n_chars;
      g_object_notify (G_OBJECT (entry), "width-chars");
      btk_widget_queue_resize (BTK_WIDGET (entry));
    }
}

/**
 * btk_entry_get_width_chars:
 * @entry: a #BtkEntry
 * 
 * Gets the value set by btk_entry_set_width_chars().
 * 
 * Return value: number of chars to request space for, or negative if unset
 **/
gint
btk_entry_get_width_chars (BtkEntry *entry)
{
  g_return_val_if_fail (BTK_IS_ENTRY (entry), 0);

  return entry->width_chars;
}

/**
 * btk_entry_set_has_frame:
 * @entry: a #BtkEntry
 * @setting: new value
 * 
 * Sets whether the entry has a beveled frame around it.
 **/
void
btk_entry_set_has_frame (BtkEntry *entry,
                         gboolean  setting)
{
  g_return_if_fail (BTK_IS_ENTRY (entry));

  setting = (setting != FALSE);

  if (entry->has_frame == setting)
    return;

  btk_widget_queue_resize (BTK_WIDGET (entry));
  entry->has_frame = setting;
  g_object_notify (G_OBJECT (entry), "has-frame");
}

/**
 * btk_entry_get_has_frame:
 * @entry: a #BtkEntry
 * 
 * Gets the value set by btk_entry_set_has_frame().
 * 
 * Return value: whether the entry has a beveled frame
 **/
gboolean
btk_entry_get_has_frame (BtkEntry *entry)
{
  g_return_val_if_fail (BTK_IS_ENTRY (entry), FALSE);

  return entry->has_frame;
}

/**
 * btk_entry_set_inner_border:
 * @entry: a #BtkEntry
 * @border: (allow-none): a #BtkBorder, or %NULL
 *
 * Sets %entry's inner-border property to %border, or clears it if %NULL
 * is passed. The inner-border is the area around the entry's text, but
 * inside its frame.
 *
 * If set, this property overrides the inner-border style property.
 * Overriding the style-provided border is useful when you want to do
 * in-place editing of some text in a canvas or list widget, where
 * pixel-exact positioning of the entry is important.
 *
 * Since: 2.10
 **/
void
btk_entry_set_inner_border (BtkEntry        *entry,
                            const BtkBorder *border)
{
  g_return_if_fail (BTK_IS_ENTRY (entry));

  btk_widget_queue_resize (BTK_WIDGET (entry));

  if (border)
    g_object_set_qdata_full (G_OBJECT (entry), quark_inner_border,
                             btk_border_copy (border),
                             (GDestroyNotify) btk_border_free);
  else
    g_object_set_qdata (G_OBJECT (entry), quark_inner_border, NULL);

  g_object_notify (G_OBJECT (entry), "inner-border");
}

/**
 * btk_entry_get_inner_border:
 * @entry: a #BtkEntry
 *
 * This function returns the entry's #BtkEntry:inner-border property. See
 * btk_entry_set_inner_border() for more information.
 *
 * Return value: (transfer none): the entry's #BtkBorder, or %NULL if none was set.
 *
 * Since: 2.10
 **/
const BtkBorder *
btk_entry_get_inner_border (BtkEntry *entry)
{
  g_return_val_if_fail (BTK_IS_ENTRY (entry), NULL);

  return g_object_get_qdata (G_OBJECT (entry), quark_inner_border);
}

/**
 * btk_entry_get_layout:
 * @entry: a #BtkEntry
 * 
 * Gets the #BangoLayout used to display the entry.
 * The layout is useful to e.g. convert text positions to
 * pixel positions, in combination with btk_entry_get_layout_offsets().
 * The returned layout is owned by the entry and must not be 
 * modified or freed by the caller.
 *
 * Keep in mind that the layout text may contain a preedit string, so
 * btk_entry_layout_index_to_text_index() and
 * btk_entry_text_index_to_layout_index() are needed to convert byte
 * indices in the layout to byte indices in the entry contents.
 *
 * Return value: (transfer none): the #BangoLayout for this entry
 **/
BangoLayout*
btk_entry_get_layout (BtkEntry *entry)
{
  BangoLayout *layout;
  
  g_return_val_if_fail (BTK_IS_ENTRY (entry), NULL);

  layout = btk_entry_ensure_layout (entry, TRUE);

  return layout;
}


/**
 * btk_entry_layout_index_to_text_index:
 * @entry: a #BtkEntry
 * @layout_index: byte index into the entry layout text
 * 
 * Converts from a position in the entry contents (returned
 * by btk_entry_get_text()) to a position in the
 * entry's #BangoLayout (returned by btk_entry_get_layout(),
 * with text retrieved via bango_layout_get_text()).
 * 
 * Return value: byte index into the entry contents
 **/
gint
btk_entry_layout_index_to_text_index (BtkEntry *entry,
                                      gint      layout_index)
{
  BangoLayout *layout;
  const gchar *text;
  gint cursor_index;
  
  g_return_val_if_fail (BTK_IS_ENTRY (entry), 0);

  layout = btk_entry_ensure_layout (entry, TRUE);
  text = bango_layout_get_text (layout);
  cursor_index = g_utf8_offset_to_pointer (text, entry->current_pos) - text;
  
  if (layout_index >= cursor_index && entry->preedit_length)
    {
      if (layout_index >= cursor_index + entry->preedit_length)
	layout_index -= entry->preedit_length;
      else
        layout_index = cursor_index;
    }

  return layout_index;
}

/**
 * btk_entry_text_index_to_layout_index:
 * @entry: a #BtkEntry
 * @text_index: byte index into the entry contents
 * 
 * Converts from a position in the entry's #BangoLayout (returned by
 * btk_entry_get_layout()) to a position in the entry contents
 * (returned by btk_entry_get_text()).
 * 
 * Return value: byte index into the entry layout text
 **/
gint
btk_entry_text_index_to_layout_index (BtkEntry *entry,
                                      gint      text_index)
{
  BangoLayout *layout;
  const gchar *text;
  gint cursor_index;
  g_return_val_if_fail (BTK_IS_ENTRY (entry), 0);

  layout = btk_entry_ensure_layout (entry, TRUE);
  text = bango_layout_get_text (layout);
  cursor_index = g_utf8_offset_to_pointer (text, entry->current_pos) - text;
  
  if (text_index > cursor_index)
    text_index += entry->preedit_length;

  return text_index;
}

/**
 * btk_entry_get_layout_offsets:
 * @entry: a #BtkEntry
 * @x: (out) (allow-none): location to store X offset of layout, or %NULL
 * @y: (out) (allow-none): location to store Y offset of layout, or %NULL
 *
 *
 * Obtains the position of the #BangoLayout used to render text
 * in the entry, in widget coordinates. Useful if you want to line
 * up the text in an entry with some other text, e.g. when using the
 * entry to implement editable cells in a sheet widget.
 *
 * Also useful to convert mouse events into coordinates inside the
 * #BangoLayout, e.g. to take some action if some part of the entry text
 * is clicked.
 * 
 * Note that as the user scrolls around in the entry the offsets will
 * change; you'll need to connect to the "notify::scroll-offset"
 * signal to track this. Remember when using the #BangoLayout
 * functions you need to convert to and from pixels using
 * BANGO_PIXELS() or #BANGO_SCALE.
 *
 * Keep in mind that the layout text may contain a preedit string, so
 * btk_entry_layout_index_to_text_index() and
 * btk_entry_text_index_to_layout_index() are needed to convert byte
 * indices in the layout to byte indices in the entry contents.
 **/
void
btk_entry_get_layout_offsets (BtkEntry *entry,
                              gint     *x,
                              gint     *y)
{
  gint text_area_x, text_area_y;
  
  g_return_if_fail (BTK_IS_ENTRY (entry));

  /* this gets coords relative to text area */
  get_layout_position (entry, x, y);

  /* convert to widget coords */
  btk_entry_get_text_area_size (entry, &text_area_x, &text_area_y, NULL, NULL);
  
  if (x)
    *x += text_area_x;

  if (y)
    *y += text_area_y;
}


/**
 * btk_entry_set_alignment:
 * @entry: a #BtkEntry
 * @xalign: The horizontal alignment, from 0 (left) to 1 (right).
 *          Reversed for RTL layouts
 * 
 * Sets the alignment for the contents of the entry. This controls
 * the horizontal positioning of the contents when the displayed
 * text is shorter than the width of the entry.
 *
 * Since: 2.4
 **/
void
btk_entry_set_alignment (BtkEntry *entry, gfloat xalign)
{
  BtkEntryPrivate *priv;
  
  g_return_if_fail (BTK_IS_ENTRY (entry));

  priv = BTK_ENTRY_GET_PRIVATE (entry);

  if (xalign < 0.0)
    xalign = 0.0;
  else if (xalign > 1.0)
    xalign = 1.0;

  if (xalign != priv->xalign)
    {
      priv->xalign = xalign;

      btk_entry_recompute (entry);

      g_object_notify (G_OBJECT (entry), "xalign");
    }
}

/**
 * btk_entry_get_alignment:
 * @entry: a #BtkEntry
 * 
 * Gets the value set by btk_entry_set_alignment().
 * 
 * Return value: the alignment
 *
 * Since: 2.4
 **/
gfloat
btk_entry_get_alignment (BtkEntry *entry)
{
  BtkEntryPrivate *priv;
  
  g_return_val_if_fail (BTK_IS_ENTRY (entry), 0.0);

  priv = BTK_ENTRY_GET_PRIVATE (entry);

  return priv->xalign;
}

/**
 * btk_entry_set_icon_from_pixbuf:
 * @entry: a #BtkEntry
 * @icon_pos: Icon position
 * @pixbuf: (allow-none): A #BdkPixbuf, or %NULL
 *
 * Sets the icon shown in the specified position using a pixbuf.
 *
 * If @pixbuf is %NULL, no icon will be shown in the specified position.
 *
 * Since: 2.16
 */
void
btk_entry_set_icon_from_pixbuf (BtkEntry             *entry,
                                BtkEntryIconPosition  icon_pos,
                                BdkPixbuf            *pixbuf)
{
  BtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_if_fail (BTK_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  priv = BTK_ENTRY_GET_PRIVATE (entry);

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (BTK_WIDGET (entry), icon_pos);

  g_object_freeze_notify (G_OBJECT (entry));

  if (pixbuf)
    g_object_ref (pixbuf);

  btk_entry_clear (entry, icon_pos);

  if (pixbuf)
    {
      icon_info->storage_type = BTK_IMAGE_PIXBUF;
      icon_info->pixbuf = pixbuf;

      if (icon_pos == BTK_ENTRY_ICON_PRIMARY)
        {
          g_object_notify (G_OBJECT (entry), "primary-icon-pixbuf");
          g_object_notify (G_OBJECT (entry), "primary-icon-storage-type");
        }
      else
        {
          g_object_notify (G_OBJECT (entry), "secondary-icon-pixbuf");
          g_object_notify (G_OBJECT (entry), "secondary-icon-storage-type");
        }

      if (btk_widget_get_mapped (BTK_WIDGET (entry)))
          bdk_window_show_unraised (icon_info->window);
    }

  btk_entry_ensure_pixbuf (entry, icon_pos);
  
  if (btk_widget_get_visible (BTK_WIDGET (entry)))
    btk_widget_queue_resize (BTK_WIDGET (entry));

  g_object_thaw_notify (G_OBJECT (entry));
}

/**
 * btk_entry_set_icon_from_stock:
 * @entry: A #BtkEntry
 * @icon_pos: Icon position
 * @stock_id: (allow-none): The name of the stock item, or %NULL
 *
 * Sets the icon shown in the entry at the specified position from
 * a stock image.
 *
 * If @stock_id is %NULL, no icon will be shown in the specified position.
 *
 * Since: 2.16
 */
void
btk_entry_set_icon_from_stock (BtkEntry             *entry,
                               BtkEntryIconPosition  icon_pos,
                               const gchar          *stock_id)
{
  BtkEntryPrivate *priv;
  EntryIconInfo *icon_info;
  gchar *new_id;

  g_return_if_fail (BTK_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  priv = BTK_ENTRY_GET_PRIVATE (entry);

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (BTK_WIDGET (entry), icon_pos);

  g_object_freeze_notify (G_OBJECT (entry));

  btk_widget_ensure_style (BTK_WIDGET (entry));

  /* need to dup before clearing */
  new_id = g_strdup (stock_id);

  btk_entry_clear (entry, icon_pos);

  if (new_id != NULL)
    {
      icon_info->storage_type = BTK_IMAGE_STOCK;
      icon_info->stock_id = new_id;

      if (icon_pos == BTK_ENTRY_ICON_PRIMARY)
        {
          g_object_notify (G_OBJECT (entry), "primary-icon-stock");
          g_object_notify (G_OBJECT (entry), "primary-icon-storage-type");
        }
      else
        {
          g_object_notify (G_OBJECT (entry), "secondary-icon-stock");
          g_object_notify (G_OBJECT (entry), "secondary-icon-storage-type");
        }

      if (btk_widget_get_mapped (BTK_WIDGET (entry)))
          bdk_window_show_unraised (icon_info->window);
    }

  btk_entry_ensure_pixbuf (entry, icon_pos);

  if (btk_widget_get_visible (BTK_WIDGET (entry)))
    btk_widget_queue_resize (BTK_WIDGET (entry));

  g_object_thaw_notify (G_OBJECT (entry));
}

/**
 * btk_entry_set_icon_from_icon_name:
 * @entry: A #BtkEntry
 * @icon_pos: The position at which to set the icon
 * @icon_name: (allow-none): An icon name, or %NULL
 *
 * Sets the icon shown in the entry at the specified position
 * from the current icon theme.
 *
 * If the icon name isn't known, a "broken image" icon will be displayed
 * instead.
 *
 * If @icon_name is %NULL, no icon will be shown in the specified position.
 *
 * Since: 2.16
 */
void
btk_entry_set_icon_from_icon_name (BtkEntry             *entry,
                                   BtkEntryIconPosition  icon_pos,
                                   const gchar          *icon_name)
{
  BtkEntryPrivate *priv;
  EntryIconInfo *icon_info;
  gchar *new_name;

  g_return_if_fail (BTK_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  priv = BTK_ENTRY_GET_PRIVATE (entry);

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (BTK_WIDGET (entry), icon_pos);

  g_object_freeze_notify (G_OBJECT (entry));

  btk_widget_ensure_style (BTK_WIDGET (entry));

  /* need to dup before clearing */
  new_name = g_strdup (icon_name);

  btk_entry_clear (entry, icon_pos);

  if (new_name != NULL)
    {
      icon_info->storage_type = BTK_IMAGE_ICON_NAME;
      icon_info->icon_name = new_name;

      if (icon_pos == BTK_ENTRY_ICON_PRIMARY)
        {
          g_object_notify (G_OBJECT (entry), "primary-icon-name");
          g_object_notify (G_OBJECT (entry), "primary-icon-storage-type");
        }
      else
        {
          g_object_notify (G_OBJECT (entry), "secondary-icon-name");
          g_object_notify (G_OBJECT (entry), "secondary-icon-storage-type");
        }

      if (btk_widget_get_mapped (BTK_WIDGET (entry)))
          bdk_window_show_unraised (icon_info->window);
    }

  btk_entry_ensure_pixbuf (entry, icon_pos);

  if (btk_widget_get_visible (BTK_WIDGET (entry)))
    btk_widget_queue_resize (BTK_WIDGET (entry));

  g_object_thaw_notify (G_OBJECT (entry));
}

/**
 * btk_entry_set_icon_from_gicon:
 * @entry: A #BtkEntry
 * @icon_pos: The position at which to set the icon
 * @icon: (allow-none): The icon to set, or %NULL
 *
 * Sets the icon shown in the entry at the specified position
 * from the current icon theme.
 * If the icon isn't known, a "broken image" icon will be displayed
 * instead.
 *
 * If @icon is %NULL, no icon will be shown in the specified position.
 *
 * Since: 2.16
 */
void
btk_entry_set_icon_from_gicon (BtkEntry             *entry,
                               BtkEntryIconPosition  icon_pos,
                               GIcon                *icon)
{
  BtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_if_fail (BTK_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  priv = BTK_ENTRY_GET_PRIVATE (entry);

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (BTK_WIDGET (entry), icon_pos);

  g_object_freeze_notify (G_OBJECT (entry));

  /* need to ref before clearing */
  if (icon)
    g_object_ref (icon);

  btk_entry_clear (entry, icon_pos);

  if (icon)
    {
      icon_info->storage_type = BTK_IMAGE_GICON;
      icon_info->gicon = icon;

      if (icon_pos == BTK_ENTRY_ICON_PRIMARY)
        {
          g_object_notify (G_OBJECT (entry), "primary-icon-gicon");
          g_object_notify (G_OBJECT (entry), "primary-icon-storage-type");
        }
      else
        {
          g_object_notify (G_OBJECT (entry), "secondary-icon-gicon");
          g_object_notify (G_OBJECT (entry), "secondary-icon-storage-type");
        }

      if (btk_widget_get_mapped (BTK_WIDGET (entry)))
          bdk_window_show_unraised (icon_info->window);
    }

  btk_entry_ensure_pixbuf (entry, icon_pos);

  if (btk_widget_get_visible (BTK_WIDGET (entry)))
    btk_widget_queue_resize (BTK_WIDGET (entry));

  g_object_thaw_notify (G_OBJECT (entry));
}

/**
 * btk_entry_set_icon_activatable:
 * @entry: A #BtkEntry
 * @icon_pos: Icon position
 * @activatable: %TRUE if the icon should be activatable
 *
 * Sets whether the icon is activatable.
 *
 * Since: 2.16
 */
void
btk_entry_set_icon_activatable (BtkEntry             *entry,
                                BtkEntryIconPosition  icon_pos,
                                gboolean              activatable)
{
  BtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_if_fail (BTK_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  priv = BTK_ENTRY_GET_PRIVATE (entry);

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (BTK_WIDGET (entry), icon_pos);

  activatable = activatable != FALSE;

  if (icon_info->nonactivatable != !activatable)
    {
      icon_info->nonactivatable = !activatable;

      if (btk_widget_get_realized (BTK_WIDGET (entry)))
        update_cursors (BTK_WIDGET (entry));

      g_object_notify (G_OBJECT (entry),
                       icon_pos == BTK_ENTRY_ICON_PRIMARY ? "primary-icon-activatable" : "secondary-icon-activatable");
    }
}

/**
 * btk_entry_get_icon_activatable:
 * @entry: a #BtkEntry
 * @icon_pos: Icon position
 *
 * Returns whether the icon is activatable.
 *
 * Returns: %TRUE if the icon is activatable.
 *
 * Since: 2.16
 */
gboolean
btk_entry_get_icon_activatable (BtkEntry             *entry,
                                BtkEntryIconPosition  icon_pos)
{
  BtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_val_if_fail (BTK_IS_ENTRY (entry), FALSE);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), FALSE);

  priv = BTK_ENTRY_GET_PRIVATE (entry);
  icon_info = priv->icons[icon_pos];

  return (icon_info != NULL && !icon_info->nonactivatable);
}

/**
 * btk_entry_get_icon_pixbuf:
 * @entry: A #BtkEntry
 * @icon_pos: Icon position
 *
 * Retrieves the image used for the icon.
 *
 * Unlike the other methods of setting and getting icon data, this
 * method will work regardless of whether the icon was set using a
 * #BdkPixbuf, a #GIcon, a stock item, or an icon name.
 *
 * Returns: (transfer none): A #BdkPixbuf, or %NULL if no icon is
 *     set for this position.
 *
 * Since: 2.16
 */
BdkPixbuf *
btk_entry_get_icon_pixbuf (BtkEntry             *entry,
                           BtkEntryIconPosition  icon_pos)
{
  BtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_val_if_fail (BTK_IS_ENTRY (entry), NULL);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), NULL);

  priv = BTK_ENTRY_GET_PRIVATE (entry);
  icon_info = priv->icons[icon_pos];

  if (!icon_info)
    return NULL;

  btk_entry_ensure_pixbuf (entry, icon_pos);

  return icon_info->pixbuf;
}

/**
 * btk_entry_get_icon_gicon:
 * @entry: A #BtkEntry
 * @icon_pos: Icon position
 *
 * Retrieves the #GIcon used for the icon, or %NULL if there is
 * no icon or if the icon was set by some other method (e.g., by
 * stock, pixbuf, or icon name).
 *
 * Returns: (transfer none): A #GIcon, or %NULL if no icon is set
 *     or if the icon is not a #GIcon
 *
 * Since: 2.16
 */
GIcon *
btk_entry_get_icon_gicon (BtkEntry             *entry,
                          BtkEntryIconPosition  icon_pos)
{
  BtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_val_if_fail (BTK_IS_ENTRY (entry), NULL);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), NULL);

  priv = BTK_ENTRY_GET_PRIVATE (entry);
  icon_info = priv->icons[icon_pos];

  if (!icon_info)
    return NULL;

  return icon_info->storage_type == BTK_IMAGE_GICON ? icon_info->gicon : NULL;
}

/**
 * btk_entry_get_icon_stock:
 * @entry: A #BtkEntry
 * @icon_pos: Icon position
 *
 * Retrieves the stock id used for the icon, or %NULL if there is
 * no icon or if the icon was set by some other method (e.g., by
 * pixbuf, icon name or gicon).
 *
 * Returns: A stock id, or %NULL if no icon is set or if the icon
 *          wasn't set from a stock id
 *
 * Since: 2.16
 */
const gchar *
btk_entry_get_icon_stock (BtkEntry             *entry,
                          BtkEntryIconPosition  icon_pos)
{
  BtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_val_if_fail (BTK_IS_ENTRY (entry), NULL);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), NULL);

  priv = BTK_ENTRY_GET_PRIVATE (entry);
  icon_info = priv->icons[icon_pos];

  if (!icon_info)
    return NULL;

  return icon_info->storage_type == BTK_IMAGE_STOCK ? icon_info->stock_id : NULL;
}

/**
 * btk_entry_get_icon_name:
 * @entry: A #BtkEntry
 * @icon_pos: Icon position
 *
 * Retrieves the icon name used for the icon, or %NULL if there is
 * no icon or if the icon was set by some other method (e.g., by
 * pixbuf, stock or gicon).
 *
 * Returns: An icon name, or %NULL if no icon is set or if the icon
 *          wasn't set from an icon name
 *
 * Since: 2.16
 */
const gchar *
btk_entry_get_icon_name (BtkEntry             *entry,
                         BtkEntryIconPosition  icon_pos)
{
  BtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_val_if_fail (BTK_IS_ENTRY (entry), NULL);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), NULL);

  priv = BTK_ENTRY_GET_PRIVATE (entry);
  icon_info = priv->icons[icon_pos];

  if (!icon_info)
    return NULL;

  return icon_info->storage_type == BTK_IMAGE_ICON_NAME ? icon_info->icon_name : NULL;
}

/**
 * btk_entry_set_icon_sensitive:
 * @entry: A #BtkEntry
 * @icon_pos: Icon position
 * @sensitive: Specifies whether the icon should appear
 *             sensitive or insensitive
 *
 * Sets the sensitivity for the specified icon.
 *
 * Since: 2.16
 */
void
btk_entry_set_icon_sensitive (BtkEntry             *entry,
                              BtkEntryIconPosition  icon_pos,
                              gboolean              sensitive)
{
  BtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_if_fail (BTK_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  priv = BTK_ENTRY_GET_PRIVATE (entry);

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (BTK_WIDGET (entry), icon_pos);

  if (icon_info->insensitive != !sensitive)
    {
      icon_info->insensitive = !sensitive;

      icon_info->pressed = FALSE;
      icon_info->prelight = FALSE;

      if (btk_widget_get_realized (BTK_WIDGET (entry)))
        update_cursors (BTK_WIDGET (entry));

      btk_widget_queue_draw (BTK_WIDGET (entry));

      g_object_notify (G_OBJECT (entry),
                       icon_pos == BTK_ENTRY_ICON_PRIMARY ? "primary-icon-sensitive" : "secondary-icon-sensitive");
    }
}

/**
 * btk_entry_get_icon_sensitive:
 * @entry: a #BtkEntry
 * @icon_pos: Icon position
 *
 * Returns whether the icon appears sensitive or insensitive.
 *
 * Returns: %TRUE if the icon is sensitive.
 *
 * Since: 2.16
 */
gboolean
btk_entry_get_icon_sensitive (BtkEntry             *entry,
                              BtkEntryIconPosition  icon_pos)
{
  BtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_val_if_fail (BTK_IS_ENTRY (entry), TRUE);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), TRUE);

  priv = BTK_ENTRY_GET_PRIVATE (entry);
  icon_info = priv->icons[icon_pos];

  return (!icon_info || !icon_info->insensitive);

}

/**
 * btk_entry_get_icon_storage_type:
 * @entry: a #BtkEntry
 * @icon_pos: Icon position
 *
 * Gets the type of representation being used by the icon
 * to store image data. If the icon has no image data,
 * the return value will be %BTK_IMAGE_EMPTY.
 *
 * Return value: image representation being used
 *
 * Since: 2.16
 **/
BtkImageType
btk_entry_get_icon_storage_type (BtkEntry             *entry,
                                 BtkEntryIconPosition  icon_pos)
{
  BtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_val_if_fail (BTK_IS_ENTRY (entry), BTK_IMAGE_EMPTY);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), BTK_IMAGE_EMPTY);

  priv = BTK_ENTRY_GET_PRIVATE (entry);
  icon_info = priv->icons[icon_pos];

  if (!icon_info)
    return BTK_IMAGE_EMPTY;

  return icon_info->storage_type;
}

/**
 * btk_entry_get_icon_at_pos:
 * @entry: a #BtkEntry
 * @x: the x coordinate of the position to find
 * @y: the y coordinate of the position to find
 *
 * Finds the icon at the given position and return its index.
 * If @x, @y doesn't lie inside an icon, -1 is returned.
 * This function is intended for use in a #BtkWidget::query-tooltip
 * signal handler.
 *
 * Returns: the index of the icon at the given position, or -1
 *
 * Since: 2.16
 */
gint
btk_entry_get_icon_at_pos (BtkEntry *entry,
                           gint      x,
                           gint      y)
{
  BtkAllocation primary;
  BtkAllocation secondary;

  g_return_val_if_fail (BTK_IS_ENTRY (entry), -1);

  get_icon_allocations (entry, &primary, &secondary);

  if (primary.x <= x && x < primary.x + primary.width &&
      primary.y <= y && y < primary.y + primary.height)
    return BTK_ENTRY_ICON_PRIMARY;

  if (secondary.x <= x && x < secondary.x + secondary.width &&
      secondary.y <= y && y < secondary.y + secondary.height)
    return BTK_ENTRY_ICON_SECONDARY;

  return -1;
}

/**
 * btk_entry_set_icon_drag_source:
 * @entry: a #BtkIconEntry
 * @icon_pos: icon position
 * @target_list: the targets (data formats) in which the data can be provided
 * @actions: a bitmask of the allowed drag actions
 *
 * Sets up the icon at the given position so that BTK+ will start a drag
 * operation when the user clicks and drags the icon.
 *
 * To handle the drag operation, you need to connect to the usual
 * #BtkWidget::drag-data-get (or possibly #BtkWidget::drag-data-delete)
 * signal, and use btk_entry_get_current_icon_drag_source() in
 * your signal handler to find out if the drag was started from
 * an icon.
 *
 * By default, BTK+ uses the icon as the drag icon. You can use the 
 * #BtkWidget::drag-begin signal to set a different icon. Note that you 
 * have to use g_signal_connect_after() to ensure that your signal handler
 * gets executed after the default handler.
 *
 * Since: 2.16
 */
void
btk_entry_set_icon_drag_source (BtkEntry             *entry,
                                BtkEntryIconPosition  icon_pos,
                                BtkTargetList        *target_list,
                                BdkDragAction         actions)
{
  BtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_if_fail (BTK_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  priv = BTK_ENTRY_GET_PRIVATE (entry);

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (BTK_WIDGET (entry), icon_pos);

  if (icon_info->target_list)
    btk_target_list_unref (icon_info->target_list);
  icon_info->target_list = target_list;
  if (icon_info->target_list)
    btk_target_list_ref (icon_info->target_list);

  icon_info->actions = actions;
}

/**
 * btk_entry_get_current_icon_drag_source:
 * @entry: a #BtkIconEntry
 *
 * Returns the index of the icon which is the source of the current
 * DND operation, or -1.
 *
 * This function is meant to be used in a #BtkWidget::drag-data-get
 * callback.
 *
 * Returns: index of the icon which is the source of the current
 *          DND operation, or -1.
 *
 * Since: 2.16
 */
gint
btk_entry_get_current_icon_drag_source (BtkEntry *entry)
{
  BtkEntryPrivate *priv;
  EntryIconInfo *icon_info = NULL;
  gint i;

  g_return_val_if_fail (BTK_IS_ENTRY (entry), -1);

  priv = BTK_ENTRY_GET_PRIVATE (entry);

  for (i = 0; i < MAX_ICONS; i++)
    {
      if ((icon_info = priv->icons[i]))
        {
          if (icon_info->in_drag)
            return i;
        }
    }

  return -1;
}

/**
 * btk_entry_get_icon_window:
 * @entry: A #BtkEntry
 * @icon_pos: Icon position
 *
 * Returns the #BdkWindow which contains the entry's icon at
 * @icon_pos. This function is useful when drawing something to the
 * entry in an expose-event callback because it enables the callback
 * to distinguish between the text window and entry's icon windows.
 *
 * See also btk_entry_get_text_window().
 *
 * Note that BTK+ 3 does not have this function anymore; it has
 * been replaced by btk_entry_get_icon_area().
 *
 * Return value: (transfer none): the entry's icon window at @icon_pos.
 *
 * Since: 2.20
 */
BdkWindow  *
btk_entry_get_icon_window (BtkEntry             *entry,
                           BtkEntryIconPosition  icon_pos)
{
  BtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_val_if_fail (BTK_IS_ENTRY (entry), NULL);

  priv = BTK_ENTRY_GET_PRIVATE (entry);

  icon_info = priv->icons[icon_pos];

  if (icon_info)
    return icon_info->window;

  return NULL;
}

static void
ensure_has_tooltip (BtkEntry *entry)
{
  gchar *text = btk_widget_get_tooltip_text (BTK_WIDGET (entry));
  gboolean has_tooltip = text != NULL;

  if (!has_tooltip)
    {
      BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);
      int i;

      for (i = 0; i < MAX_ICONS; i++)
        {
          EntryIconInfo *icon_info = priv->icons[i];

          if (icon_info != NULL && icon_info->tooltip != NULL)
            {
              has_tooltip = TRUE;
              break;
            }
        }
    }
  else
    {
      g_free (text);
    }

  btk_widget_set_has_tooltip (BTK_WIDGET (entry), has_tooltip);
}

/**
 * btk_entry_get_icon_tooltip_text:
 * @entry: a #BtkEntry
 * @icon_pos: the icon position
 *
 * Gets the contents of the tooltip on the icon at the specified 
 * position in @entry.
 * 
 * Returns: the tooltip text, or %NULL. Free the returned string
 *     with g_free() when done.
 * 
 * Since: 2.16
 */
gchar *
btk_entry_get_icon_tooltip_text (BtkEntry             *entry,
                                 BtkEntryIconPosition  icon_pos)
{
  BtkEntryPrivate *priv;
  EntryIconInfo *icon_info;
  gchar *text = NULL;

  g_return_val_if_fail (BTK_IS_ENTRY (entry), NULL);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), NULL);

  priv = BTK_ENTRY_GET_PRIVATE (entry);
  icon_info = priv->icons[icon_pos];

  if (!icon_info)
    return NULL;
 
  if (icon_info->tooltip && 
      !bango_parse_markup (icon_info->tooltip, -1, 0, NULL, &text, NULL, NULL))
    g_assert (NULL == text); /* text should still be NULL in case of markup errors */

  return text;
}

/**
 * btk_entry_set_icon_tooltip_text:
 * @entry: a #BtkEntry
 * @icon_pos: the icon position
 * @tooltip: (allow-none): the contents of the tooltip for the icon, or %NULL
 *
 * Sets @tooltip as the contents of the tooltip for the icon
 * at the specified position.
 *
 * Use %NULL for @tooltip to remove an existing tooltip.
 *
 * See also btk_widget_set_tooltip_text() and 
 * btk_entry_set_icon_tooltip_markup().
 *
 * If you unset the widget tooltip via btk_widget_set_tooltip_text() or
 * btk_widget_set_tooltip_markup(), this sets BtkWidget:has-tooltip to %FALSE,
 * which suppresses icon tooltips too. You can resolve this by then calling
 * btk_widget_set_has_tooltip() to set BtkWidget:has-tooltip back to %TRUE, or
 * setting at least one non-empty tooltip on any icon achieves the same result.
 *
 * Since: 2.16
 */
void
btk_entry_set_icon_tooltip_text (BtkEntry             *entry,
                                 BtkEntryIconPosition  icon_pos,
                                 const gchar          *tooltip)
{
  BtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_if_fail (BTK_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  priv = BTK_ENTRY_GET_PRIVATE (entry);

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (BTK_WIDGET (entry), icon_pos);

  if (icon_info->tooltip)
    g_free (icon_info->tooltip);

  /* Treat an empty string as a NULL string,
   * because an empty string would be useless for a tooltip:
   */
  if (tooltip && tooltip[0] == '\0')
    tooltip = NULL;

  icon_info->tooltip = tooltip ? g_markup_escape_text (tooltip, -1) : NULL;

  ensure_has_tooltip (entry);
}

/**
 * btk_entry_get_icon_tooltip_markup:
 * @entry: a #BtkEntry
 * @icon_pos: the icon position
 *
 * Gets the contents of the tooltip on the icon at the specified 
 * position in @entry.
 * 
 * Returns: the tooltip text, or %NULL. Free the returned string
 *     with g_free() when done.
 * 
 * Since: 2.16
 */
gchar *
btk_entry_get_icon_tooltip_markup (BtkEntry             *entry,
                                   BtkEntryIconPosition  icon_pos)
{
  BtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_val_if_fail (BTK_IS_ENTRY (entry), NULL);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), NULL);

  priv = BTK_ENTRY_GET_PRIVATE (entry);
  icon_info = priv->icons[icon_pos];

  if (!icon_info)
    return NULL;
 
  return g_strdup (icon_info->tooltip);
}

/**
 * btk_entry_set_icon_tooltip_markup:
 * @entry: a #BtkEntry
 * @icon_pos: the icon position
 * @tooltip: (allow-none): the contents of the tooltip for the icon, or %NULL
 *
 * Sets @tooltip as the contents of the tooltip for the icon at
 * the specified position. @tooltip is assumed to be marked up with
 * the <link linkend="BangoMarkupFormat">Bango text markup language</link>.
 *
 * Use %NULL for @tooltip to remove an existing tooltip.
 *
 * See also btk_widget_set_tooltip_markup() and 
 * btk_entry_set_icon_tooltip_text().
 *
 * Since: 2.16
 */
void
btk_entry_set_icon_tooltip_markup (BtkEntry             *entry,
                                   BtkEntryIconPosition  icon_pos,
                                   const gchar          *tooltip)
{
  BtkEntryPrivate *priv;
  EntryIconInfo *icon_info;

  g_return_if_fail (BTK_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  priv = BTK_ENTRY_GET_PRIVATE (entry);

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (BTK_WIDGET (entry), icon_pos);

  if (icon_info->tooltip)
    g_free (icon_info->tooltip);

  /* Treat an empty string as a NULL string,
   * because an empty string would be useless for a tooltip:
   */
  if (tooltip && tooltip[0] == '\0')
    tooltip = NULL;

  icon_info->tooltip = g_strdup (tooltip);

  ensure_has_tooltip (entry);
}

static gboolean
btk_entry_query_tooltip (BtkWidget  *widget,
                         gint        x,
                         gint        y,
                         gboolean    keyboard_tip,
                         BtkTooltip *tooltip)
{
  BtkEntry *entry;
  BtkEntryPrivate *priv;
  EntryIconInfo *icon_info;
  gint icon_pos;

  entry = BTK_ENTRY (widget);
  priv = BTK_ENTRY_GET_PRIVATE (entry);

  if (!keyboard_tip)
    {
      icon_pos = btk_entry_get_icon_at_pos (entry, x, y);
      if (icon_pos != -1)
        {
          if ((icon_info = priv->icons[icon_pos]) != NULL)
            {
              if (icon_info->tooltip)
                {
                  btk_tooltip_set_markup (tooltip, icon_info->tooltip);
                  return TRUE;
                }

              return FALSE;
            }
        }
    }

  return BTK_WIDGET_CLASS (btk_entry_parent_class)->query_tooltip (widget,
                                                                   x, y,
                                                                   keyboard_tip,
                                                                   tooltip);
}


/* Quick hack of a popup menu
 */
static void
activate_cb (BtkWidget *menuitem,
	     BtkEntry  *entry)
{
  const gchar *signal = g_object_get_data (G_OBJECT (menuitem), "btk-signal");
  g_signal_emit_by_name (entry, signal);
}


static gboolean
btk_entry_mnemonic_activate (BtkWidget *widget,
			     gboolean   group_cycling)
{
  btk_widget_grab_focus (widget);
  return TRUE;
}

static void
append_action_signal (BtkEntry     *entry,
		      BtkWidget    *menu,
		      const gchar  *stock_id,
		      const gchar  *signal,
                      gboolean      sensitive)
{
  BtkWidget *menuitem = btk_image_menu_item_new_from_stock (stock_id, NULL);

  g_object_set_data (G_OBJECT (menuitem), I_("btk-signal"), (char *)signal);
  g_signal_connect (menuitem, "activate",
		    G_CALLBACK (activate_cb), entry);

  btk_widget_set_sensitive (menuitem, sensitive);
  
  btk_widget_show (menuitem);
  btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
}
	
static void
popup_menu_detach (BtkWidget *attach_widget,
		   BtkMenu   *menu)
{
  BTK_ENTRY (attach_widget)->popup_menu = NULL;
}

static void
popup_position_func (BtkMenu   *menu,
                     gint      *x,
                     gint      *y,
                     gboolean  *push_in,
                     gpointer	user_data)
{
  BtkEntry *entry = BTK_ENTRY (user_data);
  BtkWidget *widget = BTK_WIDGET (entry);
  BdkScreen *screen;
  BtkRequisition menu_req;
  BdkRectangle monitor;
  BtkBorder inner_border;
  gint monitor_num, strong_x, height;
 
  g_return_if_fail (btk_widget_get_realized (widget));

  bdk_window_get_origin (entry->text_area, x, y);

  screen = btk_widget_get_screen (widget);
  monitor_num = bdk_screen_get_monitor_at_window (screen, entry->text_area);
  if (monitor_num < 0)
    monitor_num = 0;
  btk_menu_set_monitor (menu, monitor_num);

  bdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);
  btk_widget_size_request (entry->popup_menu, &menu_req);
  height = bdk_window_get_height (entry->text_area);
  btk_entry_get_cursor_locations (entry, CURSOR_STANDARD, &strong_x, NULL);
  _btk_entry_effective_inner_border (entry, &inner_border);

  *x += inner_border.left + strong_x - entry->scroll_offset;
  if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL)
    *x -= menu_req.width;

  if ((*y + height + menu_req.height) <= monitor.y + monitor.height)
    *y += height;
  else if ((*y - menu_req.height) >= monitor.y)
    *y -= menu_req.height;
  else if (monitor.y + monitor.height - (*y + height) > *y)
    *y += height;
  else
    *y -= menu_req.height;

  *push_in = FALSE;
}

static void
unichar_chosen_func (const char *text,
                     gpointer    data)
{
  BtkEntry *entry = BTK_ENTRY (data);

  if (entry->editable)
    btk_entry_enter_text (entry, text);
}

typedef struct
{
  BtkEntry *entry;
  gint button;
  guint time;
} PopupInfo;

static void
popup_targets_received (BtkClipboard     *clipboard,
			BtkSelectionData *data,
			gpointer          user_data)
{
  PopupInfo *info = user_data;
  BtkEntry *entry = info->entry;
  
  if (btk_widget_get_realized (BTK_WIDGET (entry)))
    {
      DisplayMode mode;
      gboolean clipboard_contains_text;
      BtkWidget *menuitem;
      BtkWidget *submenu;
      gboolean show_input_method_menu;
      gboolean show_unicode_menu;
      
      clipboard_contains_text = btk_selection_data_targets_include_text (data);
      if (entry->popup_menu)
	btk_widget_destroy (entry->popup_menu);
      
      entry->popup_menu = btk_menu_new ();
      
      btk_menu_attach_to_widget (BTK_MENU (entry->popup_menu),
				 BTK_WIDGET (entry),
				 popup_menu_detach);
      
      mode = btk_entry_get_display_mode (entry);
      append_action_signal (entry, entry->popup_menu, BTK_STOCK_CUT, "cut-clipboard",
			    entry->editable && mode == DISPLAY_NORMAL &&
			    entry->current_pos != entry->selection_bound);

      append_action_signal (entry, entry->popup_menu, BTK_STOCK_COPY, "copy-clipboard",
                            mode == DISPLAY_NORMAL &&
                            entry->current_pos != entry->selection_bound);

      append_action_signal (entry, entry->popup_menu, BTK_STOCK_PASTE, "paste-clipboard",
			    entry->editable && clipboard_contains_text);

      menuitem = btk_image_menu_item_new_from_stock (BTK_STOCK_DELETE, NULL);
      btk_widget_set_sensitive (menuitem, entry->editable && entry->current_pos != entry->selection_bound);
      g_signal_connect_swapped (menuitem, "activate",
			        G_CALLBACK (btk_entry_delete_cb), entry);
      btk_widget_show (menuitem);
      btk_menu_shell_append (BTK_MENU_SHELL (entry->popup_menu), menuitem);

      menuitem = btk_separator_menu_item_new ();
      btk_widget_show (menuitem);
      btk_menu_shell_append (BTK_MENU_SHELL (entry->popup_menu), menuitem);
      
      menuitem = btk_image_menu_item_new_from_stock (BTK_STOCK_SELECT_ALL, NULL);
      g_signal_connect_swapped (menuitem, "activate",
			        G_CALLBACK (btk_entry_select_all), entry);
      btk_widget_show (menuitem);
      btk_menu_shell_append (BTK_MENU_SHELL (entry->popup_menu), menuitem);
      
      g_object_get (btk_widget_get_settings (BTK_WIDGET (entry)),
                    "btk-show-input-method-menu", &show_input_method_menu,
                    "btk-show-unicode-menu", &show_unicode_menu,
                    NULL);

      if (show_input_method_menu || show_unicode_menu)
        {
          menuitem = btk_separator_menu_item_new ();
          btk_widget_show (menuitem);
          btk_menu_shell_append (BTK_MENU_SHELL (entry->popup_menu), menuitem);
        }
      
      if (show_input_method_menu)
        {
          menuitem = btk_menu_item_new_with_mnemonic (_("Input _Methods"));
          btk_widget_set_sensitive (menuitem, entry->editable);      
          btk_widget_show (menuitem);
          submenu = btk_menu_new ();
          btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), submenu);
          
          btk_menu_shell_append (BTK_MENU_SHELL (entry->popup_menu), menuitem);
      
          btk_im_multicontext_append_menuitems (BTK_IM_MULTICONTEXT (entry->im_context),
                                                BTK_MENU_SHELL (submenu));
        }
      
      if (show_unicode_menu)
        {
          menuitem = btk_menu_item_new_with_mnemonic (_("_Insert Unicode Control Character"));
          btk_widget_set_sensitive (menuitem, entry->editable);      
          btk_widget_show (menuitem);
          
          submenu = btk_menu_new ();
          btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), submenu);
          btk_menu_shell_append (BTK_MENU_SHELL (entry->popup_menu), menuitem);      
          
          _btk_text_util_append_special_char_menuitems (BTK_MENU_SHELL (submenu),
                                                        unichar_chosen_func,
                                                        entry);
        }
      
      g_signal_emit (entry,
		     signals[POPULATE_POPUP],
		     0,
		     entry->popup_menu);
  

      if (info->button)
	btk_menu_popup (BTK_MENU (entry->popup_menu), NULL, NULL,
			NULL, NULL,
			info->button, info->time);
      else
	{
	  btk_menu_popup (BTK_MENU (entry->popup_menu), NULL, NULL,
			  popup_position_func, entry,
			  info->button, info->time);
	  btk_menu_shell_select_first (BTK_MENU_SHELL (entry->popup_menu), FALSE);
	}
    }

  g_object_unref (entry);
  g_slice_free (PopupInfo, info);
}
			
static void
btk_entry_do_popup (BtkEntry       *entry,
                    BdkEventButton *event)
{
  PopupInfo *info = g_slice_new (PopupInfo);

  /* In order to know what entries we should make sensitive, we
   * ask for the current targets of the clipboard, and when
   * we get them, then we actually pop up the menu.
   */
  info->entry = g_object_ref (entry);
  
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

  btk_clipboard_request_contents (btk_widget_get_clipboard (BTK_WIDGET (entry), BDK_SELECTION_CLIPBOARD),
				  bdk_atom_intern_static_string ("TARGETS"),
				  popup_targets_received,
				  info);
}

static gboolean
btk_entry_popup_menu (BtkWidget *widget)
{
  btk_entry_do_popup (BTK_ENTRY (widget), NULL);
  return TRUE;
}

static void
btk_entry_drag_begin (BtkWidget      *widget,
                      BdkDragContext *context)
{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (widget);
  gint i;

  for (i = 0; i < MAX_ICONS; i++)
    {
      EntryIconInfo *icon_info = priv->icons[i];
      
      if (icon_info != NULL)
        {
          if (icon_info->in_drag) 
            {
              switch (icon_info->storage_type)
                {
                case BTK_IMAGE_STOCK:
                  btk_drag_set_icon_stock (context, icon_info->stock_id, -2, -2);
                  break;

                case BTK_IMAGE_ICON_NAME:
                  btk_drag_set_icon_name (context, icon_info->icon_name, -2, -2);
                  break;

                  /* FIXME: No GIcon support for dnd icons */
                case BTK_IMAGE_GICON:
                case BTK_IMAGE_PIXBUF:
                  btk_drag_set_icon_pixbuf (context, icon_info->pixbuf, -2, -2);
                  break;
                default:
                  g_assert_not_reached ();
                }
            }
        }
    }
}

static void
btk_entry_drag_end (BtkWidget      *widget,
                    BdkDragContext *context)
{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (widget);
  gint i;

  for (i = 0; i < MAX_ICONS; i++)
    {
      EntryIconInfo *icon_info = priv->icons[i];

      if (icon_info != NULL)
        icon_info->in_drag = 0;
    }
}

static void
btk_entry_drag_leave (BtkWidget        *widget,
		      BdkDragContext   *context,
		      guint             time)
{
  BtkEntry *entry = BTK_ENTRY (widget);

  entry->dnd_position = -1;
  btk_widget_queue_draw (widget);
}

static gboolean
btk_entry_drag_drop  (BtkWidget        *widget,
		      BdkDragContext   *context,
		      gint              x,
		      gint              y,
		      guint             time)
{
  BtkEntry *entry = BTK_ENTRY (widget);
  BdkAtom target = BDK_NONE;
  
  if (entry->editable)
    target = btk_drag_dest_find_target (widget, context, NULL);

  if (target != BDK_NONE)
    btk_drag_get_data (widget, context, target, time);
  else
    btk_drag_finish (context, FALSE, FALSE, time);
  
  return TRUE;
}

static gboolean
btk_entry_drag_motion (BtkWidget        *widget,
		       BdkDragContext   *context,
		       gint              x,
		       gint              y,
		       guint             time)
{
  BtkEntry *entry = BTK_ENTRY (widget);
  BtkWidget *source_widget;
  BdkDragAction suggested_action;
  gint new_position, old_position;
  gint sel1, sel2;
  
  x -= widget->style->xthickness;
  y -= widget->style->ythickness;
  
  old_position = entry->dnd_position;
  new_position = btk_entry_find_position (entry, x + entry->scroll_offset);

  if (entry->editable &&
      btk_drag_dest_find_target (widget, context, NULL) != BDK_NONE)
    {
      source_widget = btk_drag_get_source_widget (context);
      suggested_action = bdk_drag_context_get_suggested_action (context);

      if (!btk_editable_get_selection_bounds (BTK_EDITABLE (entry), &sel1, &sel2) ||
          new_position < sel1 || new_position > sel2)
        {
          if (source_widget == widget)
	    {
	      /* Default to MOVE, unless the user has
	       * pressed ctrl or alt to affect available actions
	       */
	      if ((bdk_drag_context_get_actions (context) & BDK_ACTION_MOVE) != 0)
	        suggested_action = BDK_ACTION_MOVE;
	    }
              
          entry->dnd_position = new_position;
        }
      else
        {
          if (source_widget == widget)
	    suggested_action = 0;	/* Can't drop in selection where drag started */
          
          entry->dnd_position = -1;
        }
    }
  else
    {
      /* Entry not editable, or no text */
      suggested_action = 0;
      entry->dnd_position = -1;
    }
  
  bdk_drag_status (context, suggested_action, time);
  
  if (entry->dnd_position != old_position)
    btk_widget_queue_draw (widget);

  return TRUE;
}

static void
btk_entry_drag_data_received (BtkWidget        *widget,
			      BdkDragContext   *context,
			      gint              x,
			      gint              y,
			      BtkSelectionData *selection_data,
			      guint             info,
			      guint             time)
{
  BtkEntry *entry = BTK_ENTRY (widget);
  BtkEditable *editable = BTK_EDITABLE (widget);
  gchar *str;

  str = (gchar *) btk_selection_data_get_text (selection_data);

  x -= widget->style->xthickness;
  y -= widget->style->ythickness;

  if (str && entry->editable)
    {
      gint new_position;
      gint sel1, sel2;
      gint length = -1;

      if (entry->truncate_multiline)
        length = truncate_multiline (str);

      new_position = btk_entry_find_position (entry, x + entry->scroll_offset);

      if (!btk_editable_get_selection_bounds (editable, &sel1, &sel2) ||
	  new_position < sel1 || new_position > sel2)
	{
	  btk_editable_insert_text (editable, str, length, &new_position);
	}
      else
	{
	  /* Replacing selection */
          begin_change (entry);
	  btk_editable_delete_text (editable, sel1, sel2);
	  btk_editable_insert_text (editable, str, length, &sel1);
          end_change (entry);
	}
      
      btk_drag_finish (context, TRUE, bdk_drag_context_get_selected_action (context) == BDK_ACTION_MOVE, time);
    }
  else
    {
      /* Drag and drop didn't happen! */
      btk_drag_finish (context, FALSE, FALSE, time);
    }

  g_free (str);
}

static void
btk_entry_drag_data_get (BtkWidget        *widget,
			 BdkDragContext   *context,
			 BtkSelectionData *selection_data,
			 guint             info,
			 guint             time)
{
  gint sel_start, sel_end;
  gint i;

  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (widget);
  BtkEditable *editable = BTK_EDITABLE (widget);

  /* If there is an icon drag going on, exit early. */
  for (i = 0; i < MAX_ICONS; i++)
    {
      EntryIconInfo *icon_info = priv->icons[i];

      if (icon_info != NULL)
        {
          if (icon_info->in_drag)
            return;
        }
    }

  if (btk_editable_get_selection_bounds (editable, &sel_start, &sel_end))
    {
      gchar *str = btk_entry_get_display_text (BTK_ENTRY (widget), sel_start, sel_end);

      btk_selection_data_set_text (selection_data, str, -1);
      
      g_free (str);
    }

}

static void
btk_entry_drag_data_delete (BtkWidget      *widget,
			    BdkDragContext *context)
{
  gint sel_start, sel_end;
  gint i;

  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (widget);
  BtkEditable *editable = BTK_EDITABLE (widget);

  /* If there is an icon drag going on, exit early. */
  for (i = 0; i < MAX_ICONS; i++)
    {
      EntryIconInfo *icon_info = priv->icons[i];

      if (icon_info != NULL)
        {
          if (icon_info->in_drag)
            return;
        }
    }
  
  if (BTK_ENTRY (widget)->editable &&
      btk_editable_get_selection_bounds (editable, &sel_start, &sel_end))
    btk_editable_delete_text (editable, sel_start, sel_end);
}

/* We display the cursor when
 *
 *  - the selection is empty, AND
 *  - the widget has focus
 */

#define CURSOR_ON_MULTIPLIER 2
#define CURSOR_OFF_MULTIPLIER 1
#define CURSOR_PEND_MULTIPLIER 3
#define CURSOR_DIVIDER 3

static gboolean
cursor_blinks (BtkEntry *entry)
{
  if (btk_widget_has_focus (BTK_WIDGET (entry)) &&
      entry->editable &&
      entry->selection_bound == entry->current_pos)
    {
      BtkSettings *settings;
      gboolean blink;

      settings = btk_widget_get_settings (BTK_WIDGET (entry));
      g_object_get (settings, "btk-cursor-blink", &blink, NULL);

      return blink;
    }
  else
    return FALSE;
}

static gint
get_cursor_time (BtkEntry *entry)
{
  BtkSettings *settings = btk_widget_get_settings (BTK_WIDGET (entry));
  gint time;

  g_object_get (settings, "btk-cursor-blink-time", &time, NULL);

  return time;
}

static gint
get_cursor_blink_timeout (BtkEntry *entry)
{
  BtkSettings *settings = btk_widget_get_settings (BTK_WIDGET (entry));
  gint timeout;

  g_object_get (settings, "btk-cursor-blink-timeout", &timeout, NULL);

  return timeout;
}

static void
show_cursor (BtkEntry *entry)
{
  BtkWidget *widget;

  if (!entry->cursor_visible)
    {
      entry->cursor_visible = TRUE;

      widget = BTK_WIDGET (entry);
      if (btk_widget_has_focus (widget) && entry->selection_bound == entry->current_pos)
	btk_widget_queue_draw (widget);
    }
}

static void
hide_cursor (BtkEntry *entry)
{
  BtkWidget *widget;

  if (entry->cursor_visible)
    {
      entry->cursor_visible = FALSE;

      widget = BTK_WIDGET (entry);
      if (btk_widget_has_focus (widget) && entry->selection_bound == entry->current_pos)
	btk_widget_queue_draw (widget);
    }
}

/*
 * Blink!
 */
static gint
blink_cb (gpointer data)
{
  BtkEntry *entry;
  BtkEntryPrivate *priv; 
  gint blink_timeout;

  entry = BTK_ENTRY (data);
  priv = BTK_ENTRY_GET_PRIVATE (entry);
 
  if (!btk_widget_has_focus (BTK_WIDGET (entry)))
    {
      g_warning ("BtkEntry - did not receive focus-out-event. If you\n"
		 "connect a handler to this signal, it must return\n"
		 "FALSE so the entry gets the event as well");

      btk_entry_check_cursor_blink (entry);

      return FALSE;
    }
  
  g_assert (entry->selection_bound == entry->current_pos);
  
  blink_timeout = get_cursor_blink_timeout (entry);
  if (priv->blink_time > 1000 * blink_timeout && 
      blink_timeout < G_MAXINT/1000) 
    {
      /* we've blinked enough without the user doing anything, stop blinking */
      show_cursor (entry);
      entry->blink_timeout = 0;
    } 
  else if (entry->cursor_visible)
    {
      hide_cursor (entry);
      entry->blink_timeout = bdk_threads_add_timeout (get_cursor_time (entry) * CURSOR_OFF_MULTIPLIER / CURSOR_DIVIDER,
					    blink_cb,
					    entry);
    }
  else
    {
      show_cursor (entry);
      priv->blink_time += get_cursor_time (entry);
      entry->blink_timeout = bdk_threads_add_timeout (get_cursor_time (entry) * CURSOR_ON_MULTIPLIER / CURSOR_DIVIDER,
					    blink_cb,
					    entry);
    }

  /* Remove ourselves */
  return FALSE;
}

static void
btk_entry_check_cursor_blink (BtkEntry *entry)
{
  BtkEntryPrivate *priv; 
  
  priv = BTK_ENTRY_GET_PRIVATE (entry);

  if (cursor_blinks (entry))
    {
      if (!entry->blink_timeout)
	{
	  show_cursor (entry);
	  entry->blink_timeout = bdk_threads_add_timeout (get_cursor_time (entry) * CURSOR_ON_MULTIPLIER / CURSOR_DIVIDER,
						blink_cb,
						entry);
	}
    }
  else
    {
      if (entry->blink_timeout)  
	{ 
	  g_source_remove (entry->blink_timeout);
	  entry->blink_timeout = 0;
	}
      
      entry->cursor_visible = TRUE;
    }
  
}

static void
btk_entry_pend_cursor_blink (BtkEntry *entry)
{
  if (cursor_blinks (entry))
    {
      if (entry->blink_timeout != 0)
	g_source_remove (entry->blink_timeout);
      
      entry->blink_timeout = bdk_threads_add_timeout (get_cursor_time (entry) * CURSOR_PEND_MULTIPLIER / CURSOR_DIVIDER,
					    blink_cb,
					    entry);
      show_cursor (entry);
    }
}

static void
btk_entry_reset_blink_time (BtkEntry *entry)
{
  BtkEntryPrivate *priv; 
  
  priv = BTK_ENTRY_GET_PRIVATE (entry);
  
  priv->blink_time = 0;
}


/* completion */
static gint
btk_entry_completion_timeout (gpointer data)
{
  BtkEntryCompletion *completion = BTK_ENTRY_COMPLETION (data);

  completion->priv->completion_timeout = 0;

  if (completion->priv->filter_model &&
      g_utf8_strlen (btk_entry_get_text (BTK_ENTRY (completion->priv->entry)), -1)
      >= completion->priv->minimum_key_length)
    {
      gint matches;
      gint actions;
      BtkTreeSelection *s;
      gboolean popup_single;

      btk_entry_completion_complete (completion);
      matches = btk_tree_model_iter_n_children (BTK_TREE_MODEL (completion->priv->filter_model), NULL);

      btk_tree_selection_unselect_all (btk_tree_view_get_selection (BTK_TREE_VIEW (completion->priv->tree_view)));

      s = btk_tree_view_get_selection (BTK_TREE_VIEW (completion->priv->action_view));

      btk_tree_selection_unselect_all (s);

      actions = btk_tree_model_iter_n_children (BTK_TREE_MODEL (completion->priv->actions), NULL);

      g_object_get (completion, "popup-single-match", &popup_single, NULL);
      if ((matches > (popup_single ? 0: 1)) || actions > 0)
	{ 
	  if (btk_widget_get_visible (completion->priv->popup_window))
	    _btk_entry_completion_resize_popup (completion);
          else
	    _btk_entry_completion_popup (completion);
	}
      else 
	_btk_entry_completion_popdown (completion);
    }
  else if (btk_widget_get_visible (completion->priv->popup_window))
    _btk_entry_completion_popdown (completion);

  return FALSE;
}

static inline gboolean
keyval_is_cursor_move (guint keyval)
{
  if (keyval == BDK_Up || keyval == BDK_KP_Up)
    return TRUE;

  if (keyval == BDK_Down || keyval == BDK_KP_Down)
    return TRUE;

  if (keyval == BDK_Page_Up)
    return TRUE;

  if (keyval == BDK_Page_Down)
    return TRUE;

  return FALSE;
}

static gboolean
btk_entry_completion_key_press (BtkWidget   *widget,
                                BdkEventKey *event,
                                gpointer     user_data)
{
  gint matches, actions = 0;
  BtkEntryCompletion *completion = BTK_ENTRY_COMPLETION (user_data);

  if (!btk_widget_get_mapped (completion->priv->popup_window))
    return FALSE;

  matches = btk_tree_model_iter_n_children (BTK_TREE_MODEL (completion->priv->filter_model), NULL);

  if (completion->priv->actions)
    actions = btk_tree_model_iter_n_children (BTK_TREE_MODEL (completion->priv->actions), NULL);

  if (keyval_is_cursor_move (event->keyval))
    {
      BtkTreePath *path = NULL;
      
      if (event->keyval == BDK_Up || event->keyval == BDK_KP_Up)
        {
	  if (completion->priv->current_selected < 0)
	    completion->priv->current_selected = matches + actions - 1;
	  else
	    completion->priv->current_selected--;
        }
      else if (event->keyval == BDK_Down || event->keyval == BDK_KP_Down)
        {
          if (completion->priv->current_selected < matches + actions - 1)
	    completion->priv->current_selected++;
	  else
            completion->priv->current_selected = -1;
        }
      else if (event->keyval == BDK_Page_Up)
	{
	  if (completion->priv->current_selected < 0)
	    completion->priv->current_selected = matches + actions - 1;
	  else if (completion->priv->current_selected == 0)
	    completion->priv->current_selected = -1;
	  else if (completion->priv->current_selected < matches) 
	    {
	      completion->priv->current_selected -= 14;
	      if (completion->priv->current_selected < 0)
		completion->priv->current_selected = 0;
	    }
	  else 
	    {
	      completion->priv->current_selected -= 14;
	      if (completion->priv->current_selected < matches - 1)
		completion->priv->current_selected = matches - 1;
	    }
	}
      else if (event->keyval == BDK_Page_Down)
	{
	  if (completion->priv->current_selected < 0)
	    completion->priv->current_selected = 0;
	  else if (completion->priv->current_selected < matches - 1)
	    {
	      completion->priv->current_selected += 14;
	      if (completion->priv->current_selected > matches - 1)
		completion->priv->current_selected = matches - 1;
	    }
	  else if (completion->priv->current_selected == matches + actions - 1)
	    {
	      completion->priv->current_selected = -1;
	    }
	  else
	    {
	      completion->priv->current_selected += 14;
	      if (completion->priv->current_selected > matches + actions - 1)
		completion->priv->current_selected = matches + actions - 1;
	    }
	}

      if (completion->priv->current_selected < 0)
        {
          btk_tree_selection_unselect_all (btk_tree_view_get_selection (BTK_TREE_VIEW (completion->priv->tree_view)));
          btk_tree_selection_unselect_all (btk_tree_view_get_selection (BTK_TREE_VIEW (completion->priv->action_view)));

          if (completion->priv->inline_selection &&
              completion->priv->completion_prefix)
            {
              btk_entry_set_text (BTK_ENTRY (completion->priv->entry), 
                                  completion->priv->completion_prefix);
              btk_editable_set_position (BTK_EDITABLE (widget), -1);
            }
        }
      else if (completion->priv->current_selected < matches)
        {
          btk_tree_selection_unselect_all (btk_tree_view_get_selection (BTK_TREE_VIEW (completion->priv->action_view)));

          path = btk_tree_path_new_from_indices (completion->priv->current_selected, -1);
          btk_tree_view_set_cursor (BTK_TREE_VIEW (completion->priv->tree_view),
                                    path, NULL, FALSE);

          if (completion->priv->inline_selection)
            {

              BtkTreeIter iter;
              BtkTreeIter child_iter;
              BtkTreeModel *model = NULL;
              BtkTreeSelection *sel;
              gboolean entry_set;

              sel = btk_tree_view_get_selection (BTK_TREE_VIEW (completion->priv->tree_view));
              if (!btk_tree_selection_get_selected (sel, &model, &iter))
                return FALSE;

              btk_tree_model_filter_convert_iter_to_child_iter (BTK_TREE_MODEL_FILTER (model), &child_iter, &iter);
              model = btk_tree_model_filter_get_model (BTK_TREE_MODEL_FILTER (model));
              
              if (completion->priv->completion_prefix == NULL)
                completion->priv->completion_prefix = g_strdup (btk_entry_get_text (BTK_ENTRY (completion->priv->entry)));

              g_signal_emit_by_name (completion, "cursor-on-match", model,
                                     &child_iter, &entry_set);
            }
        }
      else if (completion->priv->current_selected - matches >= 0)
        {
          btk_tree_selection_unselect_all (btk_tree_view_get_selection (BTK_TREE_VIEW (completion->priv->tree_view)));

          path = btk_tree_path_new_from_indices (completion->priv->current_selected - matches, -1);
          btk_tree_view_set_cursor (BTK_TREE_VIEW (completion->priv->action_view),
                                    path, NULL, FALSE);

          if (completion->priv->inline_selection &&
              completion->priv->completion_prefix)
            {
              btk_entry_set_text (BTK_ENTRY (completion->priv->entry), 
                                  completion->priv->completion_prefix);
              btk_editable_set_position (BTK_EDITABLE (widget), -1);
            }
        }

      btk_tree_path_free (path);

      return TRUE;
    }
  else if (event->keyval == BDK_Escape ||
           event->keyval == BDK_Left ||
           event->keyval == BDK_KP_Left ||
           event->keyval == BDK_Right ||
           event->keyval == BDK_KP_Right) 
    {
      gboolean retval = TRUE;

      _btk_entry_reset_im_context (BTK_ENTRY (widget));
      _btk_entry_completion_popdown (completion);

      if (completion->priv->current_selected < 0)
        {
          retval = FALSE;
          goto keypress_completion_out;
        }
      else if (completion->priv->inline_selection)
        {
          /* Escape rejects the tentative completion */
          if (event->keyval == BDK_Escape)
            {
              if (completion->priv->completion_prefix)
                btk_entry_set_text (BTK_ENTRY (completion->priv->entry), 
                                    completion->priv->completion_prefix);
              else 
                btk_entry_set_text (BTK_ENTRY (completion->priv->entry), "");
            }

          /* Move the cursor to the end for Right/Esc, to the
             beginning for Left */
          if (event->keyval == BDK_Right ||
              event->keyval == BDK_KP_Right ||
              event->keyval == BDK_Escape)
            btk_editable_set_position (BTK_EDITABLE (widget), -1);
          else
            btk_editable_set_position (BTK_EDITABLE (widget), 0);
        }

keypress_completion_out:
      if (completion->priv->inline_selection)
        {
          g_free (completion->priv->completion_prefix);
          completion->priv->completion_prefix = NULL;
        }

      return retval;
    }
  else if (event->keyval == BDK_Tab || 
	   event->keyval == BDK_KP_Tab ||
	   event->keyval == BDK_ISO_Left_Tab) 
    {
      _btk_entry_reset_im_context (BTK_ENTRY (widget));
      _btk_entry_completion_popdown (completion);

      g_free (completion->priv->completion_prefix);
      completion->priv->completion_prefix = NULL;

      return FALSE;
    }
  else if (event->keyval == BDK_ISO_Enter ||
           event->keyval == BDK_KP_Enter ||
	   event->keyval == BDK_Return)
    {
      BtkTreeIter iter;
      BtkTreeModel *model = NULL;
      BtkTreeModel *child_model;
      BtkTreeIter child_iter;
      BtkTreeSelection *sel;
      gboolean retval = TRUE;

      _btk_entry_reset_im_context (BTK_ENTRY (widget));
      _btk_entry_completion_popdown (completion);

      if (completion->priv->current_selected < matches)
        {
          gboolean entry_set;

          sel = btk_tree_view_get_selection (BTK_TREE_VIEW (completion->priv->tree_view));
          if (btk_tree_selection_get_selected (sel, &model, &iter))
            {
              btk_tree_model_filter_convert_iter_to_child_iter (BTK_TREE_MODEL_FILTER (model), &child_iter, &iter);
              child_model = btk_tree_model_filter_get_model (BTK_TREE_MODEL_FILTER (model));
              g_signal_handler_block (widget, completion->priv->changed_id);
              g_signal_emit_by_name (completion, "match-selected",
                                     child_model, &child_iter, &entry_set);
              g_signal_handler_unblock (widget, completion->priv->changed_id);

              if (!entry_set)
                {
                  gchar *str = NULL;

                  btk_tree_model_get (model, &iter,
                                      completion->priv->text_column, &str,
                                      -1);

                  btk_entry_set_text (BTK_ENTRY (widget), str);

                  /* move the cursor to the end */
                  btk_editable_set_position (BTK_EDITABLE (widget), -1);

                  g_free (str);
                }
            }
          else
            retval = FALSE;
        }
      else if (completion->priv->current_selected - matches >= 0)
        {
          sel = btk_tree_view_get_selection (BTK_TREE_VIEW (completion->priv->action_view));
          if (btk_tree_selection_get_selected (sel, &model, &iter))
            {
              BtkTreePath *path;

              path = btk_tree_path_new_from_indices (completion->priv->current_selected - matches, -1);
              g_signal_emit_by_name (completion, "action-activated",
                                     btk_tree_path_get_indices (path)[0]);
              btk_tree_path_free (path);
            }
          else
            retval = FALSE;
        }

      g_free (completion->priv->completion_prefix);
      completion->priv->completion_prefix = NULL;

      return retval;
    }

  return FALSE;
}

static void
btk_entry_completion_changed (BtkWidget *entry,
                              gpointer   user_data)
{
  BtkEntryCompletion *completion = BTK_ENTRY_COMPLETION (user_data);

  if (!completion->priv->popup_completion)
    return;

  /* (re)install completion timeout */
  if (completion->priv->completion_timeout)
    {
      g_source_remove (completion->priv->completion_timeout);
      completion->priv->completion_timeout = 0;
    }

  if (!btk_entry_get_text (BTK_ENTRY (entry)))
    return;

  /* no need to normalize for this test */
  if (completion->priv->minimum_key_length > 0 &&
      strcmp ("", btk_entry_get_text (BTK_ENTRY (entry))) == 0)
    {
      if (btk_widget_get_visible (completion->priv->popup_window))
        _btk_entry_completion_popdown (completion);
      return;
    }

  completion->priv->completion_timeout =
    bdk_threads_add_timeout (COMPLETION_TIMEOUT,
                   btk_entry_completion_timeout,
                   completion);
}

static gboolean
check_completion_callback (BtkEntryCompletion *completion)
{
  completion->priv->check_completion_idle = NULL;
  
  btk_entry_completion_complete (completion);
  btk_entry_completion_insert_prefix (completion);

  return FALSE;
}

static void
clear_completion_callback (BtkEntry   *entry,
			   GParamSpec *pspec)
{
  BtkEntryCompletion *completion = btk_entry_get_completion (entry);

  if (!completion->priv->inline_completion)
    return;

  if (pspec->name == I_("cursor-position") ||
      pspec->name == I_("selection-bound"))
    completion->priv->has_completion = FALSE;
}

static gboolean
accept_completion_callback (BtkEntry *entry)
{
  BtkEntryCompletion *completion = btk_entry_get_completion (entry);

  if (!completion->priv->inline_completion)
    return FALSE;

  if (completion->priv->has_completion)
    btk_editable_set_position (BTK_EDITABLE (entry),
			       btk_entry_buffer_get_length (get_buffer (entry)));

  return FALSE;
}

static void
completion_insert_text_callback (BtkEntry           *entry,
				 const gchar        *text,
				 gint                length,
				 gint                position,
				 BtkEntryCompletion *completion)
{
  if (!completion->priv->inline_completion)
    return;

  /* idle to update the selection based on the file list */
  if (completion->priv->check_completion_idle == NULL)
    {
      completion->priv->check_completion_idle = g_idle_source_new ();
      g_source_set_priority (completion->priv->check_completion_idle, G_PRIORITY_HIGH);
      g_source_set_closure (completion->priv->check_completion_idle,
			    g_cclosure_new_object (G_CALLBACK (check_completion_callback),
						   G_OBJECT (completion)));
      g_source_attach (completion->priv->check_completion_idle, NULL);
    }
}

static void
disconnect_completion_signals (BtkEntry           *entry,
			       BtkEntryCompletion *completion)
{
  if (completion->priv->changed_id > 0 &&
      g_signal_handler_is_connected (entry, completion->priv->changed_id))
    {
      g_signal_handler_disconnect (entry, completion->priv->changed_id);
      completion->priv->changed_id = 0;
    }
  g_signal_handlers_disconnect_by_func (entry, 
					G_CALLBACK (btk_entry_completion_key_press), completion);
  if (completion->priv->insert_text_id > 0 &&
      g_signal_handler_is_connected (entry, completion->priv->insert_text_id))
    {
      g_signal_handler_disconnect (entry, completion->priv->insert_text_id);
      completion->priv->insert_text_id = 0;
    }
  g_signal_handlers_disconnect_by_func (entry, 
					G_CALLBACK (completion_insert_text_callback), completion);
  g_signal_handlers_disconnect_by_func (entry, 
					G_CALLBACK (clear_completion_callback), completion);
  g_signal_handlers_disconnect_by_func (entry, 
					G_CALLBACK (accept_completion_callback), completion);
}

static void
connect_completion_signals (BtkEntry           *entry,
			    BtkEntryCompletion *completion)
{
  completion->priv->changed_id =
    g_signal_connect (entry, "changed",
                      G_CALLBACK (btk_entry_completion_changed), completion);
  g_signal_connect (entry, "key-press-event",
                    G_CALLBACK (btk_entry_completion_key_press), completion);

  completion->priv->insert_text_id =
    g_signal_connect (entry, "insert-text",
                      G_CALLBACK (completion_insert_text_callback), completion);
  g_signal_connect (entry, "notify",
                    G_CALLBACK (clear_completion_callback), completion);
  g_signal_connect (entry, "activate",
                    G_CALLBACK (accept_completion_callback), completion);
  g_signal_connect (entry, "focus-out-event",
                    G_CALLBACK (accept_completion_callback), completion);
}

/**
 * btk_entry_set_completion:
 * @entry: A #BtkEntry
 * @completion: (allow-none): The #BtkEntryCompletion or %NULL
 *
 * Sets @completion to be the auxiliary completion object to use with @entry.
 * All further configuration of the completion mechanism is done on
 * @completion using the #BtkEntryCompletion API. Completion is disabled if
 * @completion is set to %NULL.
 *
 * Since: 2.4
 */
void
btk_entry_set_completion (BtkEntry           *entry,
                          BtkEntryCompletion *completion)
{
  BtkEntryCompletion *old;

  g_return_if_fail (BTK_IS_ENTRY (entry));
  g_return_if_fail (!completion || BTK_IS_ENTRY_COMPLETION (completion));

  old = btk_entry_get_completion (entry);

  if (old == completion)
    return;
  
  if (old)
    {
      if (old->priv->completion_timeout)
        {
          g_source_remove (old->priv->completion_timeout);
          old->priv->completion_timeout = 0;
        }

      if (old->priv->check_completion_idle)
        {
          g_source_destroy (old->priv->check_completion_idle);
          old->priv->check_completion_idle = NULL;
        }

      if (btk_widget_get_mapped (old->priv->popup_window))
        _btk_entry_completion_popdown (old);

      disconnect_completion_signals (entry, old);
      old->priv->entry = NULL;

      g_object_unref (old);
    }

  if (!completion)
    {
      g_object_set_data (G_OBJECT (entry), I_(BTK_ENTRY_COMPLETION_KEY), NULL);
      return;
    }

  /* hook into the entry */
  g_object_ref (completion);

  connect_completion_signals (entry, completion);    
  completion->priv->entry = BTK_WIDGET (entry);
  g_object_set_data (G_OBJECT (entry), I_(BTK_ENTRY_COMPLETION_KEY), completion);
}

/**
 * btk_entry_get_completion:
 * @entry: A #BtkEntry
 *
 * Returns the auxiliary completion object currently in use by @entry.
 *
 * Return value: (transfer none): The auxiliary completion object currently
 *     in use by @entry.
 *
 * Since: 2.4
 */
BtkEntryCompletion *
btk_entry_get_completion (BtkEntry *entry)
{
  BtkEntryCompletion *completion;

  g_return_val_if_fail (BTK_IS_ENTRY (entry), NULL);

  completion = BTK_ENTRY_COMPLETION (g_object_get_data (G_OBJECT (entry),
                                     BTK_ENTRY_COMPLETION_KEY));

  return completion;
}

/**
 * btk_entry_set_cursor_hadjustment:
 * @entry: a #BtkEntry
 * @adjustment: an adjustment which should be adjusted when the cursor 
 *              is moved, or %NULL
 *
 * Hooks up an adjustment to the cursor position in an entry, so that when 
 * the cursor is moved, the adjustment is scrolled to show that position. 
 * See btk_scrolled_window_get_hadjustment() for a typical way of obtaining 
 * the adjustment.
 *
 * The adjustment has to be in pixel units and in the same coordinate system 
 * as the entry. 
 * 
 * Since: 2.12
 */
void
btk_entry_set_cursor_hadjustment (BtkEntry      *entry,
                                  BtkAdjustment *adjustment)
{
  g_return_if_fail (BTK_IS_ENTRY (entry));
  if (adjustment)
    g_return_if_fail (BTK_IS_ADJUSTMENT (adjustment));

  if (adjustment)
    g_object_ref (adjustment);

  g_object_set_qdata_full (G_OBJECT (entry), 
                           quark_cursor_hadjustment,
                           adjustment, 
                           g_object_unref);
}

/**
 * btk_entry_get_cursor_hadjustment:
 * @entry: a #BtkEntry
 *
 * Retrieves the horizontal cursor adjustment for the entry. 
 * See btk_entry_set_cursor_hadjustment().
 *
 * Return value: (transfer none): the horizontal cursor adjustment, or %NULL
 *   if none has been set.
 *
 * Since: 2.12
 */
BtkAdjustment*
btk_entry_get_cursor_hadjustment (BtkEntry *entry)
{
  g_return_val_if_fail (BTK_IS_ENTRY (entry), NULL);

  return g_object_get_qdata (G_OBJECT (entry), quark_cursor_hadjustment);
}

/**
 * btk_entry_set_progress_fraction:
 * @entry: a #BtkEntry
 * @fraction: fraction of the task that's been completed
 *
 * Causes the entry's progress indicator to "fill in" the given
 * fraction of the bar. The fraction should be between 0.0 and 1.0,
 * inclusive.
 *
 * Since: 2.16
 */
void
btk_entry_set_progress_fraction (BtkEntry *entry,
                                 gdouble   fraction)
{
  BtkWidget       *widget;
  BtkEntryPrivate *private;
  gdouble          old_fraction;
  gint x, y, width, height;
  gint old_x, old_y, old_width, old_height;

  g_return_if_fail (BTK_IS_ENTRY (entry));

  widget = BTK_WIDGET (entry);
  private = BTK_ENTRY_GET_PRIVATE (entry);

  if (private->progress_pulse_mode)
    old_fraction = -1;
  else
    old_fraction = private->progress_fraction;

  if (btk_widget_is_drawable (widget))
    get_progress_area (widget, &old_x, &old_y, &old_width, &old_height);

  fraction = CLAMP (fraction, 0.0, 1.0);

  private->progress_fraction = fraction;
  private->progress_pulse_mode = FALSE;
  private->progress_pulse_current = 0.0;

  if (btk_widget_is_drawable (widget))
    {
      get_progress_area (widget, &x, &y, &width, &height);

      if ((x != old_x) || (y != old_y) || (width != old_width) || (height != old_height))
        btk_widget_queue_draw (widget);
    }

  if (fraction != old_fraction)
    g_object_notify (G_OBJECT (entry), "progress-fraction");
}

/**
 * btk_entry_get_progress_fraction:
 * @entry: a #BtkEntry
 *
 * Returns the current fraction of the task that's been completed.
 * See btk_entry_set_progress_fraction().
 *
 * Return value: a fraction from 0.0 to 1.0
 *
 * Since: 2.16
 */
gdouble
btk_entry_get_progress_fraction (BtkEntry *entry)
{
  BtkEntryPrivate *private;

  g_return_val_if_fail (BTK_IS_ENTRY (entry), 0.0);

  private = BTK_ENTRY_GET_PRIVATE (entry);

  return private->progress_fraction;
}

/**
 * btk_entry_set_progress_pulse_step:
 * @entry: a #BtkEntry
 * @fraction: fraction between 0.0 and 1.0
 *
 * Sets the fraction of total entry width to move the progress
 * bouncing block for each call to btk_entry_progress_pulse().
 *
 * Since: 2.16
 */
void
btk_entry_set_progress_pulse_step (BtkEntry *entry,
                                   gdouble   fraction)
{
  BtkEntryPrivate *private;

  g_return_if_fail (BTK_IS_ENTRY (entry));

  private = BTK_ENTRY_GET_PRIVATE (entry);

  fraction = CLAMP (fraction, 0.0, 1.0);

  if (fraction != private->progress_pulse_fraction)
    {
      private->progress_pulse_fraction = fraction;

      btk_widget_queue_draw (BTK_WIDGET (entry));

      g_object_notify (G_OBJECT (entry), "progress-pulse-step");
    }
}

/**
 * btk_entry_get_progress_pulse_step:
 * @entry: a #BtkEntry
 *
 * Retrieves the pulse step set with btk_entry_set_progress_pulse_step().
 *
 * Return value: a fraction from 0.0 to 1.0
 *
 * Since: 2.16
 */
gdouble
btk_entry_get_progress_pulse_step (BtkEntry *entry)
{
  BtkEntryPrivate *private;

  g_return_val_if_fail (BTK_IS_ENTRY (entry), 0.0);

  private = BTK_ENTRY_GET_PRIVATE (entry);

  return private->progress_pulse_fraction;
}

/**
 * btk_entry_progress_pulse:
 * @entry: a #BtkEntry
 *
 * Indicates that some progress is made, but you don't know how much.
 * Causes the entry's progress indicator to enter "activity mode,"
 * where a block bounces back and forth. Each call to
 * btk_entry_progress_pulse() causes the block to move by a little bit
 * (the amount of movement per pulse is determined by
 * btk_entry_set_progress_pulse_step()).
 *
 * Since: 2.16
 */
void
btk_entry_progress_pulse (BtkEntry *entry)
{
  BtkEntryPrivate *private;

  g_return_if_fail (BTK_IS_ENTRY (entry));

  private = BTK_ENTRY_GET_PRIVATE (entry);

  if (private->progress_pulse_mode)
    {
      if (private->progress_pulse_way_back)
        {
          private->progress_pulse_current -= private->progress_pulse_fraction;

          if (private->progress_pulse_current < 0.0)
            {
              private->progress_pulse_current = 0.0;
              private->progress_pulse_way_back = FALSE;
            }
        }
      else
        {
          private->progress_pulse_current += private->progress_pulse_fraction;

          if (private->progress_pulse_current > 1.0 - private->progress_pulse_fraction)
            {
              private->progress_pulse_current = 1.0 - private->progress_pulse_fraction;
              private->progress_pulse_way_back = TRUE;
            }
        }
    }
  else
    {
      private->progress_fraction = 0.0;
      private->progress_pulse_mode = TRUE;
      private->progress_pulse_way_back = FALSE;
      private->progress_pulse_current = 0.0;
    }

  btk_widget_queue_draw (BTK_WIDGET (entry));
}

/* Caps Lock warning for password entries */

static void
show_capslock_feedback (BtkEntry    *entry,
                        const gchar *text)
{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);

  if (btk_entry_get_icon_storage_type (entry, BTK_ENTRY_ICON_SECONDARY) == BTK_IMAGE_EMPTY)
    {
      btk_entry_set_icon_from_stock (entry, BTK_ENTRY_ICON_SECONDARY, BTK_STOCK_CAPS_LOCK_WARNING);
      btk_entry_set_icon_activatable (entry, BTK_ENTRY_ICON_SECONDARY, FALSE);
      priv->caps_lock_warning_shown = TRUE;
    }

  if (priv->caps_lock_warning_shown)
    btk_entry_set_icon_tooltip_text (entry, BTK_ENTRY_ICON_SECONDARY, text);
  else
    g_warning ("Can't show Caps Lock warning, since secondary icon is set");
}

static void
remove_capslock_feedback (BtkEntry *entry)
{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);

  if (priv->caps_lock_warning_shown)
    {
      btk_entry_set_icon_from_stock (entry, BTK_ENTRY_ICON_SECONDARY, NULL);
      priv->caps_lock_warning_shown = FALSE;
    } 
}

static void
keymap_state_changed (BdkKeymap *keymap, 
                      BtkEntry  *entry)
{
  BtkEntryPrivate *priv = BTK_ENTRY_GET_PRIVATE (entry);
  char *text = NULL;

  if (btk_entry_get_display_mode (entry) != DISPLAY_NORMAL && priv->caps_lock_warning)
    { 
      if (bdk_keymap_get_caps_lock_state (keymap))
        text = _("Caps Lock is on");
    }

  if (text)
    show_capslock_feedback (entry, text);
  else
    remove_capslock_feedback (entry);
}

#define __BTK_ENTRY_C__
#include "btkaliasdef.c"
