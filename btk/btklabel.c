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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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

#include "btklabel.h"
#include "btkaccellabel.h"
#include "btkdnd.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkbango.h"
#include "btkwindow.h"
#include "bdk/bdkkeysyms.h"
#include "btkclipboard.h"
#include "btkimagemenuitem.h"
#include "btkintl.h"
#include "btkseparatormenuitem.h"
#include "btktextutil.h"
#include "btkmenuitem.h"
#include "btknotebook.h"
#include "btkstock.h"
#include "btkbindings.h"
#include "btkbuildable.h"
#include "btkimage.h"
#include "btkshow.h"
#include "btktooltip.h"
#include "btkprivate.h"
#include "btkalias.h"

#define BTK_LABEL_GET_PRIVATE(obj) (B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_LABEL, BtkLabelPrivate))

typedef struct
{
  gint wrap_width;
  gint width_chars;
  gint max_width_chars;

  gboolean mnemonics_visible;
} BtkLabelPrivate;

/* Notes about the handling of links:
 *
 * Links share the BtkLabelSelectionInfo struct with selectable labels.
 * There are some new fields for links. The links field contains the list
 * of BtkLabelLink structs that describe the links which are embedded in
 * the label. The active_link field points to the link under the mouse
 * pointer. For keyboard navigation, the 'focus' link is determined by
 * finding the link which contains the selection_anchor position.
 * The link_clicked field is used with button press and release events
 * to ensure that pressing inside a link and releasing outside of it
 * does not activate the link.
 *
 * Links are rendered with the link-color/visited-link-color colors
 * that are determined by the style and with an underline. When the mouse
 * pointer is over a link, the pointer is changed to indicate the link,
 * and the background behind the link is rendered with the base[PRELIGHT]
 * color. While a button is pressed over a link, the background is rendered
 * with the base[ACTIVE] color.
 *
 * Labels with links accept keyboard focus, and it is possible to move
 * the focus between the embedded links using Tab/Shift-Tab. The focus
 * is indicated by a focus rectangle that is drawn around the link text.
 * Pressing Enter activates the focussed link, and there is a suitable
 * context menu for links that can be opened with the Menu key. Pressing
 * Control-C copies the link URI to the clipboard.
 *
 * In selectable labels with links, link functionality is only available
 * when the selection is empty.
 */
typedef struct
{
  gchar *uri;
  gchar *title;     /* the title attribute, used as tooltip */
  gboolean visited; /* get set when the link is activated; this flag
                     * gets preserved over later set_markup() calls
                     */
  gint start;       /* position of the link in the BangoLayout */
  gint end;
} BtkLabelLink;

struct _BtkLabelSelectionInfo
{
  BdkWindow *window;
  gint selection_anchor;
  gint selection_end;
  BtkWidget *popup_menu;

  GList *links;
  BtkLabelLink *active_link;

  gint drag_start_x;
  gint drag_start_y;

  guint in_drag      : 1;
  guint select_words : 1;
  guint selectable   : 1;
  guint link_clicked : 1;
};

enum {
  MOVE_CURSOR,
  COPY_CLIPBOARD,
  POPULATE_POPUP,
  ACTIVATE_LINK,
  ACTIVATE_CURRENT_LINK,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_LABEL,
  PROP_ATTRIBUTES,
  PROP_USE_MARKUP,
  PROP_USE_UNDERLINE,
  PROP_JUSTIFY,
  PROP_PATTERN,
  PROP_WRAP,
  PROP_WRAP_MODE,
  PROP_SELECTABLE,
  PROP_MNEMONIC_KEYVAL,
  PROP_MNEMONIC_WIDGET,
  PROP_CURSOR_POSITION,
  PROP_SELECTION_BOUND,
  PROP_ELLIPSIZE,
  PROP_WIDTH_CHARS,
  PROP_SINGLE_LINE_MODE,
  PROP_ANGLE,
  PROP_MAX_WIDTH_CHARS,
  PROP_TRACK_VISITED_LINKS
};

static guint signals[LAST_SIGNAL] = { 0 };

static const BdkColor default_link_color = { 0, 0, 0, 0xeeee };
static const BdkColor default_visited_link_color = { 0, 0x5555, 0x1a1a, 0x8b8b };

static void btk_label_set_property      (BObject          *object,
					 guint             prop_id,
					 const BValue     *value,
					 BParamSpec       *pspec);
static void btk_label_get_property      (BObject          *object,
					 guint             prop_id,
					 BValue           *value,
					 BParamSpec       *pspec);
static void btk_label_destroy           (BtkObject        *object);
static void btk_label_finalize          (BObject          *object);
static void btk_label_size_request      (BtkWidget        *widget,
					 BtkRequisition   *requisition);
static void btk_label_size_allocate     (BtkWidget        *widget,
                                         BtkAllocation    *allocation);
static void btk_label_state_changed     (BtkWidget        *widget,
                                         BtkStateType      state);
static void btk_label_style_set         (BtkWidget        *widget,
					 BtkStyle         *previous_style);
static void btk_label_direction_changed (BtkWidget        *widget,
					 BtkTextDirection  previous_dir);
static gint btk_label_expose            (BtkWidget        *widget,
					 BdkEventExpose   *event);
static gboolean btk_label_focus         (BtkWidget         *widget,
                                         BtkDirectionType   direction);

static void btk_label_realize           (BtkWidget        *widget);
static void btk_label_unrealize         (BtkWidget        *widget);
static void btk_label_map               (BtkWidget        *widget);
static void btk_label_unmap             (BtkWidget        *widget);

static gboolean btk_label_button_press      (BtkWidget        *widget,
					     BdkEventButton   *event);
static gboolean btk_label_button_release    (BtkWidget        *widget,
					     BdkEventButton   *event);
static gboolean btk_label_motion            (BtkWidget        *widget,
					     BdkEventMotion   *event);
static gboolean btk_label_leave_notify      (BtkWidget        *widget,
                                             BdkEventCrossing *event);

static void     btk_label_grab_focus        (BtkWidget        *widget);

static gboolean btk_label_query_tooltip     (BtkWidget        *widget,
                                             gint              x,
                                             gint              y,
                                             gboolean          keyboard_tip,
                                             BtkTooltip       *tooltip);

static void btk_label_set_text_internal          (BtkLabel      *label,
						  gchar         *str);
static void btk_label_set_label_internal         (BtkLabel      *label,
						  gchar         *str);
static void btk_label_set_use_markup_internal    (BtkLabel      *label,
						  gboolean       val);
static void btk_label_set_use_underline_internal (BtkLabel      *label,
						  gboolean       val);
static void btk_label_set_attributes_internal    (BtkLabel      *label,
						  BangoAttrList *attrs);
static void btk_label_set_uline_text_internal    (BtkLabel      *label,
						  const gchar   *str);
static void btk_label_set_pattern_internal       (BtkLabel      *label,
				                  const gchar   *pattern,
                                                  gboolean       is_mnemonic);
static void btk_label_set_markup_internal        (BtkLabel      *label,
						  const gchar   *str,
						  gboolean       with_uline);
static void btk_label_recalculate                (BtkLabel      *label);
static void btk_label_hierarchy_changed          (BtkWidget     *widget,
						  BtkWidget     *old_toplevel);
static void btk_label_screen_changed             (BtkWidget     *widget,
						  BdkScreen     *old_screen);
static gboolean btk_label_popup_menu             (BtkWidget     *widget);

static void btk_label_create_window       (BtkLabel *label);
static void btk_label_destroy_window      (BtkLabel *label);
static void btk_label_ensure_select_info  (BtkLabel *label);
static void btk_label_clear_select_info   (BtkLabel *label);
static void btk_label_update_cursor       (BtkLabel *label);
static void btk_label_clear_layout        (BtkLabel *label);
static void btk_label_ensure_layout       (BtkLabel *label);
static void btk_label_invalidate_wrap_width (BtkLabel *label);
static void btk_label_select_rebunnyion_index (BtkLabel *label,
                                           gint      anchor_index,
                                           gint      end_index);

static gboolean btk_label_mnemonic_activate (BtkWidget         *widget,
					     gboolean           group_cycling);
static void     btk_label_setup_mnemonic    (BtkLabel          *label,
					     guint              last_key);
static void     btk_label_drag_data_get     (BtkWidget         *widget,
					     BdkDragContext    *context,
					     BtkSelectionData  *selection_data,
					     guint              info,
					     guint              time);

static void     btk_label_buildable_interface_init     (BtkBuildableIface *iface);
static gboolean btk_label_buildable_custom_tag_start   (BtkBuildable     *buildable,
							BtkBuilder       *builder,
							BObject          *child,
							const gchar      *tagname,
							GMarkupParser    *parser,
							gpointer         *data);

static void     btk_label_buildable_custom_finished    (BtkBuildable     *buildable,
							BtkBuilder       *builder,
							BObject          *child,
							const gchar      *tagname,
							gpointer          user_data);


static void connect_mnemonics_visible_notify    (BtkLabel   *label);
static gboolean      separate_uline_pattern     (const gchar  *str,
                                                 guint        *accel_key,
                                                 gchar       **new_str,
                                                 gchar       **pattern);


/* For selectable labels: */
static void btk_label_move_cursor        (BtkLabel        *label,
					  BtkMovementStep  step,
					  gint             count,
					  gboolean         extend_selection);
static void btk_label_copy_clipboard     (BtkLabel        *label);
static void btk_label_select_all         (BtkLabel        *label);
static void btk_label_do_popup           (BtkLabel        *label,
					  BdkEventButton  *event);
static gint btk_label_move_forward_word  (BtkLabel        *label,
					  gint             start);
static gint btk_label_move_backward_word (BtkLabel        *label,
					  gint             start);

/* For links: */
static void          btk_label_rescan_links     (BtkLabel  *label);
static void          btk_label_clear_links      (BtkLabel  *label);
static gboolean      btk_label_activate_link    (BtkLabel    *label,
                                                 const gchar *uri);
static void          btk_label_activate_current_link (BtkLabel *label);
static BtkLabelLink *btk_label_get_current_link (BtkLabel  *label);
static void          btk_label_get_link_colors  (BtkWidget  *widget,
                                                 BdkColor  **link_color,
                                                 BdkColor  **visited_link_color);
static void          emit_activate_link         (BtkLabel     *label,
                                                 BtkLabelLink *link);

static GQuark quark_angle = 0;

static BtkBuildableIface *buildable_parent_iface = NULL;

G_DEFINE_TYPE_WITH_CODE (BtkLabel, btk_label, BTK_TYPE_MISC,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
						btk_label_buildable_interface_init));

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
				B_TYPE_ENUM, step,
				B_TYPE_INT, count,
				B_TYPE_BOOLEAN, FALSE);

  /* Selection-extending version */
  btk_binding_entry_add_signal (binding_set, keyval, modmask | BDK_SHIFT_MASK,
				"move-cursor", 3,
				B_TYPE_ENUM, step,
				B_TYPE_INT, count,
				B_TYPE_BOOLEAN, TRUE);
}

static void
btk_label_class_init (BtkLabelClass *class)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (class);
  BtkObjectClass *object_class = BTK_OBJECT_CLASS (class);
  BtkWidgetClass *widget_class = BTK_WIDGET_CLASS (class);
  BtkBindingSet *binding_set;

  quark_angle = g_quark_from_static_string ("angle");

  bobject_class->set_property = btk_label_set_property;
  bobject_class->get_property = btk_label_get_property;
  bobject_class->finalize = btk_label_finalize;

  object_class->destroy = btk_label_destroy;

  widget_class->size_request = btk_label_size_request;
  widget_class->size_allocate = btk_label_size_allocate;
  widget_class->state_changed = btk_label_state_changed;
  widget_class->style_set = btk_label_style_set;
  widget_class->query_tooltip = btk_label_query_tooltip;
  widget_class->direction_changed = btk_label_direction_changed;
  widget_class->expose_event = btk_label_expose;
  widget_class->realize = btk_label_realize;
  widget_class->unrealize = btk_label_unrealize;
  widget_class->map = btk_label_map;
  widget_class->unmap = btk_label_unmap;
  widget_class->button_press_event = btk_label_button_press;
  widget_class->button_release_event = btk_label_button_release;
  widget_class->motion_notify_event = btk_label_motion;
  widget_class->leave_notify_event = btk_label_leave_notify;
  widget_class->hierarchy_changed = btk_label_hierarchy_changed;
  widget_class->screen_changed = btk_label_screen_changed;
  widget_class->mnemonic_activate = btk_label_mnemonic_activate;
  widget_class->drag_data_get = btk_label_drag_data_get;
  widget_class->grab_focus = btk_label_grab_focus;
  widget_class->popup_menu = btk_label_popup_menu;
  widget_class->focus = btk_label_focus;

  class->move_cursor = btk_label_move_cursor;
  class->copy_clipboard = btk_label_copy_clipboard;
  class->activate_link = btk_label_activate_link;

  /**
   * BtkLabel::move-cursor:
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
		  B_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkLabelClass, move_cursor),
		  NULL, NULL,
		  _btk_marshal_VOID__ENUM_INT_BOOLEAN,
		  B_TYPE_NONE, 3,
		  BTK_TYPE_MOVEMENT_STEP,
		  B_TYPE_INT,
		  B_TYPE_BOOLEAN);

   /**
   * BtkLabel::copy-clipboard:
   * @label: the object which received the signal
   *
   * The ::copy-clipboard signal is a
   * <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted to copy the selection to the clipboard.
   *
   * The default binding for this signal is Ctrl-c.
   */ 
  signals[COPY_CLIPBOARD] =
    g_signal_new (I_("copy-clipboard"),
		  B_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkLabelClass, copy_clipboard),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
  
  /**
   * BtkLabel::populate-popup:
   * @label: The label on which the signal is emitted
   * @menu: the menu that is being populated
   *
   * The ::populate-popup signal gets emitted before showing the
   * context menu of the label. Note that only selectable labels
   * have context menus.
   *
   * If you need to add items to the context menu, connect
   * to this signal and append your menuitems to the @menu.
   */
  signals[POPULATE_POPUP] =
    g_signal_new (I_("populate-popup"),
		  B_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkLabelClass, populate_popup),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT,
		  B_TYPE_NONE, 1,
		  BTK_TYPE_MENU);

    /**
     * BtkLabel::activate-current-link:
     * @label: The label on which the signal was emitted
     *
     * A <link linkend="keybinding-signals">keybinding signal</link>
     * which gets emitted when the user activates a link in the label.
     *
     * Applications may also emit the signal with g_signal_emit_by_name()
     * if they need to control activation of URIs programmatically.
     *
     * The default bindings for this signal are all forms of the Enter key.
     *
     * Since: 2.18
     */
    signals[ACTIVATE_CURRENT_LINK] =
      g_signal_new_class_handler ("activate-current-link",
                                  B_TYPE_FROM_CLASS (object_class),
                                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                  G_CALLBACK (btk_label_activate_current_link),
                                  NULL, NULL,
                                  _btk_marshal_VOID__VOID,
                                  B_TYPE_NONE, 0);

    /**
     * BtkLabel::activate-link:
     * @label: The label on which the signal was emitted
     * @uri: the URI that is activated
     *
     * The signal which gets emitted to activate a URI.
     * Applications may connect to it to override the default behaviour,
     * which is to call btk_show_uri().
     *
     * Returns: %TRUE if the link has been activated
     *
     * Since: 2.18
     */
    signals[ACTIVATE_LINK] =
      g_signal_new ("activate-link",
                    B_TYPE_FROM_CLASS (object_class),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (BtkLabelClass, activate_link),
                    _btk_boolean_handled_accumulator, NULL,
                    _btk_marshal_BOOLEAN__STRING,
                    B_TYPE_BOOLEAN, 1, B_TYPE_STRING);

  g_object_class_install_property (bobject_class,
                                   PROP_LABEL,
                                   g_param_spec_string ("label",
                                                        P_("Label"),
                                                        P_("The text of the label"),
                                                        "",
                                                        BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_ATTRIBUTES,
				   g_param_spec_boxed ("attributes",
						       P_("Attributes"),
						       P_("A list of style attributes to apply to the text of the label"),
						       BANGO_TYPE_ATTR_LIST,
						       BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_USE_MARKUP,
                                   g_param_spec_boolean ("use-markup",
							 P_("Use markup"),
							 P_("The text of the label includes XML markup. See bango_parse_markup()"),
                                                        FALSE,
                                                        BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_USE_UNDERLINE,
                                   g_param_spec_boolean ("use-underline",
							 P_("Use underline"),
							 P_("If set, an underline in the text indicates the next character should be used for the mnemonic accelerator key"),
                                                        FALSE,
                                                        BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
				   PROP_JUSTIFY,
                                   g_param_spec_enum ("justify",
                                                      P_("Justification"),
                                                      P_("The alignment of the lines in the text of the label relative to each other. This does NOT affect the alignment of the label within its allocation. See BtkMisc::xalign for that"),
						      BTK_TYPE_JUSTIFICATION,
						      BTK_JUSTIFY_LEFT,
                                                      BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_PATTERN,
                                   g_param_spec_string ("pattern",
                                                        P_("Pattern"),
                                                        P_("A string with _ characters in positions correspond to characters in the text to underline"),
                                                        NULL,
                                                        BTK_PARAM_WRITABLE));

  g_object_class_install_property (bobject_class,
                                   PROP_WRAP,
                                   g_param_spec_boolean ("wrap",
                                                        P_("Line wrap"),
                                                        P_("If set, wrap lines if the text becomes too wide"),
                                                        FALSE,
                                                        BTK_PARAM_READWRITE));
  /**
   * BtkLabel:wrap-mode:
   *
   * If line wrapping is on (see the #BtkLabel:wrap property) this controls 
   * how the line wrapping is done. The default is %BANGO_WRAP_WORD, which 
   * means wrap on word boundaries.
   *
   * Since: 2.10
   */
  g_object_class_install_property (bobject_class,
                                   PROP_WRAP_MODE,
                                   g_param_spec_enum ("wrap-mode",
						      P_("Line wrap mode"),
						      P_("If wrap is set, controls how linewrapping is done"),
						      BANGO_TYPE_WRAP_MODE,
						      BANGO_WRAP_WORD,
						      BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_SELECTABLE,
                                   g_param_spec_boolean ("selectable",
                                                        P_("Selectable"),
                                                        P_("Whether the label text can be selected with the mouse"),
                                                        FALSE,
                                                        BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_MNEMONIC_KEYVAL,
                                   g_param_spec_uint ("mnemonic-keyval",
						      P_("Mnemonic key"),
						      P_("The mnemonic accelerator key for this label"),
						      0,
						      G_MAXUINT,
						      BDK_VoidSymbol,
						      BTK_PARAM_READABLE));
  g_object_class_install_property (bobject_class,
                                   PROP_MNEMONIC_WIDGET,
                                   g_param_spec_object ("mnemonic-widget",
							P_("Mnemonic widget"),
							P_("The widget to be activated when the label's mnemonic "
							  "key is pressed"),
							BTK_TYPE_WIDGET,
							BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_CURSOR_POSITION,
                                   g_param_spec_int ("cursor-position",
                                                     P_("Cursor Position"),
                                                     P_("The current position of the insertion cursor in chars"),
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     BTK_PARAM_READABLE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_SELECTION_BOUND,
                                   g_param_spec_int ("selection-bound",
                                                     P_("Selection Bound"),
                                                     P_("The position of the opposite end of the selection from the cursor in chars"),
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     BTK_PARAM_READABLE));
  
  /**
   * BtkLabel:ellipsize:
   *
   * The preferred place to ellipsize the string, if the label does 
   * not have enough room to display the entire string, specified as a 
   * #BangoEllisizeMode. 
   *
   * Note that setting this property to a value other than 
   * %BANGO_ELLIPSIZE_NONE has the side-effect that the label requests 
   * only enough space to display the ellipsis "...". In particular, this 
   * means that ellipsizing labels do not work well in notebook tabs, unless 
   * the tab's #BtkNotebook:tab-expand property is set to %TRUE. Other ways
   * to set a label's width are btk_widget_set_size_request() and
   * btk_label_set_width_chars().
   *
   * Since: 2.6
   */
  g_object_class_install_property (bobject_class,
				   PROP_ELLIPSIZE,
                                   g_param_spec_enum ("ellipsize",
                                                      P_("Ellipsize"),
                                                      P_("The preferred place to ellipsize the string, if the label does not have enough room to display the entire string"),
						      BANGO_TYPE_ELLIPSIZE_MODE,
						      BANGO_ELLIPSIZE_NONE,
                                                      BTK_PARAM_READWRITE));

  /**
   * BtkLabel:width-chars:
   * 
   * The desired width of the label, in characters. If this property is set to
   * -1, the width will be calculated automatically, otherwise the label will
   * request either 3 characters or the property value, whichever is greater.
   * If the "width-chars" property is set to a positive value, then the 
   * #BtkLabel:max-width-chars property is ignored. 
   *
   * Since: 2.6
   **/
  g_object_class_install_property (bobject_class,
                                   PROP_WIDTH_CHARS,
                                   g_param_spec_int ("width-chars",
                                                     P_("Width In Characters"),
                                                     P_("The desired width of the label, in characters"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     BTK_PARAM_READWRITE));
  
  /**
   * BtkLabel:single-line-mode:
   * 
   * Whether the label is in single line mode. In single line mode,
   * the height of the label does not depend on the actual text, it
   * is always set to ascent + descent of the font. This can be an
   * advantage in situations where resizing the label because of text 
   * changes would be distracting, e.g. in a statusbar.
   *
   * Since: 2.6
   **/
  g_object_class_install_property (bobject_class,
                                   PROP_SINGLE_LINE_MODE,
                                   g_param_spec_boolean ("single-line-mode",
                                                        P_("Single Line Mode"),
                                                        P_("Whether the label is in single line mode"),
                                                        FALSE,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkLabel:angle:
   * 
   * The angle that the baseline of the label makes with the horizontal,
   * in degrees, measured counterclockwise. An angle of 90 reads from
   * from bottom to top, an angle of 270, from top to bottom. Ignored
   * if the label is selectable, wrapped, or ellipsized.
   *
   * Since: 2.6
   **/
  g_object_class_install_property (bobject_class,
                                   PROP_ANGLE,
                                   g_param_spec_double ("angle",
							P_("Angle"),
							P_("Angle at which the label is rotated"),
							0.0,
							360.0,
							0.0, 
							BTK_PARAM_READWRITE));
  
  /**
   * BtkLabel:max-width-chars:
   * 
   * The desired maximum width of the label, in characters. If this property 
   * is set to -1, the width will be calculated automatically, otherwise the 
   * label will request space for no more than the requested number of 
   * characters. If the #BtkLabel:width-chars property is set to a positive 
   * value, then the "max-width-chars" property is ignored.
   * 
   * Since: 2.6
   **/
  g_object_class_install_property (bobject_class,
                                   PROP_MAX_WIDTH_CHARS,
                                   g_param_spec_int ("max-width-chars",
                                                     P_("Maximum Width In Characters"),
                                                     P_("The desired maximum width of the label, in characters"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     BTK_PARAM_READWRITE));

  /**
   * BtkLabel:track-visited-links:
   *
   * Set this property to %TRUE to make the label track which links
   * have been clicked. It will then apply the ::visited-link-color
   * color, instead of ::link-color.
   *
   * Since: 2.18
   */
  g_object_class_install_property (bobject_class,
                                   PROP_TRACK_VISITED_LINKS,
                                   g_param_spec_boolean ("track-visited-links",
                                                         P_("Track visited links"),
                                                         P_("Whether visited links should be tracked"),
                                                         TRUE,
                                                         BTK_PARAM_READWRITE));
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
  
  add_move_binding (binding_set, BDK_f, BDK_CONTROL_MASK,
		    BTK_MOVEMENT_LOGICAL_POSITIONS, 1);
  
  add_move_binding (binding_set, BDK_b, BDK_CONTROL_MASK,
		    BTK_MOVEMENT_LOGICAL_POSITIONS, -1);
  
  add_move_binding (binding_set, BDK_Right, BDK_CONTROL_MASK,
		    BTK_MOVEMENT_WORDS, 1);

  add_move_binding (binding_set, BDK_Left, BDK_CONTROL_MASK,
		    BTK_MOVEMENT_WORDS, -1);

  add_move_binding (binding_set, BDK_KP_Right, BDK_CONTROL_MASK,
		    BTK_MOVEMENT_WORDS, 1);

  add_move_binding (binding_set, BDK_KP_Left, BDK_CONTROL_MASK,
		    BTK_MOVEMENT_WORDS, -1);

  /* select all */
  btk_binding_entry_add_signal (binding_set, BDK_a, BDK_CONTROL_MASK,
				"move-cursor", 3,
				B_TYPE_ENUM, BTK_MOVEMENT_PARAGRAPH_ENDS,
				B_TYPE_INT, -1,
				B_TYPE_BOOLEAN, FALSE);

  btk_binding_entry_add_signal (binding_set, BDK_a, BDK_CONTROL_MASK,
				"move-cursor", 3,
				B_TYPE_ENUM, BTK_MOVEMENT_PARAGRAPH_ENDS,
				B_TYPE_INT, 1,
				B_TYPE_BOOLEAN, TRUE);

  btk_binding_entry_add_signal (binding_set, BDK_slash, BDK_CONTROL_MASK,
				"move-cursor", 3,
				B_TYPE_ENUM, BTK_MOVEMENT_PARAGRAPH_ENDS,
				B_TYPE_INT, -1,
				B_TYPE_BOOLEAN, FALSE);

  btk_binding_entry_add_signal (binding_set, BDK_slash, BDK_CONTROL_MASK,
				"move-cursor", 3,
				B_TYPE_ENUM, BTK_MOVEMENT_PARAGRAPH_ENDS,
				B_TYPE_INT, 1,
				B_TYPE_BOOLEAN, TRUE);

  /* unselect all */
  btk_binding_entry_add_signal (binding_set, BDK_a, BDK_SHIFT_MASK | BDK_CONTROL_MASK,
				"move-cursor", 3,
				B_TYPE_ENUM, BTK_MOVEMENT_PARAGRAPH_ENDS,
				B_TYPE_INT, 0,
				B_TYPE_BOOLEAN, FALSE);

  btk_binding_entry_add_signal (binding_set, BDK_backslash, BDK_CONTROL_MASK,
				"move-cursor", 3,
				B_TYPE_ENUM, BTK_MOVEMENT_PARAGRAPH_ENDS,
				B_TYPE_INT, 0,
				B_TYPE_BOOLEAN, FALSE);

  add_move_binding (binding_set, BDK_f, BDK_MOD1_MASK,
		    BTK_MOVEMENT_WORDS, 1);

  add_move_binding (binding_set, BDK_b, BDK_MOD1_MASK,
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

  /* copy */
  btk_binding_entry_add_signal (binding_set, BDK_c, BDK_CONTROL_MASK,
				"copy-clipboard", 0);

  btk_binding_entry_add_signal (binding_set, BDK_Return, 0,
				"activate-current-link", 0);
  btk_binding_entry_add_signal (binding_set, BDK_ISO_Enter, 0,
				"activate-current-link", 0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Enter, 0,
				"activate-current-link", 0);

  g_type_class_add_private (class, sizeof (BtkLabelPrivate));
}

static void 
btk_label_set_property (BObject      *object,
			guint         prop_id,
			const BValue *value,
			BParamSpec   *pspec)
{
  BtkLabel *label;

  label = BTK_LABEL (object);
  
  switch (prop_id)
    {
    case PROP_LABEL:
      btk_label_set_label (label, b_value_get_string (value));
      break;
    case PROP_ATTRIBUTES:
      btk_label_set_attributes (label, b_value_get_boxed (value));
      break;
    case PROP_USE_MARKUP:
      btk_label_set_use_markup (label, b_value_get_boolean (value));
      break;
    case PROP_USE_UNDERLINE:
      btk_label_set_use_underline (label, b_value_get_boolean (value));
      break;
    case PROP_JUSTIFY:
      btk_label_set_justify (label, b_value_get_enum (value));
      break;
    case PROP_PATTERN:
      btk_label_set_pattern (label, b_value_get_string (value));
      break;
    case PROP_WRAP:
      btk_label_set_line_wrap (label, b_value_get_boolean (value));
      break;	  
    case PROP_WRAP_MODE:
      btk_label_set_line_wrap_mode (label, b_value_get_enum (value));
      break;	  
    case PROP_SELECTABLE:
      btk_label_set_selectable (label, b_value_get_boolean (value));
      break;	  
    case PROP_MNEMONIC_WIDGET:
      btk_label_set_mnemonic_widget (label, (BtkWidget*) b_value_get_object (value));
      break;
    case PROP_ELLIPSIZE:
      btk_label_set_ellipsize (label, b_value_get_enum (value));
      break;
    case PROP_WIDTH_CHARS:
      btk_label_set_width_chars (label, b_value_get_int (value));
      break;
    case PROP_SINGLE_LINE_MODE:
      btk_label_set_single_line_mode (label, b_value_get_boolean (value));
      break;	  
    case PROP_ANGLE:
      btk_label_set_angle (label, b_value_get_double (value));
      break;
    case PROP_MAX_WIDTH_CHARS:
      btk_label_set_max_width_chars (label, b_value_get_int (value));
      break;
    case PROP_TRACK_VISITED_LINKS:
      btk_label_set_track_visited_links (label, b_value_get_boolean (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
btk_label_get_property (BObject     *object,
			guint        prop_id,
			BValue      *value,
			BParamSpec  *pspec)
{
  BtkLabel *label;
  
  label = BTK_LABEL (object);
  
  switch (prop_id)
    {
    case PROP_LABEL:
      b_value_set_string (value, label->label);
      break;
    case PROP_ATTRIBUTES:
      b_value_set_boxed (value, label->attrs);
      break;
    case PROP_USE_MARKUP:
      b_value_set_boolean (value, label->use_markup);
      break;
    case PROP_USE_UNDERLINE:
      b_value_set_boolean (value, label->use_underline);
      break;
    case PROP_JUSTIFY:
      b_value_set_enum (value, label->jtype);
      break;
    case PROP_WRAP:
      b_value_set_boolean (value, label->wrap);
      break;
    case PROP_WRAP_MODE:
      b_value_set_enum (value, label->wrap_mode);
      break;
    case PROP_SELECTABLE:
      b_value_set_boolean (value, btk_label_get_selectable (label));
      break;
    case PROP_MNEMONIC_KEYVAL:
      b_value_set_uint (value, label->mnemonic_keyval);
      break;
    case PROP_MNEMONIC_WIDGET:
      b_value_set_object (value, (BObject*) label->mnemonic_widget);
      break;
    case PROP_CURSOR_POSITION:
      if (label->select_info && label->select_info->selectable)
	{
	  gint offset = g_utf8_pointer_to_offset (label->text,
						  label->text + label->select_info->selection_end);
	  b_value_set_int (value, offset);
	}
      else
	b_value_set_int (value, 0);
      break;
    case PROP_SELECTION_BOUND:
      if (label->select_info && label->select_info->selectable)
	{
	  gint offset = g_utf8_pointer_to_offset (label->text,
						  label->text + label->select_info->selection_anchor);
	  b_value_set_int (value, offset);
	}
      else
	b_value_set_int (value, 0);
      break;
    case PROP_ELLIPSIZE:
      b_value_set_enum (value, label->ellipsize);
      break;
    case PROP_WIDTH_CHARS:
      b_value_set_int (value, btk_label_get_width_chars (label));
      break;
    case PROP_SINGLE_LINE_MODE:
      b_value_set_boolean (value, btk_label_get_single_line_mode (label));
      break;
    case PROP_ANGLE:
      b_value_set_double (value, btk_label_get_angle (label));
      break;
    case PROP_MAX_WIDTH_CHARS:
      b_value_set_int (value, btk_label_get_max_width_chars (label));
      break;
    case PROP_TRACK_VISITED_LINKS:
      b_value_set_boolean (value, btk_label_get_track_visited_links (label));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_label_init (BtkLabel *label)
{
  BtkLabelPrivate *priv;

  btk_widget_set_has_window (BTK_WIDGET (label), FALSE);

  priv = BTK_LABEL_GET_PRIVATE (label);
  priv->width_chars = -1;
  priv->max_width_chars = -1;
  priv->wrap_width = -1;
  label->label = NULL;

  label->jtype = BTK_JUSTIFY_LEFT;
  label->wrap = FALSE;
  label->wrap_mode = BANGO_WRAP_WORD;
  label->ellipsize = BANGO_ELLIPSIZE_NONE;

  label->use_underline = FALSE;
  label->use_markup = FALSE;
  label->pattern_set = FALSE;
  label->track_links = TRUE;

  label->mnemonic_keyval = BDK_VoidSymbol;
  label->layout = NULL;
  label->text = NULL;
  label->attrs = NULL;

  label->mnemonic_widget = NULL;
  label->mnemonic_window = NULL;

  priv->mnemonics_visible = TRUE;

  btk_label_set_text (label, "");
}


static void
btk_label_buildable_interface_init (BtkBuildableIface *iface)
{
  buildable_parent_iface = g_type_interface_peek_parent (iface);

  iface->custom_tag_start = btk_label_buildable_custom_tag_start;
  iface->custom_finished = btk_label_buildable_custom_finished;
}

typedef struct {
  BtkBuilder    *builder;
  BObject       *object;
  BangoAttrList *attrs;
} BangoParserData;

static BangoAttribute *
attribute_from_text (BtkBuilder   *builder,
		     const gchar  *name, 
		     const gchar  *value,
		     GError      **error)
{
  BangoAttribute *attribute = NULL;
  BangoAttrType   type;
  BangoLanguage  *language;
  BangoFontDescription *font_desc;
  BdkColor       *color;
  BValue          val = { 0, };

  if (!btk_builder_value_from_string_type (builder, BANGO_TYPE_ATTR_TYPE, name, &val, error))
    return NULL;

  type = b_value_get_enum (&val);
  b_value_unset (&val);

  switch (type)
    {
      /* BangoAttrLanguage */
    case BANGO_ATTR_LANGUAGE:
      if ((language = bango_language_from_string (value)))
	{
	  attribute = bango_attr_language_new (language);
	  b_value_init (&val, B_TYPE_INT);
	}
      break;
      /* BangoAttrInt */
    case BANGO_ATTR_STYLE:
      if (btk_builder_value_from_string_type (builder, BANGO_TYPE_STYLE, value, &val, error))
	attribute = bango_attr_style_new (b_value_get_enum (&val));
      break;
    case BANGO_ATTR_WEIGHT:
      if (btk_builder_value_from_string_type (builder, BANGO_TYPE_WEIGHT, value, &val, error))
	attribute = bango_attr_weight_new (b_value_get_enum (&val));
      break;
    case BANGO_ATTR_VARIANT:
      if (btk_builder_value_from_string_type (builder, BANGO_TYPE_VARIANT, value, &val, error))
	attribute = bango_attr_variant_new (b_value_get_enum (&val));
      break;
    case BANGO_ATTR_STRETCH:
      if (btk_builder_value_from_string_type (builder, BANGO_TYPE_STRETCH, value, &val, error))
	attribute = bango_attr_stretch_new (b_value_get_enum (&val));
      break;
    case BANGO_ATTR_UNDERLINE:
      if (btk_builder_value_from_string_type (builder, B_TYPE_BOOLEAN, value, &val, error))
	attribute = bango_attr_underline_new (b_value_get_boolean (&val));
      break;
    case BANGO_ATTR_STRIKETHROUGH:	
      if (btk_builder_value_from_string_type (builder, B_TYPE_BOOLEAN, value, &val, error))
	attribute = bango_attr_strikethrough_new (b_value_get_boolean (&val));
      break;
    case BANGO_ATTR_GRAVITY:
      if (btk_builder_value_from_string_type (builder, BANGO_TYPE_GRAVITY, value, &val, error))
	attribute = bango_attr_gravity_new (b_value_get_enum (&val));
      break;
    case BANGO_ATTR_GRAVITY_HINT:
      if (btk_builder_value_from_string_type (builder, BANGO_TYPE_GRAVITY_HINT, 
					      value, &val, error))
	attribute = bango_attr_gravity_hint_new (b_value_get_enum (&val));
      break;

      /* BangoAttrString */	  
    case BANGO_ATTR_FAMILY:
      attribute = bango_attr_family_new (value);
      b_value_init (&val, B_TYPE_INT);
      break;

      /* BangoAttrSize */	  
    case BANGO_ATTR_SIZE:
      if (btk_builder_value_from_string_type (builder, B_TYPE_INT, 
					      value, &val, error))
	attribute = bango_attr_size_new (b_value_get_int (&val));
      break;
    case BANGO_ATTR_ABSOLUTE_SIZE:
      if (btk_builder_value_from_string_type (builder, B_TYPE_INT, 
					      value, &val, error))
	attribute = bango_attr_size_new_absolute (b_value_get_int (&val));
      break;
    
      /* BangoAttrFontDesc */
    case BANGO_ATTR_FONT_DESC:
      if ((font_desc = bango_font_description_from_string (value)))
	{
	  attribute = bango_attr_font_desc_new (font_desc);
	  bango_font_description_free (font_desc);
	  b_value_init (&val, B_TYPE_INT);
	}
      break;

      /* BangoAttrColor */
    case BANGO_ATTR_FOREGROUND:
      if (btk_builder_value_from_string_type (builder, BDK_TYPE_COLOR, 
					      value, &val, error))
	{
	  color = b_value_get_boxed (&val);
	  attribute = bango_attr_foreground_new (color->red, color->green, color->blue);
	}
      break;
    case BANGO_ATTR_BACKGROUND: 
      if (btk_builder_value_from_string_type (builder, BDK_TYPE_COLOR, 
					      value, &val, error))
	{
	  color = b_value_get_boxed (&val);
	  attribute = bango_attr_background_new (color->red, color->green, color->blue);
	}
      break;
    case BANGO_ATTR_UNDERLINE_COLOR:
      if (btk_builder_value_from_string_type (builder, BDK_TYPE_COLOR, 
					      value, &val, error))
	{
	  color = b_value_get_boxed (&val);
	  attribute = bango_attr_underline_color_new (color->red, color->green, color->blue);
	}
      break;
    case BANGO_ATTR_STRIKETHROUGH_COLOR:
      if (btk_builder_value_from_string_type (builder, BDK_TYPE_COLOR, 
					      value, &val, error))
	{
	  color = b_value_get_boxed (&val);
	  attribute = bango_attr_strikethrough_color_new (color->red, color->green, color->blue);
	}
      break;
      
      /* BangoAttrShape */
    case BANGO_ATTR_SHAPE:
      /* Unsupported for now */
      break;
      /* BangoAttrFloat */
    case BANGO_ATTR_SCALE:
      if (btk_builder_value_from_string_type (builder, B_TYPE_DOUBLE, 
					      value, &val, error))
	attribute = bango_attr_scale_new (b_value_get_double (&val));
      break;

    case BANGO_ATTR_INVALID:
    case BANGO_ATTR_LETTER_SPACING:
    case BANGO_ATTR_RISE:
    case BANGO_ATTR_FALLBACK:
    default:
      break;
    }

  b_value_unset (&val);

  return attribute;
}


static void
bango_start_element (GMarkupParseContext *context,
		     const gchar         *element_name,
		     const gchar        **names,
		     const gchar        **values,
		     gpointer             user_data,
		     GError             **error)
{
  BangoParserData *data = (BangoParserData*)user_data;
  BValue val = { 0, };
  guint i;
  gint line_number, char_number;

  if (strcmp (element_name, "attribute") == 0)
    {
      BangoAttribute *attr = NULL;
      const gchar *name = NULL;
      const gchar *value = NULL;
      const gchar *start = NULL;
      const gchar *end = NULL;
      guint start_val = 0;
      guint end_val   = G_MAXUINT;

      for (i = 0; names[i]; i++)
	{
	  if (strcmp (names[i], "name") == 0)
	    name = values[i];
	  else if (strcmp (names[i], "value") == 0)
	    value = values[i];
	  else if (strcmp (names[i], "start") == 0)
	    start = values[i];
	  else if (strcmp (names[i], "end") == 0)
	    end = values[i];
	  else
	    {
	      g_markup_parse_context_get_position (context,
						   &line_number,
						   &char_number);
	      g_set_error (error,
			   BTK_BUILDER_ERROR,
			   BTK_BUILDER_ERROR_INVALID_ATTRIBUTE,
			   "%s:%d:%d '%s' is not a valid attribute of <%s>",
			   "<input>",
			   line_number, char_number, names[i], "attribute");
	      return;
	    }
	}

      if (!name || !value)
	{
	  g_markup_parse_context_get_position (context,
					       &line_number,
					       &char_number);
	  g_set_error (error,
		       BTK_BUILDER_ERROR,
		       BTK_BUILDER_ERROR_MISSING_ATTRIBUTE,
		       "%s:%d:%d <%s> requires attribute \"%s\"",
		       "<input>",
		       line_number, char_number, "attribute",
		       name ? "value" : "name");
	  return;
	}

      if (start)
	{
	  if (!btk_builder_value_from_string_type (data->builder, B_TYPE_UINT, 
						   start, &val, error))
	    return;
	  start_val = b_value_get_uint (&val);
	  b_value_unset (&val);
	}

      if (end)
	{
	  if (!btk_builder_value_from_string_type (data->builder, B_TYPE_UINT, 
						   end, &val, error))
	    return;
	  end_val = b_value_get_uint (&val);
	  b_value_unset (&val);
	}

      attr = attribute_from_text (data->builder, name, value, error);

      if (attr)
	{
          attr->start_index = start_val;
          attr->end_index   = end_val;

	  if (!data->attrs)
	    data->attrs = bango_attr_list_new ();

	  bango_attr_list_insert (data->attrs, attr);
	}
    }
  else if (strcmp (element_name, "attributes") == 0)
    ;
  else
    g_warning ("Unsupported tag for BtkLabel: %s\n", element_name);
}

static const GMarkupParser bango_parser =
  {
    bango_start_element,
  };

static gboolean
btk_label_buildable_custom_tag_start (BtkBuildable     *buildable,
				      BtkBuilder       *builder,
				      BObject          *child,
				      const gchar      *tagname,
				      GMarkupParser    *parser,
				      gpointer         *data)
{
  if (buildable_parent_iface->custom_tag_start (buildable, builder, child, 
						tagname, parser, data))
    return TRUE;

  if (strcmp (tagname, "attributes") == 0)
    {
      BangoParserData *parser_data;

      parser_data = g_slice_new0 (BangoParserData);
      parser_data->builder = g_object_ref (builder);
      parser_data->object = g_object_ref (buildable);
      *parser = bango_parser;
      *data = parser_data;
      return TRUE;
    }
  return FALSE;
}

static void
btk_label_buildable_custom_finished (BtkBuildable *buildable,
				     BtkBuilder   *builder,
				     BObject      *child,
				     const gchar  *tagname,
				     gpointer      user_data)
{
  BangoParserData *data;

  buildable_parent_iface->custom_finished (buildable, builder, child, 
					   tagname, user_data);

  if (strcmp (tagname, "attributes") == 0)
    {
      data = (BangoParserData*)user_data;

      if (data->attrs)
	{
	  btk_label_set_attributes (BTK_LABEL (buildable), data->attrs);
	  bango_attr_list_unref (data->attrs);
	}

      g_object_unref (data->object);
      g_object_unref (data->builder);
      g_slice_free (BangoParserData, data);
    }
}


/**
 * btk_label_new:
 * @str: The text of the label
 *
 * Creates a new label with the given text inside it. You can
 * pass %NULL to get an empty label widget.
 *
 * Return value: the new #BtkLabel
 **/
BtkWidget*
btk_label_new (const gchar *str)
{
  BtkLabel *label;
  
  label = g_object_new (BTK_TYPE_LABEL, NULL);

  if (str && *str)
    btk_label_set_text (label, str);
  
  return BTK_WIDGET (label);
}

/**
 * btk_label_new_with_mnemonic:
 * @str: The text of the label, with an underscore in front of the
 *       mnemonic character
 *
 * Creates a new #BtkLabel, containing the text in @str.
 *
 * If characters in @str are preceded by an underscore, they are
 * underlined. If you need a literal underscore character in a label, use
 * '__' (two underscores). The first underlined character represents a 
 * keyboard accelerator called a mnemonic. The mnemonic key can be used 
 * to activate another widget, chosen automatically, or explicitly using
 * btk_label_set_mnemonic_widget().
 * 
 * If btk_label_set_mnemonic_widget() is not called, then the first 
 * activatable ancestor of the #BtkLabel will be chosen as the mnemonic 
 * widget. For instance, if the label is inside a button or menu item, 
 * the button or menu item will automatically become the mnemonic widget 
 * and be activated by the mnemonic.
 *
 * Return value: the new #BtkLabel
 **/
BtkWidget*
btk_label_new_with_mnemonic (const gchar *str)
{
  BtkLabel *label;
  
  label = g_object_new (BTK_TYPE_LABEL, NULL);

  if (str && *str)
    btk_label_set_text_with_mnemonic (label, str);
  
  return BTK_WIDGET (label);
}

static gboolean
btk_label_mnemonic_activate (BtkWidget *widget,
			     gboolean   group_cycling)
{
  BtkWidget *parent;

  if (BTK_LABEL (widget)->mnemonic_widget)
    return btk_widget_mnemonic_activate (BTK_LABEL (widget)->mnemonic_widget, group_cycling);

  /* Try to find the widget to activate by traversing the
   * widget's ancestry.
   */
  parent = widget->parent;

  if (BTK_IS_NOTEBOOK (parent))
    return FALSE;
  
  while (parent)
    {
      if (btk_widget_get_can_focus (parent) ||
	  (!group_cycling && BTK_WIDGET_GET_CLASS (parent)->activate_signal) ||
          BTK_IS_NOTEBOOK (parent->parent) ||
	  BTK_IS_MENU_ITEM (parent))
	return btk_widget_mnemonic_activate (parent, group_cycling);
      parent = parent->parent;
    }

  /* barf if there was nothing to activate */
  g_warning ("Couldn't find a target for a mnemonic activation.");
  btk_widget_error_bell (widget);

  return FALSE;
}

static void
btk_label_setup_mnemonic (BtkLabel *label,
			  guint     last_key)
{
  BtkWidget *widget = BTK_WIDGET (label);
  BtkWidget *toplevel;
  BtkWidget *mnemonic_menu;
  
  mnemonic_menu = g_object_get_data (B_OBJECT (label), "btk-mnemonic-menu");
  
  if (last_key != BDK_VoidSymbol)
    {
      if (label->mnemonic_window)
	{
	  btk_window_remove_mnemonic  (label->mnemonic_window,
				       last_key,
				       widget);
	  label->mnemonic_window = NULL;
	}
      if (mnemonic_menu)
	{
	  _btk_menu_shell_remove_mnemonic (BTK_MENU_SHELL (mnemonic_menu),
					   last_key,
					   widget);
	  mnemonic_menu = NULL;
	}
    }
  
  if (label->mnemonic_keyval == BDK_VoidSymbol)
      goto done;

  connect_mnemonics_visible_notify (BTK_LABEL (widget));

  toplevel = btk_widget_get_toplevel (widget);
  if (btk_widget_is_toplevel (toplevel))
    {
      BtkWidget *menu_shell;
      
      menu_shell = btk_widget_get_ancestor (widget,
					    BTK_TYPE_MENU_SHELL);

      if (menu_shell)
	{
	  _btk_menu_shell_add_mnemonic (BTK_MENU_SHELL (menu_shell),
					label->mnemonic_keyval,
					widget);
	  mnemonic_menu = menu_shell;
	}
      
      if (!BTK_IS_MENU (menu_shell))
	{
	  btk_window_add_mnemonic (BTK_WINDOW (toplevel),
				   label->mnemonic_keyval,
				   widget);
	  label->mnemonic_window = BTK_WINDOW (toplevel);
	}
    }
  
 done:
  g_object_set_data (B_OBJECT (label), I_("btk-mnemonic-menu"), mnemonic_menu);
}

static void
btk_label_hierarchy_changed (BtkWidget *widget,
			     BtkWidget *old_toplevel)
{
  BtkLabel *label = BTK_LABEL (widget);

  btk_label_setup_mnemonic (label, label->mnemonic_keyval);
}

static void
label_shortcut_setting_apply (BtkLabel *label)
{
  btk_label_recalculate (label);
  if (BTK_IS_ACCEL_LABEL (label))
    btk_accel_label_refetch (BTK_ACCEL_LABEL (label));
}

static void
label_shortcut_setting_traverse_container (BtkWidget *widget,
                                           gpointer   data)
{
  if (BTK_IS_LABEL (widget))
    label_shortcut_setting_apply (BTK_LABEL (widget));
  else if (BTK_IS_CONTAINER (widget))
    btk_container_forall (BTK_CONTAINER (widget),
                          label_shortcut_setting_traverse_container, data);
}

static void
label_shortcut_setting_changed (BtkSettings *settings)
{
  GList *list, *l;

  list = btk_window_list_toplevels ();

  for (l = list; l ; l = l->next)
    {
      BtkWidget *widget = l->data;

      if (btk_widget_get_settings (widget) == settings)
        btk_container_forall (BTK_CONTAINER (widget),
                              label_shortcut_setting_traverse_container, NULL);
    }

  g_list_free (list);
}

static void
mnemonics_visible_apply (BtkWidget *widget,
                         gboolean   mnemonics_visible)
{
  BtkLabel *label;
  BtkLabelPrivate *priv;

  label = BTK_LABEL (widget);

  priv = BTK_LABEL_GET_PRIVATE (label);

  mnemonics_visible = mnemonics_visible != FALSE;

  if (priv->mnemonics_visible != mnemonics_visible)
    {
      priv->mnemonics_visible = mnemonics_visible;

      btk_label_recalculate (label);
    }
}

static void
label_mnemonics_visible_traverse_container (BtkWidget *widget,
                                            gpointer   data)
{
  gboolean mnemonics_visible = GPOINTER_TO_INT (data);

  _btk_label_mnemonics_visible_apply_recursively (widget, mnemonics_visible);
}

void
_btk_label_mnemonics_visible_apply_recursively (BtkWidget *widget,
                                                gboolean   mnemonics_visible)
{
  if (BTK_IS_LABEL (widget))
    mnemonics_visible_apply (widget, mnemonics_visible);
  else if (BTK_IS_CONTAINER (widget))
    btk_container_forall (BTK_CONTAINER (widget),
                          label_mnemonics_visible_traverse_container,
                          GINT_TO_POINTER (mnemonics_visible));
}

static void
label_mnemonics_visible_changed (BtkWindow  *window,
                                 BParamSpec *pspec,
                                 gpointer    data)
{
  gboolean mnemonics_visible;

  g_object_get (window, "mnemonics-visible", &mnemonics_visible, NULL);

  btk_container_forall (BTK_CONTAINER (window),
                        label_mnemonics_visible_traverse_container,
                        GINT_TO_POINTER (mnemonics_visible));
}

static void
btk_label_screen_changed (BtkWidget *widget,
			  BdkScreen *old_screen)
{
  BtkSettings *settings;
  gboolean shortcuts_connected;

  if (!btk_widget_has_screen (widget))
    return;

  settings = btk_widget_get_settings (widget);

  shortcuts_connected =
    GPOINTER_TO_INT (g_object_get_data (B_OBJECT (settings),
                                        "btk-label-shortcuts-connected"));

  if (! shortcuts_connected)
    {
      g_signal_connect (settings, "notify::btk-enable-mnemonics",
                        G_CALLBACK (label_shortcut_setting_changed),
                        NULL);
      g_signal_connect (settings, "notify::btk-enable-accels",
                        G_CALLBACK (label_shortcut_setting_changed),
                        NULL);

      g_object_set_data (B_OBJECT (settings), "btk-label-shortcuts-connected",
                         GINT_TO_POINTER (TRUE));
    }

  label_shortcut_setting_apply (BTK_LABEL (widget));
}


static void
label_mnemonic_widget_weak_notify (gpointer      data,
				   BObject      *where_the_object_was)
{
  BtkLabel *label = data;

  label->mnemonic_widget = NULL;
  g_object_notify (B_OBJECT (label), "mnemonic-widget");
}

/**
 * btk_label_set_mnemonic_widget:
 * @label: a #BtkLabel
 * @widget: (allow-none): the target #BtkWidget
 *
 * If the label has been set so that it has an mnemonic key (using
 * i.e. btk_label_set_markup_with_mnemonic(),
 * btk_label_set_text_with_mnemonic(), btk_label_new_with_mnemonic()
 * or the "use_underline" property) the label can be associated with a
 * widget that is the target of the mnemonic. When the label is inside
 * a widget (like a #BtkButton or a #BtkNotebook tab) it is
 * automatically associated with the correct widget, but sometimes
 * (i.e. when the target is a #BtkEntry next to the label) you need to
 * set it explicitly using this function.
 *
 * The target widget will be accelerated by emitting the 
 * BtkWidget::mnemonic-activate signal on it. The default handler for 
 * this signal will activate the widget if there are no mnemonic collisions 
 * and toggle focus between the colliding widgets otherwise.
 **/
void
btk_label_set_mnemonic_widget (BtkLabel  *label,
			       BtkWidget *widget)
{
  g_return_if_fail (BTK_IS_LABEL (label));
  if (widget)
    g_return_if_fail (BTK_IS_WIDGET (widget));

  if (label->mnemonic_widget)
    {
      btk_widget_remove_mnemonic_label (label->mnemonic_widget, BTK_WIDGET (label));
      g_object_weak_unref (B_OBJECT (label->mnemonic_widget),
			   label_mnemonic_widget_weak_notify,
			   label);
    }
  label->mnemonic_widget = widget;
  if (label->mnemonic_widget)
    {
      g_object_weak_ref (B_OBJECT (label->mnemonic_widget),
		         label_mnemonic_widget_weak_notify,
		         label);
      btk_widget_add_mnemonic_label (label->mnemonic_widget, BTK_WIDGET (label));
    }
  
  g_object_notify (B_OBJECT (label), "mnemonic-widget");
}

/**
 * btk_label_get_mnemonic_widget:
 * @label: a #BtkLabel
 *
 * Retrieves the target of the mnemonic (keyboard shortcut) of this
 * label. See btk_label_set_mnemonic_widget().
 *
 * Return value: (transfer none): the target of the label's mnemonic,
 *     or %NULL if none has been set and the default algorithm will be used.
 **/
BtkWidget *
btk_label_get_mnemonic_widget (BtkLabel *label)
{
  g_return_val_if_fail (BTK_IS_LABEL (label), NULL);

  return label->mnemonic_widget;
}

/**
 * btk_label_get_mnemonic_keyval:
 * @label: a #BtkLabel
 *
 * If the label has been set so that it has an mnemonic key this function
 * returns the keyval used for the mnemonic accelerator. If there is no
 * mnemonic set up it returns #BDK_VoidSymbol.
 *
 * Returns: BDK keyval usable for accelerators, or #BDK_VoidSymbol
 **/
guint
btk_label_get_mnemonic_keyval (BtkLabel *label)
{
  g_return_val_if_fail (BTK_IS_LABEL (label), BDK_VoidSymbol);

  return label->mnemonic_keyval;
}

static void
btk_label_set_text_internal (BtkLabel *label,
			     gchar    *str)
{
  g_free (label->text);
  
  label->text = str;

  btk_label_select_rebunnyion_index (label, 0, 0);
}

static void
btk_label_set_label_internal (BtkLabel *label,
			      gchar    *str)
{
  g_free (label->label);
  
  label->label = str;

  g_object_notify (B_OBJECT (label), "label");
}

static void
btk_label_set_use_markup_internal (BtkLabel *label,
				   gboolean  val)
{
  val = val != FALSE;
  if (label->use_markup != val)
    {
      label->use_markup = val;

      g_object_notify (B_OBJECT (label), "use-markup");
    }
}

static void
btk_label_set_use_underline_internal (BtkLabel *label,
				      gboolean val)
{
  val = val != FALSE;
  if (label->use_underline != val)
    {
      label->use_underline = val;

      g_object_notify (B_OBJECT (label), "use-underline");
    }
}

static void
btk_label_compose_effective_attrs (BtkLabel *label)
{
  BangoAttrIterator *iter;
  BangoAttribute    *attr;
  GSList            *iter_attrs, *l;

  if (label->attrs)
    {
      if (label->effective_attrs)
	{
	  if ((iter = bango_attr_list_get_iterator (label->attrs)))
	    {
	      do
		{
		  iter_attrs = bango_attr_iterator_get_attrs (iter);
		  for (l = iter_attrs; l; l = l->next)
		    {
		      attr = l->data;
		      bango_attr_list_insert (label->effective_attrs, attr);
		    }
		  b_slist_free (iter_attrs);
	        }
	      while (bango_attr_iterator_next (iter));
	      bango_attr_iterator_destroy (iter);
	    }
	}
      else
	label->effective_attrs =
	  bango_attr_list_ref (label->attrs);
    }
}

static void
btk_label_set_attributes_internal (BtkLabel      *label,
				   BangoAttrList *attrs)
{
  if (attrs)
    bango_attr_list_ref (attrs);
  
  if (label->attrs)
    bango_attr_list_unref (label->attrs);
  label->attrs = attrs;

  g_object_notify (B_OBJECT (label), "attributes");
}


/* Calculates text, attrs and mnemonic_keyval from
 * label, use_underline and use_markup
 */
static void
btk_label_recalculate (BtkLabel *label)
{
  guint keyval = label->mnemonic_keyval;

  if (label->use_markup)
    btk_label_set_markup_internal (label, label->label, label->use_underline);
  else if (label->use_underline)
    btk_label_set_uline_text_internal (label, label->label);
  else
    {
      if (!label->pattern_set)
        {
          if (label->effective_attrs)
            bango_attr_list_unref (label->effective_attrs);
          label->effective_attrs = NULL;
        }
      btk_label_set_text_internal (label, g_strdup (label->label));
    }

  btk_label_compose_effective_attrs (label);

  if (!label->use_underline)
    label->mnemonic_keyval = BDK_VoidSymbol;

  if (keyval != label->mnemonic_keyval)
    {
      btk_label_setup_mnemonic (label, keyval);
      g_object_notify (B_OBJECT (label), "mnemonic-keyval");
    }

  btk_label_clear_layout (label);
  btk_label_clear_select_info (label);
  btk_widget_queue_resize (BTK_WIDGET (label));
}

/**
 * btk_label_set_text:
 * @label: a #BtkLabel
 * @str: The text you want to set
 *
 * Sets the text within the #BtkLabel widget. It overwrites any text that
 * was there before.  
 *
 * This will also clear any previously set mnemonic accelerators.
 **/
void
btk_label_set_text (BtkLabel    *label,
		    const gchar *str)
{
  g_return_if_fail (BTK_IS_LABEL (label));
  
  g_object_freeze_notify (B_OBJECT (label));

  btk_label_set_label_internal (label, g_strdup (str ? str : ""));
  btk_label_set_use_markup_internal (label, FALSE);
  btk_label_set_use_underline_internal (label, FALSE);
  
  btk_label_recalculate (label);

  g_object_thaw_notify (B_OBJECT (label));
}

/**
 * btk_label_set_attributes:
 * @label: a #BtkLabel
 * @attrs: a #BangoAttrList
 * 
 * Sets a #BangoAttrList; the attributes in the list are applied to the
 * label text. 
 *
 * <note><para>The attributes set with this function will be applied
 * and merged with any other attributes previously effected by way
 * of the #BtkLabel:use-underline or #BtkLabel:use-markup properties.
 * While it is not recommended to mix markup strings with manually set
 * attributes, if you must; know that the attributes will be applied
 * to the label after the markup string is parsed.</para></note>
 **/
void
btk_label_set_attributes (BtkLabel         *label,
                          BangoAttrList    *attrs)
{
  g_return_if_fail (BTK_IS_LABEL (label));

  btk_label_set_attributes_internal (label, attrs);

  btk_label_recalculate (label);

  btk_label_clear_layout (label);
  btk_widget_queue_resize (BTK_WIDGET (label));
}

/**
 * btk_label_get_attributes:
 * @label: a #BtkLabel
 *
 * Gets the attribute list that was set on the label using
 * btk_label_set_attributes(), if any. This function does
 * not reflect attributes that come from the labels markup
 * (see btk_label_set_markup()). If you want to get the
 * effective attributes for the label, use
 * bango_layout_get_attribute (btk_label_get_layout (label)).
 *
 * Return value: (transfer none): the attribute list, or %NULL
 *     if none was set.
 **/
BangoAttrList *
btk_label_get_attributes (BtkLabel *label)
{
  g_return_val_if_fail (BTK_IS_LABEL (label), NULL);

  return label->attrs;
}

/**
 * btk_label_set_label:
 * @label: a #BtkLabel
 * @str: the new text to set for the label
 *
 * Sets the text of the label. The label is interpreted as
 * including embedded underlines and/or Bango markup depending
 * on the values of the #BtkLabel:use-underline" and
 * #BtkLabel:use-markup properties.
 **/
void
btk_label_set_label (BtkLabel    *label,
		     const gchar *str)
{
  g_return_if_fail (BTK_IS_LABEL (label));

  g_object_freeze_notify (B_OBJECT (label));

  btk_label_set_label_internal (label, g_strdup (str ? str : ""));
  btk_label_recalculate (label);

  g_object_thaw_notify (B_OBJECT (label));
}

/**
 * btk_label_get_label:
 * @label: a #BtkLabel
 *
 * Fetches the text from a label widget including any embedded
 * underlines indicating mnemonics and Bango markup. (See
 * btk_label_get_text()).
 *
 * Return value: the text of the label widget. This string is
 *   owned by the widget and must not be modified or freed.
 **/
const gchar *
btk_label_get_label (BtkLabel *label)
{
  g_return_val_if_fail (BTK_IS_LABEL (label), NULL);

  return label->label;
}

typedef struct
{
  BtkLabel *label;
  GList *links;
  GString *new_str;
  BdkColor *link_color;
  BdkColor *visited_link_color;
} UriParserData;

static void
start_element_handler (GMarkupParseContext  *context,
                       const gchar          *element_name,
                       const gchar         **attribute_names,
                       const gchar         **attribute_values,
                       gpointer              user_data,
                       GError              **error)
{
  UriParserData *pdata = user_data;

  if (strcmp (element_name, "a") == 0)
    {
      BtkLabelLink *link;
      const gchar *uri = NULL;
      const gchar *title = NULL;
      gboolean visited = FALSE;
      gint line_number;
      gint char_number;
      gint i;
      BdkColor *color = NULL;

      g_markup_parse_context_get_position (context, &line_number, &char_number);

      for (i = 0; attribute_names[i] != NULL; i++)
        {
          const gchar *attr = attribute_names[i];

          if (strcmp (attr, "href") == 0)
            uri = attribute_values[i];
          else if (strcmp (attr, "title") == 0)
            title = attribute_values[i];
          else
            {
              g_set_error (error,
                           G_MARKUP_ERROR,
                           G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
                           "Attribute '%s' is not allowed on the <a> tag "
                           "on line %d char %d",
                            attr, line_number, char_number);
              return;
            }
        }

      if (uri == NULL)
        {
          g_set_error (error,
                       G_MARKUP_ERROR,
                       G_MARKUP_ERROR_INVALID_CONTENT,
                       "Attribute 'href' was missing on the <a> tag "
                       "on line %d char %d",
                       line_number, char_number);
          return;
        }

      visited = FALSE;
      if (pdata->label->track_links && pdata->label->select_info)
        {
          GList *l;
          for (l = pdata->label->select_info->links; l; l = l->next)
            {
              link = l->data;
              if (strcmp (uri, link->uri) == 0)
                {
                  visited = link->visited;
                  break;
                }
            }
        }

      if (visited)
        color = pdata->visited_link_color;
      else
        color = pdata->link_color;

      g_string_append_printf (pdata->new_str,
                              "<span color=\"#%04x%04x%04x\" underline=\"single\">",
                              color->red,
                              color->green,
                              color->blue);

      link = g_new0 (BtkLabelLink, 1);
      link->uri = g_strdup (uri);
      link->title = g_strdup (title);
      link->visited = visited;
      pdata->links = g_list_append (pdata->links, link);
    }
  else
    {
      gint i;

      g_string_append_c (pdata->new_str, '<');
      g_string_append (pdata->new_str, element_name);

      for (i = 0; attribute_names[i] != NULL; i++)
        {
          const gchar *attr  = attribute_names[i];
          const gchar *value = attribute_values[i];
          gchar *newvalue;

          newvalue = g_markup_escape_text (value, -1);

          g_string_append_c (pdata->new_str, ' ');
          g_string_append (pdata->new_str, attr);
          g_string_append (pdata->new_str, "=\"");
          g_string_append (pdata->new_str, newvalue);
          g_string_append_c (pdata->new_str, '\"');

          g_free (newvalue);
        }
      g_string_append_c (pdata->new_str, '>');
    }
}

static void
end_element_handler (GMarkupParseContext  *context,
                     const gchar          *element_name,
                     gpointer              user_data,
                     GError              **error)
{
  UriParserData *pdata = user_data;

  if (!strcmp (element_name, "a"))
    g_string_append (pdata->new_str, "</span>");
  else
    {
      g_string_append (pdata->new_str, "</");
      g_string_append (pdata->new_str, element_name);
      g_string_append_c (pdata->new_str, '>');
    }
}

static void
text_handler (GMarkupParseContext  *context,
              const gchar          *text,
              gsize                 text_len,
              gpointer              user_data,
              GError              **error)
{
  UriParserData *pdata = user_data;
  gchar *newtext;

  newtext = g_markup_escape_text (text, text_len);
  g_string_append (pdata->new_str, newtext);
  g_free (newtext);
}

static const GMarkupParser markup_parser =
{
  start_element_handler,
  end_element_handler,
  text_handler,
  NULL,
  NULL
};

static gboolean
xml_isspace (gchar c)
{
  return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

static void
link_free (BtkLabelLink *link)
{
  g_free (link->uri);
  g_free (link->title);
  g_free (link);
}

static void
btk_label_get_link_colors (BtkWidget  *widget,
                           BdkColor  **link_color,
                           BdkColor  **visited_link_color)
{
  btk_widget_ensure_style (widget);
  btk_widget_style_get (widget,
                        "link-color", link_color,
                        "visited-link-color", visited_link_color,
                        NULL);
  if (!*link_color)
    *link_color = bdk_color_copy (&default_link_color);
  if (!*visited_link_color)
    *visited_link_color = bdk_color_copy (&default_visited_link_color);
}

static gboolean
parse_uri_markup (BtkLabel     *label,
                  const gchar  *str,
                  gchar       **new_str,
                  GList       **links,
                  GError      **error)
{
  GMarkupParseContext *context = NULL;
  const gchar *p, *end;
  gboolean needs_root = TRUE;
  gsize length;
  UriParserData pdata;

  length = strlen (str);
  p = str;
  end = str + length;

  pdata.label = label;
  pdata.links = NULL;
  pdata.new_str = g_string_sized_new (length);

  btk_label_get_link_colors (BTK_WIDGET (label), &pdata.link_color, &pdata.visited_link_color);

  while (p != end && xml_isspace (*p))
    p++;

  if (end - p >= 8 && strncmp (p, "<markup>", 8) == 0)
    needs_root = FALSE;

  context = g_markup_parse_context_new (&markup_parser, 0, &pdata, NULL);

  if (needs_root)
    {
      if (!g_markup_parse_context_parse (context, "<markup>", -1, error))
        goto failed;
    }

  if (!g_markup_parse_context_parse (context, str, length, error))
    goto failed;

  if (needs_root)
    {
      if (!g_markup_parse_context_parse (context, "</markup>", -1, error))
        goto failed;
    }

  if (!g_markup_parse_context_end_parse (context, error))
    goto failed;

  g_markup_parse_context_free (context);

  *new_str = g_string_free (pdata.new_str, FALSE);
  *links = pdata.links;

  bdk_color_free (pdata.link_color);
  bdk_color_free (pdata.visited_link_color);

  return TRUE;

failed:
  g_markup_parse_context_free (context);
  g_string_free (pdata.new_str, TRUE);
  g_list_foreach (pdata.links, (GFunc)link_free, NULL);
  g_list_free (pdata.links);
  bdk_color_free (pdata.link_color);
  bdk_color_free (pdata.visited_link_color);

  return FALSE;
}

static void
btk_label_ensure_has_tooltip (BtkLabel *label)
{
  GList *l;
  gboolean has_tooltip = FALSE;

  for (l = label->select_info->links; l; l = l->next)
    {
      BtkLabelLink *link = l->data;
      if (link->title)
        {
          has_tooltip = TRUE;
          break;
        }
    }

  btk_widget_set_has_tooltip (BTK_WIDGET (label), has_tooltip);
}

static void
btk_label_set_markup_internal (BtkLabel    *label,
                               const gchar *str,
                               gboolean     with_uline)
{
  BtkLabelPrivate *priv = BTK_LABEL_GET_PRIVATE (label);
  gchar *text = NULL;
  GError *error = NULL;
  BangoAttrList *attrs = NULL;
  gunichar accel_char = 0;
  gchar *new_str;
  GList *links = NULL;

  if (!parse_uri_markup (label, str, &new_str, &links, &error))
    {
      g_warning ("Failed to set text from markup due to error parsing markup: %s",
                 error->message);
      g_error_free (error);
      return;
    }

  btk_label_clear_links (label);
  if (links)
    {
      btk_label_ensure_select_info (label);
      label->select_info->links = links;
      btk_label_ensure_has_tooltip (label);
    }

  if (with_uline)
    {
      gboolean enable_mnemonics;
      gboolean auto_mnemonics;

      g_object_get (btk_widget_get_settings (BTK_WIDGET (label)),
                    "btk-enable-mnemonics", &enable_mnemonics,
                    "btk-auto-mnemonics", &auto_mnemonics,
                    NULL);

      if (!(enable_mnemonics && priv->mnemonics_visible &&
            (!auto_mnemonics ||
             (btk_widget_is_sensitive (BTK_WIDGET (label)) &&
              (!label->mnemonic_widget ||
               btk_widget_is_sensitive (label->mnemonic_widget))))))
        {
          gchar *tmp;
          gchar *pattern;
          guint key;

          if (separate_uline_pattern (new_str, &key, &tmp, &pattern))
            {
              g_free (new_str);
              new_str = tmp;
              g_free (pattern);
            }
        }
    }

  if (!bango_parse_markup (new_str,
                           -1,
                           with_uline ? '_' : 0,
                           &attrs,
                           &text,
                           with_uline ? &accel_char : NULL,
                           &error))
    {
      g_warning ("Failed to set text from markup due to error parsing markup: %s",
                 error->message);
      g_free (new_str);
      g_error_free (error);
      return;
    }

  g_free (new_str);

  if (text)
    btk_label_set_text_internal (label, text);

  if (attrs)
    {
      if (label->effective_attrs)
	bango_attr_list_unref (label->effective_attrs);
      label->effective_attrs = attrs;
    }

  if (accel_char != 0)
    label->mnemonic_keyval = bdk_keyval_to_lower (bdk_unicode_to_keyval (accel_char));
  else
    label->mnemonic_keyval = BDK_VoidSymbol;
}

/**
 * btk_label_set_markup:
 * @label: a #BtkLabel
 * @str: a markup string (see <link linkend="BangoMarkupFormat">Bango markup format</link>)
 * 
 * Parses @str which is marked up with the <link
 * linkend="BangoMarkupFormat">Bango text markup language</link>, setting the
 * label's text and attribute list based on the parse results. If the @str is
 * external data, you may need to escape it with g_markup_escape_text() or
 * g_markup_printf_escaped()<!-- -->:
 * |[
 * char *markup;
 *   
 * markup = g_markup_printf_escaped ("&lt;span style=\"italic\"&gt;&percnt;s&lt;/span&gt;", str);
 * btk_label_set_markup (BTK_LABEL (label), markup);
 * g_free (markup);
 * ]|
 **/
void
btk_label_set_markup (BtkLabel    *label,
                      const gchar *str)
{  
  g_return_if_fail (BTK_IS_LABEL (label));

  g_object_freeze_notify (B_OBJECT (label));

  btk_label_set_label_internal (label, g_strdup (str ? str : ""));
  btk_label_set_use_markup_internal (label, TRUE);
  btk_label_set_use_underline_internal (label, FALSE);
  
  btk_label_recalculate (label);

  g_object_thaw_notify (B_OBJECT (label));
}

/**
 * btk_label_set_markup_with_mnemonic:
 * @label: a #BtkLabel
 * @str: a markup string (see <link linkend="BangoMarkupFormat">Bango markup format</link>)
 * 
 * Parses @str which is marked up with the <link linkend="BangoMarkupFormat">Bango text markup language</link>,
 * setting the label's text and attribute list based on the parse results.
 * If characters in @str are preceded by an underscore, they are underlined
 * indicating that they represent a keyboard accelerator called a mnemonic.
 *
 * The mnemonic key can be used to activate another widget, chosen 
 * automatically, or explicitly using btk_label_set_mnemonic_widget().
 **/
void
btk_label_set_markup_with_mnemonic (BtkLabel    *label,
				    const gchar *str)
{
  g_return_if_fail (BTK_IS_LABEL (label));

  g_object_freeze_notify (B_OBJECT (label));

  btk_label_set_label_internal (label, g_strdup (str ? str : ""));
  btk_label_set_use_markup_internal (label, TRUE);
  btk_label_set_use_underline_internal (label, TRUE);
  
  btk_label_recalculate (label);

  g_object_thaw_notify (B_OBJECT (label));
}

/**
 * btk_label_get_text:
 * @label: a #BtkLabel
 * 
 * Fetches the text from a label widget, as displayed on the
 * screen. This does not include any embedded underlines
 * indicating mnemonics or Bango markup. (See btk_label_get_label())
 * 
 * Return value: the text in the label widget. This is the internal
 *   string used by the label, and must not be modified.
 **/
const gchar *
btk_label_get_text (BtkLabel *label)
{
  g_return_val_if_fail (BTK_IS_LABEL (label), NULL);

  return label->text;
}

static BangoAttrList *
btk_label_pattern_to_attrs (BtkLabel      *label,
			    const gchar   *pattern)
{
  const char *start;
  const char *p = label->text;
  const char *q = pattern;
  BangoAttrList *attrs;

  attrs = bango_attr_list_new ();

  while (1)
    {
      while (*p && *q && *q != '_')
	{
	  p = g_utf8_next_char (p);
	  q++;
	}
      start = p;
      while (*p && *q && *q == '_')
	{
	  p = g_utf8_next_char (p);
	  q++;
	}
      
      if (p > start)
	{
	  BangoAttribute *attr = bango_attr_underline_new (BANGO_UNDERLINE_LOW);
	  attr->start_index = start - label->text;
	  attr->end_index = p - label->text;
	  
	  bango_attr_list_insert (attrs, attr);
	}
      else
	break;
    }

  return attrs;
}

static void
btk_label_set_pattern_internal (BtkLabel    *label,
				const gchar *pattern,
                                gboolean     is_mnemonic)
{
  BtkLabelPrivate *priv = BTK_LABEL_GET_PRIVATE (label);
  BangoAttrList *attrs;
  gboolean enable_mnemonics;
  gboolean auto_mnemonics;

  g_return_if_fail (BTK_IS_LABEL (label));

  if (label->pattern_set)
    return;

  if (is_mnemonic)
    {
      g_object_get (btk_widget_get_settings (BTK_WIDGET (label)),
  		    "btk-enable-mnemonics", &enable_mnemonics,
	 	    "btk-auto-mnemonics", &auto_mnemonics,
		    NULL);

      if (enable_mnemonics && priv->mnemonics_visible && pattern &&
          (!auto_mnemonics ||
           (btk_widget_is_sensitive (BTK_WIDGET (label)) &&
            (!label->mnemonic_widget ||
             btk_widget_is_sensitive (label->mnemonic_widget)))))
        attrs = btk_label_pattern_to_attrs (label, pattern);
      else
        attrs = NULL;
    }
  else
    attrs = btk_label_pattern_to_attrs (label, pattern);

  if (label->effective_attrs)
    bango_attr_list_unref (label->effective_attrs);
  label->effective_attrs = attrs;
}

void
btk_label_set_pattern (BtkLabel	   *label,
		       const gchar *pattern)
{
  g_return_if_fail (BTK_IS_LABEL (label));
  
  label->pattern_set = FALSE;

  if (pattern)
    {
      btk_label_set_pattern_internal (label, pattern, FALSE);
      label->pattern_set = TRUE;
    }
  else
    btk_label_recalculate (label);

  btk_label_clear_layout (label);
  btk_widget_queue_resize (BTK_WIDGET (label));
}


/**
 * btk_label_set_justify:
 * @label: a #BtkLabel
 * @jtype: a #BtkJustification
 *
 * Sets the alignment of the lines in the text of the label relative to
 * each other. %BTK_JUSTIFY_LEFT is the default value when the
 * widget is first created with btk_label_new(). If you instead want
 * to set the alignment of the label as a whole, use
 * btk_misc_set_alignment() instead. btk_label_set_justify() has no
 * effect on labels containing only a single line.
 **/
void
btk_label_set_justify (BtkLabel        *label,
		       BtkJustification jtype)
{
  g_return_if_fail (BTK_IS_LABEL (label));
  g_return_if_fail (jtype >= BTK_JUSTIFY_LEFT && jtype <= BTK_JUSTIFY_FILL);
  
  if ((BtkJustification) label->jtype != jtype)
    {
      label->jtype = jtype;

      /* No real need to be this drastic, but easier than duplicating the code */
      btk_label_clear_layout (label);
      
      g_object_notify (B_OBJECT (label), "justify");
      btk_widget_queue_resize (BTK_WIDGET (label));
    }
}

/**
 * btk_label_get_justify:
 * @label: a #BtkLabel
 *
 * Returns the justification of the label. See btk_label_set_justify().
 *
 * Return value: #BtkJustification
 **/
BtkJustification
btk_label_get_justify (BtkLabel *label)
{
  g_return_val_if_fail (BTK_IS_LABEL (label), 0);

  return label->jtype;
}

/**
 * btk_label_set_ellipsize:
 * @label: a #BtkLabel
 * @mode: a #BangoEllipsizeMode
 *
 * Sets the mode used to ellipsize (add an ellipsis: "...") to the text 
 * if there is not enough space to render the entire string.
 *
 * Since: 2.6
 **/
void
btk_label_set_ellipsize (BtkLabel          *label,
			 BangoEllipsizeMode mode)
{
  g_return_if_fail (BTK_IS_LABEL (label));
  g_return_if_fail (mode >= BANGO_ELLIPSIZE_NONE && mode <= BANGO_ELLIPSIZE_END);
  
  if ((BangoEllipsizeMode) label->ellipsize != mode)
    {
      label->ellipsize = mode;

      /* No real need to be this drastic, but easier than duplicating the code */
      btk_label_clear_layout (label);
      
      g_object_notify (B_OBJECT (label), "ellipsize");
      btk_widget_queue_resize (BTK_WIDGET (label));
    }
}

/**
 * btk_label_get_ellipsize:
 * @label: a #BtkLabel
 *
 * Returns the ellipsizing position of the label. See btk_label_set_ellipsize().
 *
 * Return value: #BangoEllipsizeMode
 *
 * Since: 2.6
 **/
BangoEllipsizeMode
btk_label_get_ellipsize (BtkLabel *label)
{
  g_return_val_if_fail (BTK_IS_LABEL (label), BANGO_ELLIPSIZE_NONE);

  return label->ellipsize;
}

/**
 * btk_label_set_width_chars:
 * @label: a #BtkLabel
 * @n_chars: the new desired width, in characters.
 * 
 * Sets the desired width in characters of @label to @n_chars.
 * 
 * Since: 2.6
 **/
void
btk_label_set_width_chars (BtkLabel *label,
			   gint      n_chars)
{
  BtkLabelPrivate *priv;

  g_return_if_fail (BTK_IS_LABEL (label));

  priv = BTK_LABEL_GET_PRIVATE (label);

  if (priv->width_chars != n_chars)
    {
      priv->width_chars = n_chars;
      g_object_notify (B_OBJECT (label), "width-chars");
      btk_label_invalidate_wrap_width (label);
      btk_widget_queue_resize (BTK_WIDGET (label));
    }
}

/**
 * btk_label_get_width_chars:
 * @label: a #BtkLabel
 * 
 * Retrieves the desired width of @label, in characters. See
 * btk_label_set_width_chars().
 * 
 * Return value: the width of the label in characters.
 * 
 * Since: 2.6
 **/
gint
btk_label_get_width_chars (BtkLabel *label)
{
  g_return_val_if_fail (BTK_IS_LABEL (label), -1);

  return BTK_LABEL_GET_PRIVATE (label)->width_chars;
}

/**
 * btk_label_set_max_width_chars:
 * @label: a #BtkLabel
 * @n_chars: the new desired maximum width, in characters.
 * 
 * Sets the desired maximum width in characters of @label to @n_chars.
 * 
 * Since: 2.6
 **/
void
btk_label_set_max_width_chars (BtkLabel *label,
			       gint      n_chars)
{
  BtkLabelPrivate *priv;

  g_return_if_fail (BTK_IS_LABEL (label));

  priv = BTK_LABEL_GET_PRIVATE (label);

  if (priv->max_width_chars != n_chars)
    {
      priv->max_width_chars = n_chars;

      g_object_notify (B_OBJECT (label), "max-width-chars");
      btk_label_invalidate_wrap_width (label);
      btk_widget_queue_resize (BTK_WIDGET (label));
    }
}

/**
 * btk_label_get_max_width_chars:
 * @label: a #BtkLabel
 * 
 * Retrieves the desired maximum width of @label, in characters. See
 * btk_label_set_width_chars().
 * 
 * Return value: the maximum width of the label in characters.
 * 
 * Since: 2.6
 **/
gint
btk_label_get_max_width_chars (BtkLabel *label)
{
  g_return_val_if_fail (BTK_IS_LABEL (label), -1);

  return BTK_LABEL_GET_PRIVATE (label)->max_width_chars;
}

/**
 * btk_label_set_line_wrap:
 * @label: a #BtkLabel
 * @wrap: the setting
 *
 * Toggles line wrapping within the #BtkLabel widget. %TRUE makes it break
 * lines if text exceeds the widget's size. %FALSE lets the text get cut off
 * by the edge of the widget if it exceeds the widget size.
 *
 * Note that setting line wrapping to %TRUE does not make the label
 * wrap at its parent container's width, because BTK+ widgets
 * conceptually can't make their requisition depend on the parent
 * container's size. For a label that wraps at a specific position,
 * set the label's width using btk_widget_set_size_request().
 **/
void
btk_label_set_line_wrap (BtkLabel *label,
			 gboolean  wrap)
{
  g_return_if_fail (BTK_IS_LABEL (label));
  
  wrap = wrap != FALSE;
  
  if (label->wrap != wrap)
    {
      label->wrap = wrap;

      btk_label_clear_layout (label);
      btk_widget_queue_resize (BTK_WIDGET (label));
      g_object_notify (B_OBJECT (label), "wrap");
    }
}

/**
 * btk_label_get_line_wrap:
 * @label: a #BtkLabel
 *
 * Returns whether lines in the label are automatically wrapped. 
 * See btk_label_set_line_wrap().
 *
 * Return value: %TRUE if the lines of the label are automatically wrapped.
 */
gboolean
btk_label_get_line_wrap (BtkLabel *label)
{
  g_return_val_if_fail (BTK_IS_LABEL (label), FALSE);

  return label->wrap;
}

/**
 * btk_label_set_line_wrap_mode:
 * @label: a #BtkLabel
 * @wrap_mode: the line wrapping mode
 *
 * If line wrapping is on (see btk_label_set_line_wrap()) this controls how
 * the line wrapping is done. The default is %BANGO_WRAP_WORD which means
 * wrap on word boundaries.
 *
 * Since: 2.10
 **/
void
btk_label_set_line_wrap_mode (BtkLabel *label,
			      BangoWrapMode wrap_mode)
{
  g_return_if_fail (BTK_IS_LABEL (label));
  
  if (label->wrap_mode != wrap_mode)
    {
      label->wrap_mode = wrap_mode;
      g_object_notify (B_OBJECT (label), "wrap-mode");
      
      btk_widget_queue_resize (BTK_WIDGET (label));
    }
}

/**
 * btk_label_get_line_wrap_mode:
 * @label: a #BtkLabel
 *
 * Returns line wrap mode used by the label. See btk_label_set_line_wrap_mode().
 *
 * Return value: %TRUE if the lines of the label are automatically wrapped.
 *
 * Since: 2.10
 */
BangoWrapMode
btk_label_get_line_wrap_mode (BtkLabel *label)
{
  g_return_val_if_fail (BTK_IS_LABEL (label), FALSE);

  return label->wrap_mode;
}


void
btk_label_get (BtkLabel *label,
	       gchar   **str)
{
  g_return_if_fail (BTK_IS_LABEL (label));
  g_return_if_fail (str != NULL);
  
  *str = label->text;
}

static void
btk_label_destroy (BtkObject *object)
{
  BtkLabel *label = BTK_LABEL (object);

  btk_label_set_mnemonic_widget (label, NULL);

  BTK_OBJECT_CLASS (btk_label_parent_class)->destroy (object);
}

static void
btk_label_finalize (BObject *object)
{
  BtkLabel *label = BTK_LABEL (object);

  g_free (label->label);
  g_free (label->text);

  if (label->layout)
    g_object_unref (label->layout);

  if (label->attrs)
    bango_attr_list_unref (label->attrs);

  if (label->effective_attrs)
    bango_attr_list_unref (label->effective_attrs);

  btk_label_clear_links (label);
  g_free (label->select_info);

  B_OBJECT_CLASS (btk_label_parent_class)->finalize (object);
}

static void
btk_label_clear_layout (BtkLabel *label)
{
  if (label->layout)
    {
      g_object_unref (label->layout);
      label->layout = NULL;

      //btk_label_clear_links (label);
    }
}

static gint
get_label_char_width (BtkLabel *label)
{
  BtkLabelPrivate *priv;
  BangoContext *context;
  BangoFontMetrics *metrics;
  gint char_width, digit_width, char_pixels, w;
  
  priv = BTK_LABEL_GET_PRIVATE (label);
  
  context = bango_layout_get_context (label->layout);
  metrics = bango_context_get_metrics (context, BTK_WIDGET (label)->style->font_desc, 
				       bango_context_get_language (context));
  
  char_width = bango_font_metrics_get_approximate_char_width (metrics);
  digit_width = bango_font_metrics_get_approximate_digit_width (metrics);
  char_pixels = MAX (char_width, digit_width);
  bango_font_metrics_unref (metrics);
  
  if (priv->width_chars < 0)
    {
      BangoRectangle rect;
      
      bango_layout_set_width (label->layout, -1);
      bango_layout_get_extents (label->layout, NULL, &rect);
      
      w = char_pixels * MAX (priv->max_width_chars, 3);
      w = MIN (rect.width, w);
    }
  else
    {
      /* enforce minimum width for ellipsized labels at ~3 chars */
      w = char_pixels * MAX (priv->width_chars, 3);
    }
  
  return w;
}

static void
btk_label_invalidate_wrap_width (BtkLabel *label)
{
  BtkLabelPrivate *priv;

  priv = BTK_LABEL_GET_PRIVATE (label);

  priv->wrap_width = -1;
}

static gint
get_label_wrap_width (BtkLabel *label)
{
  BtkLabelPrivate *priv;

  priv = BTK_LABEL_GET_PRIVATE (label);
  
  if (priv->wrap_width < 0)
    {
      if (priv->width_chars > 0 || priv->max_width_chars > 0)
	priv->wrap_width = get_label_char_width (label);
      else
	{
	  BangoLayout *layout;
  
	  layout = btk_widget_create_bango_layout (BTK_WIDGET (label), 
						   "This long string gives a good enough length for any line to have.");
	  bango_layout_get_size (layout, &priv->wrap_width, NULL);
	  g_object_unref (layout);
	}
    }

  return priv->wrap_width;
}

static void
btk_label_ensure_layout (BtkLabel *label)
{
  BtkWidget *widget;
  BangoRectangle logical_rect;
  gboolean rtl;

  widget = BTK_WIDGET (label);

  rtl = btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL;

  if (!label->layout)
    {
      BangoAlignment align = BANGO_ALIGN_LEFT; /* Quiet gcc */
      gdouble angle = btk_label_get_angle (label);

      if (angle != 0.0 && !label->wrap && !label->ellipsize && !label->select_info)
	{
	  /* We rotate the standard singleton BangoContext for the widget,
	   * depending on the fact that it's meant pretty much exclusively
	   * for our use.
	   */
	  BangoMatrix matrix = BANGO_MATRIX_INIT;
	  
	  bango_matrix_rotate (&matrix, angle);

	  bango_context_set_matrix (btk_widget_get_bango_context (widget), &matrix);
	  
	  label->have_transform = TRUE;
	}
      else 
	{
	  if (label->have_transform)
	    bango_context_set_matrix (btk_widget_get_bango_context (widget), NULL);

	  label->have_transform = FALSE;
	}

      label->layout = btk_widget_create_bango_layout (widget, label->text);

      if (label->effective_attrs)
	bango_layout_set_attributes (label->layout, label->effective_attrs);

      btk_label_rescan_links (label);

      switch (label->jtype)
	{
	case BTK_JUSTIFY_LEFT:
	  align = rtl ? BANGO_ALIGN_RIGHT : BANGO_ALIGN_LEFT;
	  break;
	case BTK_JUSTIFY_RIGHT:
	  align = rtl ? BANGO_ALIGN_LEFT : BANGO_ALIGN_RIGHT;
	  break;
	case BTK_JUSTIFY_CENTER:
	  align = BANGO_ALIGN_CENTER;
	  break;
	case BTK_JUSTIFY_FILL:
	  align = rtl ? BANGO_ALIGN_RIGHT : BANGO_ALIGN_LEFT;
	  bango_layout_set_justify (label->layout, TRUE);
	  break;
	default:
	  g_assert_not_reached();
	}

      bango_layout_set_alignment (label->layout, align);
      bango_layout_set_ellipsize (label->layout, label->ellipsize);
      bango_layout_set_single_paragraph_mode (label->layout, label->single_line_mode);

      if (label->ellipsize)
	bango_layout_set_width (label->layout, 
				widget->allocation.width * BANGO_SCALE);
      else if (label->wrap)
	{
	  BtkWidgetAuxInfo *aux_info;
	  gint longest_paragraph;
	  gint width, height;

	  bango_layout_set_wrap (label->layout, label->wrap_mode);
	  
	  aux_info = _btk_widget_get_aux_info (widget, FALSE);
	  if (aux_info && aux_info->width > 0)
	    bango_layout_set_width (label->layout, aux_info->width * BANGO_SCALE);
	  else
	    {
	      BdkScreen *screen = btk_widget_get_screen (BTK_WIDGET (label));
	      gint wrap_width;
	      
	      bango_layout_set_width (label->layout, -1);
	      bango_layout_get_extents (label->layout, NULL, &logical_rect);

	      width = logical_rect.width;
	      
	      /* Try to guess a reasonable maximum width */
	      longest_paragraph = width;

	      wrap_width = get_label_wrap_width (label);
	      width = MIN (width, wrap_width);
	      width = MIN (width,
			   BANGO_SCALE * (bdk_screen_get_width (screen) + 1) / 2);
	      
	      bango_layout_set_width (label->layout, width);
	      bango_layout_get_extents (label->layout, NULL, &logical_rect);
	      width = logical_rect.width;
	      height = logical_rect.height;
	      
	      /* Unfortunately, the above may leave us with a very unbalanced looking paragraph,
	       * so we try short search for a narrower width that leaves us with the same height
	       */
	      if (longest_paragraph > 0)
		{
		  gint nlines, perfect_width;
		  
		  nlines = bango_layout_get_line_count (label->layout);
		  perfect_width = (longest_paragraph + nlines - 1) / nlines;
		  
		  if (perfect_width < width)
		    {
		      bango_layout_set_width (label->layout, perfect_width);
		      bango_layout_get_extents (label->layout, NULL, &logical_rect);
		      
		      if (logical_rect.height <= height)
			width = logical_rect.width;
		      else
			{
			  gint mid_width = (perfect_width + width) / 2;
			  
			  if (mid_width > perfect_width)
			    {
			      bango_layout_set_width (label->layout, mid_width);
			      bango_layout_get_extents (label->layout, NULL, &logical_rect);
			      
			      if (logical_rect.height <= height)
				width = logical_rect.width;
			    }
			}
		    }
		}
	      bango_layout_set_width (label->layout, width);
	    }
	}
      else /* !label->wrap */
	bango_layout_set_width (label->layout, -1);
    }
}

static void
btk_label_size_request (BtkWidget      *widget,
			BtkRequisition *requisition)
{
  BtkLabel *label = BTK_LABEL (widget);
  BtkLabelPrivate *priv;
  gint width, height;
  BangoRectangle logical_rect;
  BtkWidgetAuxInfo *aux_info;

  priv = BTK_LABEL_GET_PRIVATE (widget);

  /*  
   * If word wrapping is on, then the height requisition can depend
   * on:
   *
   *   - Any width set on the widget via btk_widget_set_size_request().
   *   - The padding of the widget (xpad, set by btk_misc_set_padding)
   *
   * Instead of trying to detect changes to these quantities, if we
   * are wrapping, we just rewrap for each size request. Since
   * size requisitions are cached by the BTK+ core, this is not
   * expensive.
   */

  if (label->wrap)
    btk_label_clear_layout (label);

  btk_label_ensure_layout (label);

  width = label->misc.xpad * 2;
  height = label->misc.ypad * 2;

  aux_info = _btk_widget_get_aux_info (widget, FALSE);

  if (label->have_transform)
    {
      BangoRectangle rect;
      BangoContext *context = bango_layout_get_context (label->layout);
      const BangoMatrix *matrix = bango_context_get_matrix (context);

      bango_layout_get_extents (label->layout, NULL, &rect);
      bango_matrix_transform_rectangle (matrix, &rect);
      bango_extents_to_pixels (&rect, NULL);
      
      requisition->width = width + rect.width;
      requisition->height = height + rect.height;

      return;
    }
  else
    bango_layout_get_extents (label->layout, NULL, &logical_rect);

  if ((label->wrap || label->ellipsize || 
       priv->width_chars > 0 || priv->max_width_chars > 0) && 
      aux_info && aux_info->width > 0)
    width += aux_info->width;
  else if (label->ellipsize || priv->width_chars > 0 || priv->max_width_chars > 0)
    {
      width += BANGO_PIXELS (get_label_char_width (label));
    }
  else
    width += BANGO_PIXELS (logical_rect.width);

  if (label->single_line_mode)
    {
      BangoContext *context;
      BangoFontMetrics *metrics;
      gint ascent, descent;

      context = bango_layout_get_context (label->layout);
      metrics = bango_context_get_metrics (context, widget->style->font_desc,
                                           bango_context_get_language (context));

      ascent = bango_font_metrics_get_ascent (metrics);
      descent = bango_font_metrics_get_descent (metrics);
      bango_font_metrics_unref (metrics);
    
      height += BANGO_PIXELS (ascent + descent);
    }
  else
    height += BANGO_PIXELS (logical_rect.height);

  requisition->width = width;
  requisition->height = height;
}

static void
btk_label_size_allocate (BtkWidget     *widget,
                         BtkAllocation *allocation)
{
  BtkLabel *label;

  label = BTK_LABEL (widget);

  BTK_WIDGET_CLASS (btk_label_parent_class)->size_allocate (widget, allocation);

  if (label->ellipsize)
    {
      if (label->layout)
	{
	  gint width;
	  BangoRectangle logical;

	  width = (allocation->width - label->misc.xpad * 2) * BANGO_SCALE;

	  bango_layout_set_width (label->layout, -1);
	  bango_layout_get_extents (label->layout, NULL, &logical);

	  if (logical.width > width)
	    bango_layout_set_width (label->layout, width);
	}
    }

  if (label->select_info && label->select_info->window)
    {
      bdk_window_move_resize (label->select_info->window,
                              allocation->x,
                              allocation->y,
                              allocation->width,
                              allocation->height);
    }
}

static void
btk_label_update_cursor (BtkLabel *label)
{
  BtkWidget *widget;

  if (!label->select_info)
    return;

  widget = BTK_WIDGET (label);

  if (btk_widget_get_realized (widget))
    {
      BdkDisplay *display;
      BdkCursor *cursor;

      if (btk_widget_is_sensitive (widget))
        {
          display = btk_widget_get_display (widget);

          if (label->select_info->active_link)
            cursor = bdk_cursor_new_for_display (display, BDK_HAND2);
          else if (label->select_info->selectable)
            cursor = bdk_cursor_new_for_display (display, BDK_XTERM);
          else
            cursor = NULL;
        }
      else
        cursor = NULL;

      bdk_window_set_cursor (label->select_info->window, cursor);

      if (cursor)
        bdk_cursor_unref (cursor);
    }
}

static void
btk_label_state_changed (BtkWidget   *widget,
                         BtkStateType prev_state)
{
  BtkLabel *label = BTK_LABEL (widget);

  if (label->select_info)
    {
      btk_label_select_rebunnyion (label, 0, 0);
      btk_label_update_cursor (label);
    }

  if (BTK_WIDGET_CLASS (btk_label_parent_class)->state_changed)
    BTK_WIDGET_CLASS (btk_label_parent_class)->state_changed (widget, prev_state);
}

static void
btk_label_style_set (BtkWidget *widget,
		     BtkStyle  *previous_style)
{
  BtkLabel *label = BTK_LABEL (widget);

  /* We have to clear the layout, fonts etc. may have changed */
  btk_label_clear_layout (label);
  btk_label_invalidate_wrap_width (label);
}

static void 
btk_label_direction_changed (BtkWidget        *widget,
			     BtkTextDirection previous_dir)
{
  BtkLabel *label = BTK_LABEL (widget);

  if (label->layout)
    bango_layout_context_changed (label->layout);

  BTK_WIDGET_CLASS (btk_label_parent_class)->direction_changed (widget, previous_dir);
}

static void
get_layout_location (BtkLabel  *label,
                     gint      *xp,
                     gint      *yp)
{
  BtkMisc *misc;
  BtkWidget *widget; 
  BtkLabelPrivate *priv;
  gfloat xalign;
  gint req_width, x, y;
  BangoRectangle logical;
  
  misc = BTK_MISC (label);
  widget = BTK_WIDGET (label);
  priv = BTK_LABEL_GET_PRIVATE (label);

  if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_LTR)
    xalign = misc->xalign;
  else
    xalign = 1.0 - misc->xalign;

  bango_layout_get_pixel_extents (label->layout, NULL, &logical);

  if (label->ellipsize || priv->width_chars > 0)
    {
      int width;

      width = bango_layout_get_width (label->layout);

      req_width = logical.width;
      if (width != -1)
	req_width = MIN(BANGO_PIXELS (width), req_width);
      req_width += 2 * misc->xpad;
    }
  else
    req_width = widget->requisition.width;

  x = floor (widget->allocation.x + (gint)misc->xpad +
	      xalign * (widget->allocation.width - req_width));

  if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_LTR)
    x = MAX (x, widget->allocation.x + misc->xpad);
  else
    x = MIN (x, widget->allocation.x + widget->allocation.width - misc->xpad);
  x -= logical.x;

  /* bgo#315462 - For single-line labels, *do* align the requisition with
   * respect to the allocation, even if we are under-allocated.  For multi-line
   * labels, always show the top of the text when they are under-allocated.  The
   * rationale is this:
   *
   * - Single-line labels appear in BtkButtons, and it is very easy to get them
   *   to be smaller than their requisition.  The button may clip the label, but
   *   the label will still be able to show most of itself and the focus
   *   rectangle.  Also, it is fairly easy to read a single line of clipped text.
   *
   * - Multi-line labels should not be clipped to showing "something in the
   *   middle".  You want to read the first line, at least, to get some context.
   */
  if (bango_layout_get_line_count (label->layout) == 1)
    y = floor (widget->allocation.y + (gint)misc->ypad 
	       + (widget->allocation.height - widget->requisition.height) * misc->yalign);
  else
    y = floor (widget->allocation.y + (gint)misc->ypad 
	       + MAX (((widget->allocation.height - widget->requisition.height) * misc->yalign),
		      0));

  if (xp)
    *xp = x;

  if (yp)
    *yp = y;
}

static void
draw_insertion_cursor (BtkLabel      *label,
		       BdkRectangle  *cursor_location,
		       gboolean       is_primary,
		       BangoDirection direction,
		       gboolean       draw_arrow)
{
  BtkWidget *widget = BTK_WIDGET (label);
  BtkTextDirection text_dir;

  if (direction == BANGO_DIRECTION_LTR)
    text_dir = BTK_TEXT_DIR_LTR;
  else
    text_dir = BTK_TEXT_DIR_RTL;

  btk_draw_insertion_cursor (widget, widget->window, &(widget->allocation),
			     cursor_location,
			     is_primary, text_dir, draw_arrow);
}

static BangoDirection
get_cursor_direction (BtkLabel *label)
{
  GSList *l;

  g_assert (label->select_info);

  btk_label_ensure_layout (label);

  for (l = bango_layout_get_lines_readonly (label->layout); l; l = l->next)
    {
      BangoLayoutLine *line = l->data;

      /* If label->select_info->selection_end is at the very end of
       * the line, we don't know if the cursor is on this line or
       * the next without looking ahead at the next line. (End
       * of paragraph is different from line break.) But it's
       * definitely in this paragraph, which is good enough
       * to figure out the resolved direction.
       */
       if (line->start_index + line->length >= label->select_info->selection_end)
	return line->resolved_dir;
    }

  return BANGO_DIRECTION_LTR;
}

static void
btk_label_draw_cursor (BtkLabel  *label, gint xoffset, gint yoffset)
{
  BtkWidget *widget;

  if (label->select_info == NULL)
    return;

  widget = BTK_WIDGET (label);
  
  if (btk_widget_is_drawable (widget))
    {
      BangoDirection keymap_direction;
      BangoDirection cursor_direction;
      BangoRectangle strong_pos, weak_pos;
      gboolean split_cursor;
      BangoRectangle *cursor1 = NULL;
      BangoRectangle *cursor2 = NULL;
      BdkRectangle cursor_location;
      BangoDirection dir1 = BANGO_DIRECTION_NEUTRAL;
      BangoDirection dir2 = BANGO_DIRECTION_NEUTRAL;

      keymap_direction = bdk_keymap_get_direction (bdk_keymap_get_for_display (btk_widget_get_display (widget)));
      cursor_direction = get_cursor_direction (label);

      btk_label_ensure_layout (label);
      
      bango_layout_get_cursor_pos (label->layout, label->select_info->selection_end,
				   &strong_pos, &weak_pos);

      g_object_get (btk_widget_get_settings (widget),
		    "btk-split-cursor", &split_cursor,
		    NULL);

      dir1 = cursor_direction;
      
      if (split_cursor)
	{
	  cursor1 = &strong_pos;

	  if (strong_pos.x != weak_pos.x ||
	      strong_pos.y != weak_pos.y)
	    {
	      dir2 = (cursor_direction == BANGO_DIRECTION_LTR) ? BANGO_DIRECTION_RTL : BANGO_DIRECTION_LTR;
	      cursor2 = &weak_pos;
	    }
	}
      else
	{
	  if (keymap_direction == cursor_direction)
	    cursor1 = &strong_pos;
	  else
	    cursor1 = &weak_pos;
	}
      
      cursor_location.x = xoffset + BANGO_PIXELS (cursor1->x);
      cursor_location.y = yoffset + BANGO_PIXELS (cursor1->y);
      cursor_location.width = 0;
      cursor_location.height = BANGO_PIXELS (cursor1->height);

      draw_insertion_cursor (label,
			     &cursor_location, TRUE, dir1,
			     dir2 != BANGO_DIRECTION_NEUTRAL);
      
      if (dir2 != BANGO_DIRECTION_NEUTRAL)
	{
	  cursor_location.x = xoffset + BANGO_PIXELS (cursor2->x);
	  cursor_location.y = yoffset + BANGO_PIXELS (cursor2->y);
	  cursor_location.width = 0;
	  cursor_location.height = BANGO_PIXELS (cursor2->height);

	  draw_insertion_cursor (label,
				 &cursor_location, FALSE, dir2,
				 TRUE);
	}
    }
}

static BtkLabelLink *
btk_label_get_focus_link (BtkLabel *label)
{
  BtkLabelSelectionInfo *info = label->select_info;
  GList *l;

  if (!info)
    return NULL;

  if (info->selection_anchor != info->selection_end)
    return NULL;

  for (l = info->links; l; l = l->next)
    {
      BtkLabelLink *link = l->data;
      if (link->start <= info->selection_anchor &&
          info->selection_anchor <= link->end)
        return link;
    }

  return NULL;
}

static gint
btk_label_expose (BtkWidget      *widget,
		  BdkEventExpose *event)
{
  BtkLabel *label = BTK_LABEL (widget);
  BtkLabelSelectionInfo *info = label->select_info;
  gint x, y;

  btk_label_ensure_layout (label);
  
  if (btk_widget_get_visible (widget) && btk_widget_get_mapped (widget) &&
      label->text && (*label->text != '\0'))
    {
      get_layout_location (label, &x, &y);

      btk_paint_layout (widget->style,
                        widget->window,
                        btk_widget_get_state (widget),
			FALSE,
                        &event->area,
                        widget,
                        "label",
                        x, y,
                        label->layout);

      if (info &&
          (info->selection_anchor != info->selection_end))
        {
          gint range[2];
          BdkRebunnyion *clip;
	  BtkStateType state;
          bairo_t *cr;

          range[0] = info->selection_anchor;
          range[1] = info->selection_end;

          if (range[0] > range[1])
            {
              gint tmp = range[0];
              range[0] = range[1];
              range[1] = tmp;
            }

          clip = bdk_bango_layout_get_clip_rebunnyion (label->layout,
                                                   x, y,
                                                   range,
                                                   1);
	  bdk_rebunnyion_intersect (clip, event->rebunnyion);

         /* FIXME should use btk_paint, but it can't use a clip
           * rebunnyion
           */

          cr = bdk_bairo_create (event->window);

          bdk_bairo_rebunnyion (cr, clip);
          bairo_clip (cr);

	  state = BTK_STATE_SELECTED;
	  if (!btk_widget_has_focus (widget))
	    state = BTK_STATE_ACTIVE;

          bdk_bairo_set_source_color (cr, &widget->style->base[state]);
          bairo_paint (cr);

          bdk_bairo_set_source_color (cr, &widget->style->text[state]);
          bairo_move_to (cr, x, y);
          _btk_bango_fill_layout (cr, label->layout);

          bairo_destroy (cr);
          bdk_rebunnyion_destroy (clip);
        }
      else if (info)
        {
          BtkLabelLink *focus_link;
          BtkLabelLink *active_link;
          gint range[2];
          BdkRebunnyion *clip;
          BdkRectangle rect;
          BdkColor *text_color;
          BdkColor *base_color;
          BdkColor *link_color;
          BdkColor *visited_link_color;

          if (info->selectable && btk_widget_has_focus (widget))
	    btk_label_draw_cursor (label, x, y);

          focus_link = btk_label_get_focus_link (label);
          active_link = info->active_link;


          if (active_link)
            {
              bairo_t *cr;

              range[0] = active_link->start;
              range[1] = active_link->end;

              cr = bdk_bairo_create (event->window);

              bdk_bairo_rebunnyion (cr, event->rebunnyion);
              bairo_clip (cr);

              clip = bdk_bango_layout_get_clip_rebunnyion (label->layout,
                                                       x, y,
                                                       range,
                                                       1);
              bdk_bairo_rebunnyion (cr, clip);
              bairo_clip (cr);
              bdk_rebunnyion_destroy (clip);

              btk_label_get_link_colors (widget, &link_color, &visited_link_color);
              if (active_link->visited)
                text_color = visited_link_color;
              else
                text_color = link_color;
              if (info->link_clicked)
                base_color = &widget->style->base[BTK_STATE_ACTIVE];
              else
                base_color = &widget->style->base[BTK_STATE_PRELIGHT];

              bdk_bairo_set_source_color (cr, base_color);
              bairo_paint (cr);

              bdk_bairo_set_source_color (cr, text_color);
              bairo_move_to (cr, x, y);
              _btk_bango_fill_layout (cr, label->layout);

              bdk_color_free (link_color);
              bdk_color_free (visited_link_color);

              bairo_destroy (cr);
            }

          if (focus_link && btk_widget_has_focus (widget))
            {
              range[0] = focus_link->start;
              range[1] = focus_link->end;

              clip = bdk_bango_layout_get_clip_rebunnyion (label->layout,
                                                       x, y,
                                                       range,
                                                       1);
              bdk_rebunnyion_get_clipbox (clip, &rect);

              btk_paint_focus (widget->style, widget->window, btk_widget_get_state (widget),
                               &event->area, widget, "label",
                               rect.x, rect.y, rect.width, rect.height);

              bdk_rebunnyion_destroy (clip);
            }
        }
    }

  return FALSE;
}

static gboolean
separate_uline_pattern (const gchar  *str,
                        guint        *accel_key,
                        gchar       **new_str,
                        gchar       **pattern)
{
  gboolean underscore;
  const gchar *src;
  gchar *dest;
  gchar *pattern_dest;

  *accel_key = BDK_VoidSymbol;
  *new_str = g_new (gchar, strlen (str) + 1);
  *pattern = g_new (gchar, g_utf8_strlen (str, -1) + 1);

  underscore = FALSE;

  src = str;
  dest = *new_str;
  pattern_dest = *pattern;

  while (*src)
    {
      gunichar c;
      const gchar *next_src;

      c = g_utf8_get_char (src);
      if (c == (gunichar)-1)
	{
	  g_warning ("Invalid input string");
	  g_free (*new_str);
	  g_free (*pattern);

	  return FALSE;
	}
      next_src = g_utf8_next_char (src);

      if (underscore)
	{
	  if (c == '_')
	    *pattern_dest++ = ' ';
	  else
	    {
	      *pattern_dest++ = '_';
	      if (*accel_key == BDK_VoidSymbol)
		*accel_key = bdk_keyval_to_lower (bdk_unicode_to_keyval (c));
	    }

	  while (src < next_src)
	    *dest++ = *src++;

	  underscore = FALSE;
	}
      else
	{
	  if (c == '_')
	    {
	      underscore = TRUE;
	      src = next_src;
	    }
	  else
	    {
	      while (src < next_src)
		*dest++ = *src++;

	      *pattern_dest++ = ' ';
	    }
	}
    }

  *dest = 0;
  *pattern_dest = 0;

  return TRUE;
}

static void
btk_label_set_uline_text_internal (BtkLabel    *label,
				   const gchar *str)
{
  guint accel_key = BDK_VoidSymbol;
  gchar *new_str;
  gchar *pattern;

  g_return_if_fail (BTK_IS_LABEL (label));
  g_return_if_fail (str != NULL);

  /* Split text into the base text and a separate pattern
   * of underscores.
   */
  if (!separate_uline_pattern (str, &accel_key, &new_str, &pattern))
    return;

  btk_label_set_text_internal (label, new_str);
  btk_label_set_pattern_internal (label, pattern, TRUE);
  label->mnemonic_keyval = accel_key;

  g_free (pattern);
}

guint
btk_label_parse_uline (BtkLabel    *label,
		       const gchar *str)
{
  guint keyval;
  
  g_return_val_if_fail (BTK_IS_LABEL (label), BDK_VoidSymbol);
  g_return_val_if_fail (str != NULL, BDK_VoidSymbol);

  g_object_freeze_notify (B_OBJECT (label));
  
  btk_label_set_label_internal (label, g_strdup (str ? str : ""));
  btk_label_set_use_markup_internal (label, FALSE);
  btk_label_set_use_underline_internal (label, TRUE);
  
  btk_label_recalculate (label);

  keyval = label->mnemonic_keyval;
  if (keyval != BDK_VoidSymbol)
    {
      label->mnemonic_keyval = BDK_VoidSymbol;
      btk_label_setup_mnemonic (label, keyval);
      g_object_notify (B_OBJECT (label), "mnemonic-keyval");
    }
  
  g_object_thaw_notify (B_OBJECT (label));

  return keyval;
}

/**
 * btk_label_set_text_with_mnemonic:
 * @label: a #BtkLabel
 * @str: a string
 * 
 * Sets the label's text from the string @str.
 * If characters in @str are preceded by an underscore, they are underlined
 * indicating that they represent a keyboard accelerator called a mnemonic.
 * The mnemonic key can be used to activate another widget, chosen 
 * automatically, or explicitly using btk_label_set_mnemonic_widget().
 **/
void
btk_label_set_text_with_mnemonic (BtkLabel    *label,
				  const gchar *str)
{
  g_return_if_fail (BTK_IS_LABEL (label));
  g_return_if_fail (str != NULL);

  g_object_freeze_notify (B_OBJECT (label));

  btk_label_set_label_internal (label, g_strdup (str ? str : ""));
  btk_label_set_use_markup_internal (label, FALSE);
  btk_label_set_use_underline_internal (label, TRUE);
  
  btk_label_recalculate (label);

  g_object_thaw_notify (B_OBJECT (label));
}

static void
btk_label_realize (BtkWidget *widget)
{
  BtkLabel *label;

  label = BTK_LABEL (widget);

  BTK_WIDGET_CLASS (btk_label_parent_class)->realize (widget);

  if (label->select_info)
    btk_label_create_window (label);
}

static void
btk_label_unrealize (BtkWidget *widget)
{
  BtkLabel *label;

  label = BTK_LABEL (widget);

  if (label->select_info)
    btk_label_destroy_window (label);

  BTK_WIDGET_CLASS (btk_label_parent_class)->unrealize (widget);
}

static void
btk_label_map (BtkWidget *widget)
{
  BtkLabel *label;

  label = BTK_LABEL (widget);

  BTK_WIDGET_CLASS (btk_label_parent_class)->map (widget);

  if (label->select_info)
    bdk_window_show (label->select_info->window);
}

static void
btk_label_unmap (BtkWidget *widget)
{
  BtkLabel *label;

  label = BTK_LABEL (widget);

  if (label->select_info)
    bdk_window_hide (label->select_info->window);

  BTK_WIDGET_CLASS (btk_label_parent_class)->unmap (widget);
}

static void
window_to_layout_coords (BtkLabel *label,
                         gint     *x,
                         gint     *y)
{
  gint lx, ly;
  BtkWidget *widget;

  widget = BTK_WIDGET (label);
  
  /* get layout location in widget->window coords */
  get_layout_location (label, &lx, &ly);
  
  if (x)
    {
      *x += widget->allocation.x; /* go to widget->window */
      *x -= lx;                   /* go to layout */
    }

  if (y)
    {
      *y += widget->allocation.y; /* go to widget->window */
      *y -= ly;                   /* go to layout */
    }
}

#if 0
static void
layout_to_window_coords (BtkLabel *label,
                         gint     *x,
                         gint     *y)
{
  gint lx, ly;
  BtkWidget *widget;

  widget = BTK_WIDGET (label);
  
  /* get layout location in widget->window coords */
  get_layout_location (label, &lx, &ly);
  
  if (x)
    {
      *x += lx;                   /* go to widget->window */
      *x -= widget->allocation.x; /* go to selection window */
    }

  if (y)
    {
      *y += ly;                   /* go to widget->window */
      *y -= widget->allocation.y; /* go to selection window */
    }
}
#endif

static gboolean
get_layout_index (BtkLabel *label,
                  gint      x,
                  gint      y,
                  gint     *index)
{
  gint trailing = 0;
  const gchar *cluster;
  const gchar *cluster_end;
  gboolean inside;

  *index = 0;

  btk_label_ensure_layout (label);

  window_to_layout_coords (label, &x, &y);

  x *= BANGO_SCALE;
  y *= BANGO_SCALE;

  inside = bango_layout_xy_to_index (label->layout,
                                     x, y,
                                     index, &trailing);

  cluster = label->text + *index;
  cluster_end = cluster;
  while (trailing)
    {
      cluster_end = g_utf8_next_char (cluster_end);
      --trailing;
    }

  *index += (cluster_end - cluster);

  return inside;
}

static void
btk_label_select_word (BtkLabel *label)
{
  gint min, max;
  
  gint start_index = btk_label_move_backward_word (label, label->select_info->selection_end);
  gint end_index = btk_label_move_forward_word (label, label->select_info->selection_end);

  min = MIN (label->select_info->selection_anchor,
	     label->select_info->selection_end);
  max = MAX (label->select_info->selection_anchor,
	     label->select_info->selection_end);

  min = MIN (min, start_index);
  max = MAX (max, end_index);

  btk_label_select_rebunnyion_index (label, min, max);
}

static void
btk_label_grab_focus (BtkWidget *widget)
{
  BtkLabel *label;
  gboolean select_on_focus;
  BtkLabelLink *link;

  label = BTK_LABEL (widget);

  if (label->select_info == NULL)
    return;

  BTK_WIDGET_CLASS (btk_label_parent_class)->grab_focus (widget);

  if (label->select_info->selectable)
    {
      g_object_get (btk_widget_get_settings (widget),
                    "btk-label-select-on-focus",
                    &select_on_focus,
                    NULL);

      if (select_on_focus && !label->in_click)
        btk_label_select_rebunnyion (label, 0, -1);
    }
  else
    {
      if (label->select_info->links && !label->in_click)
        {
          link = label->select_info->links->data;
          label->select_info->selection_anchor = link->start;
          label->select_info->selection_end = link->start;
        }
    }
}

static gboolean
btk_label_focus (BtkWidget        *widget,
                 BtkDirectionType  direction)
{
  BtkLabel *label = BTK_LABEL (widget);
  BtkLabelSelectionInfo *info = label->select_info;
  BtkLabelLink *focus_link;
  GList *l;

  if (!btk_widget_is_focus (widget))
    {
      btk_widget_grab_focus (widget);
      if (info)
        {
          focus_link = btk_label_get_focus_link (label);
          if (focus_link && direction == BTK_DIR_TAB_BACKWARD)
            {
              l = g_list_last (info->links);
              focus_link = l->data;
              info->selection_anchor = focus_link->start;
              info->selection_end = focus_link->start;
            }
        }

      return TRUE;
    }

  if (!info)
    return FALSE;

  if (info->selectable)
    {
      gint index;

      if (info->selection_anchor != info->selection_end)
        goto out;

      index = info->selection_anchor;

      if (direction == BTK_DIR_TAB_FORWARD)
        for (l = info->links; l; l = l->next)
          {
            BtkLabelLink *link = l->data;

            if (link->start > index)
              {
                btk_label_select_rebunnyion_index (label, link->start, link->start);
                return TRUE;
              }
          }
      else if (direction == BTK_DIR_TAB_BACKWARD)
        for (l = g_list_last (info->links); l; l = l->prev)
          {
            BtkLabelLink *link = l->data;

            if (link->end < index)
              {
                btk_label_select_rebunnyion_index (label, link->start, link->start);
                return TRUE;
              }
          }

      goto out;
    }
  else
    {
      focus_link = btk_label_get_focus_link (label);
      switch (direction)
        {
        case BTK_DIR_TAB_FORWARD:
          if (focus_link)
            {
              l = g_list_find (info->links, focus_link);
              l = l->next;
            }
          else
            l = info->links;
          break;

        case BTK_DIR_TAB_BACKWARD:
          if (focus_link)
            {
              l = g_list_find (info->links, focus_link);
              l = l->prev;
            }
          else
            l = g_list_last (info->links);
          break;

        default:
          goto out;
        }

      if (l)
        {
          focus_link = l->data;
          info->selection_anchor = focus_link->start;
          info->selection_end = focus_link->start;
          btk_widget_queue_draw (widget);

          return TRUE;
        }
    }

out:

  return FALSE;
}

static gboolean
btk_label_button_press (BtkWidget      *widget,
                        BdkEventButton *event)
{
  BtkLabel *label = BTK_LABEL (widget);
  BtkLabelSelectionInfo *info = label->select_info;
  gint index = 0;
  gint min, max;

  if (info == NULL)
    return FALSE;

  if (info->active_link)
    {
      if (_btk_button_event_triggers_context_menu (event))
        {
          info->link_clicked = 1;
          btk_label_do_popup (label, event);
          return TRUE;
        }
      else if (event->button == 1)
        {
          info->link_clicked = 1;
          btk_widget_queue_draw (widget);
        }
    }

  if (!info->selectable)
    return FALSE;

  info->in_drag = FALSE;
  info->select_words = FALSE;

  if (_btk_button_event_triggers_context_menu (event))
    {
      btk_label_do_popup (label, event);

      return TRUE;
    }
  else if (event->button == 1)
    {
      if (!btk_widget_has_focus (widget))
	{
	  label->in_click = TRUE;
	  btk_widget_grab_focus (widget);
	  label->in_click = FALSE;
	}

      if (event->type == BDK_3BUTTON_PRESS)
	{
	  btk_label_select_rebunnyion_index (label, 0, strlen (label->text));
	  return TRUE;
	}

      if (event->type == BDK_2BUTTON_PRESS)
	{
          info->select_words = TRUE;
	  btk_label_select_word (label);
	  return TRUE;
	}

      get_layout_index (label, event->x, event->y, &index);

      min = MIN (info->selection_anchor, info->selection_end);
      max = MAX (info->selection_anchor, info->selection_end);

      if ((info->selection_anchor != info->selection_end) &&
	  (event->state & BDK_SHIFT_MASK))
	{
	  /* extend (same as motion) */
	  min = MIN (min, index);
	  max = MAX (max, index);

	  /* ensure the anchor is opposite index */
	  if (index == min)
	    {
	      gint tmp = min;
	      min = max;
	      max = tmp;
	    }

	  btk_label_select_rebunnyion_index (label, min, max);
	}
      else
	{
	  if (event->type == BDK_3BUTTON_PRESS)
	    btk_label_select_rebunnyion_index (label, 0, strlen (label->text));
	  else if (event->type == BDK_2BUTTON_PRESS)
	    btk_label_select_word (label);
	  else if (min < max && min <= index && index <= max)
	    {
	      info->in_drag = TRUE;
	      info->drag_start_x = event->x;
	      info->drag_start_y = event->y;
	    }
	  else
	    /* start a replacement */
	    btk_label_select_rebunnyion_index (label, index, index);
	}

      return TRUE;
    }

  return FALSE;
}

static gboolean
btk_label_button_release (BtkWidget      *widget,
                          BdkEventButton *event)

{
  BtkLabel *label = BTK_LABEL (widget);
  BtkLabelSelectionInfo *info = label->select_info;
  gint index;

  if (info == NULL)
    return FALSE;

  if (info->in_drag)
    {
      info->in_drag = 0;

      get_layout_index (label, event->x, event->y, &index);
      btk_label_select_rebunnyion_index (label, index, index);

      return FALSE;
    }

  if (event->button != 1)
    return FALSE;

  if (info->active_link &&
      info->selection_anchor == info->selection_end &&
      info->link_clicked)
    {
      emit_activate_link (label, info->active_link);
      info->link_clicked = 0;

      return TRUE;
    }

  /* The goal here is to return TRUE iff we ate the
   * button press to start selecting.
   */
  return TRUE;
}

static void
connect_mnemonics_visible_notify (BtkLabel *label)
{
  BtkLabelPrivate *priv = BTK_LABEL_GET_PRIVATE (label);
  BtkWidget *toplevel;
  gboolean connected;

  toplevel = btk_widget_get_toplevel (BTK_WIDGET (label));

  if (!BTK_IS_WINDOW (toplevel))
    return;

  /* always set up this widgets initial value */
  priv->mnemonics_visible =
    btk_window_get_mnemonics_visible (BTK_WINDOW (toplevel));

  connected =
    GPOINTER_TO_INT (g_object_get_data (B_OBJECT (toplevel),
                                        "btk-label-mnemonics-visible-connected"));

  if (!connected)
    {
      g_signal_connect (toplevel,
                        "notify::mnemonics-visible",
                        G_CALLBACK (label_mnemonics_visible_changed),
                        label);
      g_object_set_data (B_OBJECT (toplevel),
                         "btk-label-mnemonics-visible-connected",
                         GINT_TO_POINTER (1));
    }
}

static void
drag_begin_cb (BtkWidget      *widget,
               BdkDragContext *context,
               gpointer        data)
{
  BtkLabel *label;
  BdkPixmap *pixmap = NULL;

  g_signal_handlers_disconnect_by_func (widget, drag_begin_cb, NULL);

  label = BTK_LABEL (widget);

  if ((label->select_info->selection_anchor !=
       label->select_info->selection_end) &&
      label->text)
    {
      gint start, end;
      gint len;

      start = MIN (label->select_info->selection_anchor,
                   label->select_info->selection_end);
      end = MAX (label->select_info->selection_anchor,
                 label->select_info->selection_end);
      
      len = strlen (label->text);
      
      if (end > len)
        end = len;
      
      if (start > len)
        start = len;
      
      pixmap = _btk_text_util_create_drag_icon (widget, 
						label->text + start,
						end - start);
    }

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
}

static gboolean
btk_label_motion (BtkWidget      *widget,
                  BdkEventMotion *event)
{
  BtkLabel *label = BTK_LABEL (widget);
  BtkLabelSelectionInfo *info = label->select_info;
  gint index;
  gint x, y;

  if (info == NULL)
    return FALSE;

  if (info->links && !info->in_drag)
    {
      GList *l;
      BtkLabelLink *link;
      gboolean found = FALSE;

      if (info->selection_anchor == info->selection_end)
        {
          bdk_window_get_pointer (event->window, &x, &y, NULL);
          if (get_layout_index (label, x, y, &index))
            {
              for (l = info->links; l != NULL; l = l->next)
                {
                  link = l->data;
                  if (index >= link->start && index <= link->end)
                    {
                      found = TRUE;
                      break;
                    }
                }
            }
        }

      if (found)
        {
          if (info->active_link != link)
            {
              info->link_clicked = 0;
              info->active_link = link;
              btk_label_update_cursor (label);
              btk_widget_queue_draw (widget);
            }
        }
      else
        {
          if (info->active_link != NULL)
            {
              info->link_clicked = 0;
              info->active_link = NULL;
              btk_label_update_cursor (label);
              btk_widget_queue_draw (widget);
            }
        }
    }

  if (!info->selectable)
    return FALSE;

  if ((event->state & BDK_BUTTON1_MASK) == 0)
    return FALSE;

  bdk_window_get_pointer (info->window, &x, &y, NULL);
 
  if (info->in_drag)
    {
      if (btk_drag_check_threshold (widget,
				    info->drag_start_x,
				    info->drag_start_y,
				    event->x, event->y))
	{
	  BtkTargetList *target_list = btk_target_list_new (NULL, 0);

	  btk_target_list_add_text_targets (target_list, 0);

          g_signal_connect (widget, "drag-begin",
                            G_CALLBACK (drag_begin_cb), NULL);
	  btk_drag_begin (widget, target_list,
			  BDK_ACTION_COPY,
			  1, (BdkEvent *)event);

	  info->in_drag = FALSE;

	  btk_target_list_unref (target_list);
	}
    }
  else
    {
      get_layout_index (label, x, y, &index);

      if (info->select_words)
        {
          gint min, max;
          gint old_min, old_max;
          gint anchor, end;

          min = btk_label_move_backward_word (label, index);
          max = btk_label_move_forward_word (label, index);

          anchor = info->selection_anchor;
          end = info->selection_end;

          old_min = MIN (anchor, end);
          old_max = MAX (anchor, end);

          if (min < old_min)
            {
              anchor = min;
              end = old_max;
            }
          else if (old_max < max)
            {
              anchor = max;
              end = old_min;
            }
          else if (anchor == old_min)
            {
              if (anchor != min)
                anchor = max;
            }
          else
            {
              if (anchor != max)
                anchor = min;
            }

          btk_label_select_rebunnyion_index (label, anchor, end);
        }
      else
        btk_label_select_rebunnyion_index (label, info->selection_anchor, index);
    }

  return TRUE;
}

static gboolean
btk_label_leave_notify (BtkWidget        *widget,
                        BdkEventCrossing *event)
{
  BtkLabel *label = BTK_LABEL (widget);

  if (label->select_info)
    {
      label->select_info->active_link = NULL;
      btk_label_update_cursor (label);
      btk_widget_queue_draw (widget);
    }

  if (BTK_WIDGET_CLASS (btk_label_parent_class)->leave_notify_event)
    return BTK_WIDGET_CLASS (btk_label_parent_class)->leave_notify_event (widget, event);

 return FALSE;
}

static void
btk_label_create_window (BtkLabel *label)
{
  BtkWidget *widget;
  BdkWindowAttr attributes;
  gint attributes_mask;
  
  g_assert (label->select_info);
  widget = BTK_WIDGET (label);
  g_assert (btk_widget_get_realized (widget));
  
  if (label->select_info->window)
    return;

  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.wclass = BDK_INPUT_ONLY;
  attributes.override_redirect = TRUE;
  attributes.event_mask = btk_widget_get_events (widget) |
    BDK_BUTTON_PRESS_MASK        |
    BDK_BUTTON_RELEASE_MASK      |
    BDK_LEAVE_NOTIFY_MASK        |
    BDK_BUTTON_MOTION_MASK       |
    BDK_POINTER_MOTION_MASK      |
    BDK_POINTER_MOTION_HINT_MASK;
  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_NOREDIR;
  if (btk_widget_is_sensitive (widget))
    {
      attributes.cursor = bdk_cursor_new_for_display (btk_widget_get_display (widget),
						      BDK_XTERM);
      attributes_mask |= BDK_WA_CURSOR;
    }


  label->select_info->window = bdk_window_new (widget->window,
                                               &attributes, attributes_mask);
  bdk_window_set_user_data (label->select_info->window, widget);

  if (attributes_mask & BDK_WA_CURSOR)
    bdk_cursor_unref (attributes.cursor);
}

static void
btk_label_destroy_window (BtkLabel *label)
{
  g_assert (label->select_info);

  if (label->select_info->window == NULL)
    return;

  bdk_window_set_user_data (label->select_info->window, NULL);
  bdk_window_destroy (label->select_info->window);
  label->select_info->window = NULL;
}

static void
btk_label_ensure_select_info (BtkLabel *label)
{
  if (label->select_info == NULL)
    {
      label->select_info = g_new0 (BtkLabelSelectionInfo, 1);

      btk_widget_set_can_focus (BTK_WIDGET (label), TRUE);

      if (btk_widget_get_realized (BTK_WIDGET (label)))
	btk_label_create_window (label);

      if (btk_widget_get_mapped (BTK_WIDGET (label)))
        bdk_window_show (label->select_info->window);
    }
}

static void
btk_label_clear_select_info (BtkLabel *label)
{
  if (label->select_info == NULL)
    return;

  if (!label->select_info->selectable && !label->select_info->links)
    {
      btk_label_destroy_window (label);

      g_free (label->select_info);
      label->select_info = NULL;

      btk_widget_set_can_focus (BTK_WIDGET (label), FALSE);
    }
}

/**
 * btk_label_set_selectable:
 * @label: a #BtkLabel
 * @setting: %TRUE to allow selecting text in the label
 *
 * Selectable labels allow the user to select text from the label, for
 * copy-and-paste.
 **/
void
btk_label_set_selectable (BtkLabel *label,
                          gboolean  setting)
{
  gboolean old_setting;

  g_return_if_fail (BTK_IS_LABEL (label));

  setting = setting != FALSE;
  old_setting = label->select_info && label->select_info->selectable;

  if (setting)
    {
      btk_label_ensure_select_info (label);
      label->select_info->selectable = TRUE;
      btk_label_update_cursor (label);
    }
  else
    {
      if (old_setting)
        {
          /* unselect, to give up the selection */
          btk_label_select_rebunnyion (label, 0, 0);

          label->select_info->selectable = FALSE;
          btk_label_clear_select_info (label);
          btk_label_update_cursor (label);
        }
    }
  if (setting != old_setting)
    {
      g_object_freeze_notify (B_OBJECT (label));
      g_object_notify (B_OBJECT (label), "selectable");
      g_object_notify (B_OBJECT (label), "cursor-position");
      g_object_notify (B_OBJECT (label), "selection-bound");
      g_object_thaw_notify (B_OBJECT (label));
      btk_widget_queue_draw (BTK_WIDGET (label));
    }
}

/**
 * btk_label_get_selectable:
 * @label: a #BtkLabel
 * 
 * Gets the value set by btk_label_set_selectable().
 * 
 * Return value: %TRUE if the user can copy text from the label
 **/
gboolean
btk_label_get_selectable (BtkLabel *label)
{
  g_return_val_if_fail (BTK_IS_LABEL (label), FALSE);

  return label->select_info && label->select_info->selectable;
}

static void
free_angle (gpointer angle)
{
  g_slice_free (gdouble, angle);
}

/**
 * btk_label_set_angle:
 * @label: a #BtkLabel
 * @angle: the angle that the baseline of the label makes with
 *   the horizontal, in degrees, measured counterclockwise
 * 
 * Sets the angle of rotation for the label. An angle of 90 reads from
 * from bottom to top, an angle of 270, from top to bottom. The angle
 * setting for the label is ignored if the label is selectable,
 * wrapped, or ellipsized.
 *
 * Since: 2.6
 **/
void
btk_label_set_angle (BtkLabel *label,
		     gdouble   angle)
{
  gdouble *label_angle;

  g_return_if_fail (BTK_IS_LABEL (label));

  label_angle = (gdouble *)g_object_get_qdata (B_OBJECT (label), quark_angle);

  if (!label_angle)
    {
      label_angle = g_slice_new (gdouble);
      *label_angle = 0.0;
      g_object_set_qdata_full (B_OBJECT (label), quark_angle, 
			       label_angle, free_angle);
    }
  
  /* Canonicalize to [0,360]. We don't canonicalize 360 to 0, because
   * double property ranges are inclusive, and changing 360 to 0 would
   * make a property editor behave strangely.
   */
  if (angle < 0 || angle > 360.0)
    angle = angle - 360. * floor (angle / 360.);

  if (*label_angle != angle)
    {
      *label_angle = angle;
      
      btk_label_clear_layout (label);
      btk_widget_queue_resize (BTK_WIDGET (label));

      g_object_notify (B_OBJECT (label), "angle");
    }
}

/**
 * btk_label_get_angle:
 * @label: a #BtkLabel
 * 
 * Gets the angle of rotation for the label. See
 * btk_label_set_angle().
 * 
 * Return value: the angle of rotation for the label
 *
 * Since: 2.6
 **/
gdouble
btk_label_get_angle  (BtkLabel *label)
{
  gdouble *angle;

  g_return_val_if_fail (BTK_IS_LABEL (label), 0.0);
  
  angle = (gdouble *)g_object_get_qdata (B_OBJECT (label), quark_angle);

  if (angle)
    return *angle;
  else
    return 0.0;
}

static void
btk_label_set_selection_text (BtkLabel         *label,
			      BtkSelectionData *selection_data)
{
  if ((label->select_info->selection_anchor !=
       label->select_info->selection_end) &&
      label->text)
    {
      gint start, end;
      gint len;
      
      start = MIN (label->select_info->selection_anchor,
                   label->select_info->selection_end);
      end = MAX (label->select_info->selection_anchor,
                 label->select_info->selection_end);
      
      len = strlen (label->text);
      
      if (end > len)
        end = len;
      
      if (start > len)
        start = len;
      
      btk_selection_data_set_text (selection_data,
				   label->text + start,
				   end - start);
    }
}

static void
btk_label_drag_data_get (BtkWidget        *widget,
			 BdkDragContext   *context,
			 BtkSelectionData *selection_data,
			 guint             info,
			 guint             time)
{
  btk_label_set_selection_text (BTK_LABEL (widget), selection_data);
}

static void
get_text_callback (BtkClipboard     *clipboard,
                   BtkSelectionData *selection_data,
                   guint             info,
                   gpointer          user_data_or_owner)
{
  btk_label_set_selection_text (BTK_LABEL (user_data_or_owner), selection_data);
}

static void
clear_text_callback (BtkClipboard     *clipboard,
                     gpointer          user_data_or_owner)
{
  BtkLabel *label;

  label = BTK_LABEL (user_data_or_owner);

  if (label->select_info)
    {
      label->select_info->selection_anchor = label->select_info->selection_end;
      
      btk_widget_queue_draw (BTK_WIDGET (label));
    }
}

static void
btk_label_select_rebunnyion_index (BtkLabel *label,
                               gint      anchor_index,
                               gint      end_index)
{
  g_return_if_fail (BTK_IS_LABEL (label));
  
  if (label->select_info && label->select_info->selectable)
    {
      BtkClipboard *clipboard;

      if (label->select_info->selection_anchor == anchor_index &&
	  label->select_info->selection_end == end_index)
	return;

      label->select_info->selection_anchor = anchor_index;
      label->select_info->selection_end = end_index;

      clipboard = btk_widget_get_clipboard (BTK_WIDGET (label),
					    BDK_SELECTION_PRIMARY);

      if (anchor_index != end_index)
        {
          BtkTargetList *list;
          BtkTargetEntry *targets;
          gint n_targets;

          list = btk_target_list_new (NULL, 0);
          btk_target_list_add_text_targets (list, 0);
          targets = btk_target_table_new_from_list (list, &n_targets);

          btk_clipboard_set_with_owner (clipboard,
                                        targets, n_targets,
                                        get_text_callback,
                                        clear_text_callback,
                                        B_OBJECT (label));

          btk_target_table_free (targets, n_targets);
          btk_target_list_unref (list);
        }
      else
        {
          if (btk_clipboard_get_owner (clipboard) == B_OBJECT (label))
            btk_clipboard_clear (clipboard);
        }

      btk_widget_queue_draw (BTK_WIDGET (label));

      g_object_freeze_notify (B_OBJECT (label));
      g_object_notify (B_OBJECT (label), "cursor-position");
      g_object_notify (B_OBJECT (label), "selection-bound");
      g_object_thaw_notify (B_OBJECT (label));
    }
}

/**
 * btk_label_select_rebunnyion:
 * @label: a #BtkLabel
 * @start_offset: start offset (in characters not bytes)
 * @end_offset: end offset (in characters not bytes)
 *
 * Selects a range of characters in the label, if the label is selectable.
 * See btk_label_set_selectable(). If the label is not selectable,
 * this function has no effect. If @start_offset or
 * @end_offset are -1, then the end of the label will be substituted.
 **/
void
btk_label_select_rebunnyion  (BtkLabel *label,
                          gint      start_offset,
                          gint      end_offset)
{
  g_return_if_fail (BTK_IS_LABEL (label));
  
  if (label->text && label->select_info)
    {
      if (start_offset < 0)
        start_offset = g_utf8_strlen (label->text, -1);
      
      if (end_offset < 0)
        end_offset = g_utf8_strlen (label->text, -1);
      
      btk_label_select_rebunnyion_index (label,
                                     g_utf8_offset_to_pointer (label->text, start_offset) - label->text,
                                     g_utf8_offset_to_pointer (label->text, end_offset) - label->text);
    }
}

/**
 * btk_label_get_selection_bounds:
 * @label: a #BtkLabel
 * @start: (out): return location for start of selection, as a character offset
 * @end: (out): return location for end of selection, as a character offset
 * 
 * Gets the selected range of characters in the label, returning %TRUE
 * if there's a selection.
 * 
 * Return value: %TRUE if selection is non-empty
 **/
gboolean
btk_label_get_selection_bounds (BtkLabel  *label,
                                gint      *start,
                                gint      *end)
{
  g_return_val_if_fail (BTK_IS_LABEL (label), FALSE);

  if (label->select_info == NULL)
    {
      /* not a selectable label */
      if (start)
        *start = 0;
      if (end)
        *end = 0;

      return FALSE;
    }
  else
    {
      gint start_index, end_index;
      gint start_offset, end_offset;
      gint len;
      
      start_index = MIN (label->select_info->selection_anchor,
                   label->select_info->selection_end);
      end_index = MAX (label->select_info->selection_anchor,
                 label->select_info->selection_end);

      len = strlen (label->text);

      if (end_index > len)
        end_index = len;

      if (start_index > len)
        start_index = len;
      
      start_offset = g_utf8_strlen (label->text, start_index);
      end_offset = g_utf8_strlen (label->text, end_index);

      if (start_offset > end_offset)
        {
          gint tmp = start_offset;
          start_offset = end_offset;
          end_offset = tmp;
        }
      
      if (start)
        *start = start_offset;

      if (end)
        *end = end_offset;

      return start_offset != end_offset;
    }
}


/**
 * btk_label_get_layout:
 * @label: a #BtkLabel
 * 
 * Gets the #BangoLayout used to display the label.
 * The layout is useful to e.g. convert text positions to
 * pixel positions, in combination with btk_label_get_layout_offsets().
 * The returned layout is owned by the label so need not be
 * freed by the caller.
 *
 * Return value: (transfer none): the #BangoLayout for this label
 **/
BangoLayout*
btk_label_get_layout (BtkLabel *label)
{
  g_return_val_if_fail (BTK_IS_LABEL (label), NULL);

  btk_label_ensure_layout (label);

  return label->layout;
}

/**
 * btk_label_get_layout_offsets:
 * @label: a #BtkLabel
 * @x: (out) (allow-none): location to store X offset of layout, or %NULL
 * @y: (out) (allow-none): location to store Y offset of layout, or %NULL
 *
 * Obtains the coordinates where the label will draw the #BangoLayout
 * representing the text in the label; useful to convert mouse events
 * into coordinates inside the #BangoLayout, e.g. to take some action
 * if some part of the label is clicked. Of course you will need to
 * create a #BtkEventBox to receive the events, and pack the label
 * inside it, since labels are a #BTK_NO_WINDOW widget. Remember
 * when using the #BangoLayout functions you need to convert to
 * and from pixels using BANGO_PIXELS() or #BANGO_SCALE.
 **/
void
btk_label_get_layout_offsets (BtkLabel *label,
                              gint     *x,
                              gint     *y)
{
  g_return_if_fail (BTK_IS_LABEL (label));

  btk_label_ensure_layout (label);

  get_layout_location (label, x, y);
}

/**
 * btk_label_set_use_markup:
 * @label: a #BtkLabel
 * @setting: %TRUE if the label's text should be parsed for markup.
 *
 * Sets whether the text of the label contains markup in <link
 * linkend="BangoMarkupFormat">Bango's text markup
 * language</link>. See btk_label_set_markup().
 **/
void
btk_label_set_use_markup (BtkLabel *label,
			  gboolean  setting)
{
  g_return_if_fail (BTK_IS_LABEL (label));

  btk_label_set_use_markup_internal (label, setting);
  btk_label_recalculate (label);
}

/**
 * btk_label_get_use_markup:
 * @label: a #BtkLabel
 *
 * Returns whether the label's text is interpreted as marked up with
 * the <link linkend="BangoMarkupFormat">Bango text markup
 * language</link>. See btk_label_set_use_markup ().
 *
 * Return value: %TRUE if the label's text will be parsed for markup.
 **/
gboolean
btk_label_get_use_markup (BtkLabel *label)
{
  g_return_val_if_fail (BTK_IS_LABEL (label), FALSE);
  
  return label->use_markup;
}

/**
 * btk_label_set_use_underline:
 * @label: a #BtkLabel
 * @setting: %TRUE if underlines in the text indicate mnemonics
 *
 * If true, an underline in the text indicates the next character should be
 * used for the mnemonic accelerator key.
 */
void
btk_label_set_use_underline (BtkLabel *label,
			     gboolean  setting)
{
  g_return_if_fail (BTK_IS_LABEL (label));

  btk_label_set_use_underline_internal (label, setting);
  btk_label_recalculate (label);
}

/**
 * btk_label_get_use_underline:
 * @label: a #BtkLabel
 *
 * Returns whether an embedded underline in the label indicates a
 * mnemonic. See btk_label_set_use_underline().
 *
 * Return value: %TRUE whether an embedded underline in the label indicates
 *               the mnemonic accelerator keys.
 **/
gboolean
btk_label_get_use_underline (BtkLabel *label)
{
  g_return_val_if_fail (BTK_IS_LABEL (label), FALSE);
  
  return label->use_underline;
}

/**
 * btk_label_set_single_line_mode:
 * @label: a #BtkLabel
 * @single_line_mode: %TRUE if the label should be in single line mode
 *
 * Sets whether the label is in single line mode.
 *
 * Since: 2.6
 */
void
btk_label_set_single_line_mode (BtkLabel *label,
                                gboolean single_line_mode)
{
  g_return_if_fail (BTK_IS_LABEL (label));

  single_line_mode = single_line_mode != FALSE;

  if (label->single_line_mode != single_line_mode)
    {
      label->single_line_mode = single_line_mode;

      btk_label_clear_layout (label);
      btk_widget_queue_resize (BTK_WIDGET (label));

      g_object_notify (B_OBJECT (label), "single-line-mode");
    }
}

/**
 * btk_label_get_single_line_mode:
 * @label: a #BtkLabel
 *
 * Returns whether the label is in single line mode.
 *
 * Return value: %TRUE when the label is in single line mode.
 *
 * Since: 2.6
 **/
gboolean
btk_label_get_single_line_mode  (BtkLabel *label)
{
  g_return_val_if_fail (BTK_IS_LABEL (label), FALSE);

  return label->single_line_mode;
}

/* Compute the X position for an offset that corresponds to the "more important
 * cursor position for that offset. We use this when trying to guess to which
 * end of the selection we should go to when the user hits the left or
 * right arrow key.
 */
static void
get_better_cursor (BtkLabel *label,
		   gint      index,
		   gint      *x,
		   gint      *y)
{
  BdkKeymap *keymap = bdk_keymap_get_for_display (btk_widget_get_display (BTK_WIDGET (label)));
  BangoDirection keymap_direction = bdk_keymap_get_direction (keymap);
  BangoDirection cursor_direction = get_cursor_direction (label);
  gboolean split_cursor;
  BangoRectangle strong_pos, weak_pos;
  
  g_object_get (btk_widget_get_settings (BTK_WIDGET (label)),
		"btk-split-cursor", &split_cursor,
		NULL);

  btk_label_ensure_layout (label);
  
  bango_layout_get_cursor_pos (label->layout, index,
			       &strong_pos, &weak_pos);

  if (split_cursor)
    {
      *x = strong_pos.x / BANGO_SCALE;
      *y = strong_pos.y / BANGO_SCALE;
    }
  else
    {
      if (keymap_direction == cursor_direction)
	{
	  *x = strong_pos.x / BANGO_SCALE;
	  *y = strong_pos.y / BANGO_SCALE;
	}
      else
	{
	  *x = weak_pos.x / BANGO_SCALE;
	  *y = weak_pos.y / BANGO_SCALE;
	}
    }
}


static gint
btk_label_move_logically (BtkLabel *label,
			  gint      start,
			  gint      count)
{
  gint offset = g_utf8_pointer_to_offset (label->text,
					  label->text + start);

  if (label->text)
    {
      BangoLogAttr *log_attrs;
      gint n_attrs;
      gint length;

      btk_label_ensure_layout (label);
      
      length = g_utf8_strlen (label->text, -1);

      bango_layout_get_log_attrs (label->layout, &log_attrs, &n_attrs);

      while (count > 0 && offset < length)
	{
	  do
	    offset++;
	  while (offset < length && !log_attrs[offset].is_cursor_position);
	  
	  count--;
	}
      while (count < 0 && offset > 0)
	{
	  do
	    offset--;
	  while (offset > 0 && !log_attrs[offset].is_cursor_position);
	  
	  count++;
	}
      
      g_free (log_attrs);
    }

  return g_utf8_offset_to_pointer (label->text, offset) - label->text;
}

static gint
btk_label_move_visually (BtkLabel *label,
			 gint      start,
			 gint      count)
{
  gint index;

  index = start;
  
  while (count != 0)
    {
      int new_index, new_trailing;
      gboolean split_cursor;
      gboolean strong;

      btk_label_ensure_layout (label);

      g_object_get (btk_widget_get_settings (BTK_WIDGET (label)),
		    "btk-split-cursor", &split_cursor,
		    NULL);

      if (split_cursor)
	strong = TRUE;
      else
	{
	  BdkKeymap *keymap = bdk_keymap_get_for_display (btk_widget_get_display (BTK_WIDGET (label)));
	  BangoDirection keymap_direction = bdk_keymap_get_direction (keymap);

	  strong = keymap_direction == get_cursor_direction (label);
	}
      
      if (count > 0)
	{
	  bango_layout_move_cursor_visually (label->layout, strong, index, 0, 1, &new_index, &new_trailing);
	  count--;
	}
      else
	{
	  bango_layout_move_cursor_visually (label->layout, strong, index, 0, -1, &new_index, &new_trailing);
	  count++;
	}

      if (new_index < 0 || new_index == G_MAXINT)
	break;

      index = new_index;
      
      while (new_trailing--)
	index = g_utf8_next_char (label->text + new_index) - label->text;
    }
  
  return index;
}

static gint
btk_label_move_forward_word (BtkLabel *label,
			     gint      start)
{
  gint new_pos = g_utf8_pointer_to_offset (label->text,
					   label->text + start);
  gint length;

  length = g_utf8_strlen (label->text, -1);
  if (new_pos < length)
    {
      BangoLogAttr *log_attrs;
      gint n_attrs;

      btk_label_ensure_layout (label);
      
      bango_layout_get_log_attrs (label->layout, &log_attrs, &n_attrs);
      
      /* Find the next word end */
      new_pos++;
      while (new_pos < n_attrs && !log_attrs[new_pos].is_word_end)
	new_pos++;

      g_free (log_attrs);
    }

  return g_utf8_offset_to_pointer (label->text, new_pos) - label->text;
}


static gint
btk_label_move_backward_word (BtkLabel *label,
			      gint      start)
{
  gint new_pos = g_utf8_pointer_to_offset (label->text,
					   label->text + start);

  if (new_pos > 0)
    {
      BangoLogAttr *log_attrs;
      gint n_attrs;

      btk_label_ensure_layout (label);
      
      bango_layout_get_log_attrs (label->layout, &log_attrs, &n_attrs);
      
      new_pos -= 1;

      /* Find the previous word beginning */
      while (new_pos > 0 && !log_attrs[new_pos].is_word_start)
	new_pos--;

      g_free (log_attrs);
    }

  return g_utf8_offset_to_pointer (label->text, new_pos) - label->text;
}

static void
btk_label_move_cursor (BtkLabel       *label,
		       BtkMovementStep step,
		       gint            count,
		       gboolean        extend_selection)
{
  gint old_pos;
  gint new_pos;
  
  if (label->select_info == NULL)
    return;

  old_pos = new_pos = label->select_info->selection_end;

  if (label->select_info->selection_end != label->select_info->selection_anchor &&
      !extend_selection)
    {
      /* If we have a current selection and aren't extending it, move to the
       * start/or end of the selection as appropriate
       */
      switch (step)
	{
	case BTK_MOVEMENT_VISUAL_POSITIONS:
	  {
	    gint end_x, end_y;
	    gint anchor_x, anchor_y;
	    gboolean end_is_left;
	    
	    get_better_cursor (label, label->select_info->selection_end, &end_x, &end_y);
	    get_better_cursor (label, label->select_info->selection_anchor, &anchor_x, &anchor_y);

	    end_is_left = (end_y < anchor_y) || (end_y == anchor_y && end_x < anchor_x);
	    
	    if (count < 0)
	      new_pos = end_is_left ? label->select_info->selection_end : label->select_info->selection_anchor;
	    else
	      new_pos = !end_is_left ? label->select_info->selection_end : label->select_info->selection_anchor;
	    break;
	  }
	case BTK_MOVEMENT_LOGICAL_POSITIONS:
	case BTK_MOVEMENT_WORDS:
	  if (count < 0)
	    new_pos = MIN (label->select_info->selection_end, label->select_info->selection_anchor);
	  else
	    new_pos = MAX (label->select_info->selection_end, label->select_info->selection_anchor);
	  break;
	case BTK_MOVEMENT_DISPLAY_LINE_ENDS:
	case BTK_MOVEMENT_PARAGRAPH_ENDS:
	case BTK_MOVEMENT_BUFFER_ENDS:
	  /* FIXME: Can do better here */
	  new_pos = count < 0 ? 0 : strlen (label->text);
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
	  new_pos = btk_label_move_logically (label, new_pos, count);
	  break;
	case BTK_MOVEMENT_VISUAL_POSITIONS:
	  new_pos = btk_label_move_visually (label, new_pos, count);
          if (new_pos == old_pos)
            {
              if (!extend_selection)
                {
                  if (!btk_widget_keynav_failed (BTK_WIDGET (label),
                                                 count > 0 ?
                                                 BTK_DIR_RIGHT : BTK_DIR_LEFT))
                    {
                      BtkWidget *toplevel = btk_widget_get_toplevel (BTK_WIDGET (label));

                      if (toplevel)
                        btk_widget_child_focus (toplevel,
                                                count > 0 ?
                                                BTK_DIR_RIGHT : BTK_DIR_LEFT);
                    }
                }
              else
                {
                  btk_widget_error_bell (BTK_WIDGET (label));
                }
            }
	  break;
	case BTK_MOVEMENT_WORDS:
	  while (count > 0)
	    {
	      new_pos = btk_label_move_forward_word (label, new_pos);
	      count--;
	    }
	  while (count < 0)
	    {
	      new_pos = btk_label_move_backward_word (label, new_pos);
	      count++;
	    }
          if (new_pos == old_pos)
            btk_widget_error_bell (BTK_WIDGET (label));
	  break;
	case BTK_MOVEMENT_DISPLAY_LINE_ENDS:
	case BTK_MOVEMENT_PARAGRAPH_ENDS:
	case BTK_MOVEMENT_BUFFER_ENDS:
	  /* FIXME: Can do better here */
	  new_pos = count < 0 ? 0 : strlen (label->text);
          if (new_pos == old_pos)
            btk_widget_error_bell (BTK_WIDGET (label));
	  break;
	case BTK_MOVEMENT_DISPLAY_LINES:
	case BTK_MOVEMENT_PARAGRAPHS:
	case BTK_MOVEMENT_PAGES:
	case BTK_MOVEMENT_HORIZONTAL_PAGES:
	  break;
	}
    }

  if (extend_selection)
    btk_label_select_rebunnyion_index (label,
				   label->select_info->selection_anchor,
				   new_pos);
  else
    btk_label_select_rebunnyion_index (label, new_pos, new_pos);
}

static void
btk_label_copy_clipboard (BtkLabel *label)
{
  if (label->text && label->select_info)
    {
      gint start, end;
      gint len;
      BtkClipboard *clipboard;

      start = MIN (label->select_info->selection_anchor,
                   label->select_info->selection_end);
      end = MAX (label->select_info->selection_anchor,
                 label->select_info->selection_end);

      len = strlen (label->text);

      if (end > len)
        end = len;

      if (start > len)
        start = len;

      clipboard = btk_widget_get_clipboard (BTK_WIDGET (label), BDK_SELECTION_CLIPBOARD);

      if (start != end)
	btk_clipboard_set_text (clipboard, label->text + start, end - start);
      else
        {
          BtkLabelLink *link;

          link = btk_label_get_focus_link (label);
          if (link)
            btk_clipboard_set_text (clipboard, link->uri, -1);
        }
    }
}

static void
btk_label_select_all (BtkLabel *label)
{
  btk_label_select_rebunnyion_index (label, 0, strlen (label->text));
}

/* Quick hack of a popup menu
 */
static void
activate_cb (BtkWidget *menuitem,
	     BtkLabel  *label)
{
  const gchar *signal = g_object_get_data (B_OBJECT (menuitem), "btk-signal");
  g_signal_emit_by_name (label, signal);
}

static void
append_action_signal (BtkLabel     *label,
		      BtkWidget    *menu,
		      const gchar  *stock_id,
		      const gchar  *signal,
                      gboolean      sensitive)
{
  BtkWidget *menuitem = btk_image_menu_item_new_from_stock (stock_id, NULL);

  g_object_set_data (B_OBJECT (menuitem), I_("btk-signal"), (char *)signal);
  g_signal_connect (menuitem, "activate",
		    G_CALLBACK (activate_cb), label);

  btk_widget_set_sensitive (menuitem, sensitive);
  
  btk_widget_show (menuitem);
  btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
}

static void
popup_menu_detach (BtkWidget *attach_widget,
		   BtkMenu   *menu)
{
  BtkLabel *label = BTK_LABEL (attach_widget);

  if (label->select_info)
    label->select_info->popup_menu = NULL;
}

static void
popup_position_func (BtkMenu   *menu,
                     gint      *x,
                     gint      *y,
                     gboolean  *push_in,
                     gpointer	user_data)
{
  BtkLabel *label;
  BtkWidget *widget;
  BtkRequisition req;
  BdkScreen *screen;

  label = BTK_LABEL (user_data);
  widget = BTK_WIDGET (label);

  g_return_if_fail (btk_widget_get_realized (widget));

  screen = btk_widget_get_screen (widget);
  bdk_window_get_origin (widget->window, x, y);

  *x += widget->allocation.x;
  *y += widget->allocation.y;

  btk_widget_size_request (BTK_WIDGET (menu), &req);

  *x += widget->allocation.width / 2;
  *y += widget->allocation.height;

  *x = CLAMP (*x, 0, MAX (0, bdk_screen_get_width (screen) - req.width));
  *y = CLAMP (*y, 0, MAX (0, bdk_screen_get_height (screen) - req.height));
}

static void
open_link_activate_cb (BtkMenuItem *menu_item,
                       BtkLabel    *label)
{
  BtkLabelLink *link;

  link = btk_label_get_current_link (label);

  if (link)
    emit_activate_link (label, link);
}

static void
copy_link_activate_cb (BtkMenuItem *menu_item,
                       BtkLabel    *label)
{
  BtkClipboard *clipboard;
  const gchar *uri;

  uri = btk_label_get_current_uri (label);
  if (uri)
    {
      clipboard = btk_widget_get_clipboard (BTK_WIDGET (label), BDK_SELECTION_CLIPBOARD);
      btk_clipboard_set_text (clipboard, uri, -1);
    }
}

static gboolean
btk_label_popup_menu (BtkWidget *widget)
{
  btk_label_do_popup (BTK_LABEL (widget), NULL);

  return TRUE;
}

static void
btk_label_do_popup (BtkLabel       *label,
                    BdkEventButton *event)
{
  BtkWidget *menuitem;
  BtkWidget *menu;
  BtkWidget *image;
  gboolean have_selection;
  BtkLabelLink *link;

  if (!label->select_info)
    return;

  if (label->select_info->popup_menu)
    btk_widget_destroy (label->select_info->popup_menu);

  label->select_info->popup_menu = menu = btk_menu_new ();

  btk_menu_attach_to_widget (BTK_MENU (menu), BTK_WIDGET (label), popup_menu_detach);

  have_selection =
    label->select_info->selection_anchor != label->select_info->selection_end;

  if (event)
    {
      if (label->select_info->link_clicked)
        link = label->select_info->active_link;
      else
        link = NULL;
    }
  else
    link = btk_label_get_focus_link (label);

  if (!have_selection && link)
    {
      /* Open Link */
      menuitem = btk_image_menu_item_new_with_mnemonic (_("_Open Link"));
      btk_widget_show (menuitem);
      btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);

      g_signal_connect (B_OBJECT (menuitem), "activate",
                        G_CALLBACK (open_link_activate_cb), label);

      image = btk_image_new_from_stock (BTK_STOCK_JUMP_TO, BTK_ICON_SIZE_MENU);
      btk_widget_show (image);
      btk_image_menu_item_set_image (BTK_IMAGE_MENU_ITEM (menuitem), image);

      /* Copy Link Address */
      menuitem = btk_image_menu_item_new_with_mnemonic (_("Copy _Link Address"));
      btk_widget_show (menuitem);
      btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);

      g_signal_connect (B_OBJECT (menuitem), "activate",
                        G_CALLBACK (copy_link_activate_cb), label);

      image = btk_image_new_from_stock (BTK_STOCK_COPY, BTK_ICON_SIZE_MENU);
      btk_widget_show (image);
      btk_image_menu_item_set_image (BTK_IMAGE_MENU_ITEM (menuitem), image);
    }
  else
    {
      append_action_signal (label, menu, BTK_STOCK_CUT, "cut-clipboard", FALSE);
      append_action_signal (label, menu, BTK_STOCK_COPY, "copy-clipboard", have_selection);
      append_action_signal (label, menu, BTK_STOCK_PASTE, "paste-clipboard", FALSE);
  
      menuitem = btk_image_menu_item_new_from_stock (BTK_STOCK_DELETE, NULL);
      btk_widget_set_sensitive (menuitem, FALSE);
      btk_widget_show (menuitem);
      btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);

      menuitem = btk_separator_menu_item_new ();
      btk_widget_show (menuitem);
      btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);

      menuitem = btk_image_menu_item_new_from_stock (BTK_STOCK_SELECT_ALL, NULL);
      g_signal_connect_swapped (menuitem, "activate",
			        G_CALLBACK (btk_label_select_all), label);
      btk_widget_show (menuitem);
      btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
    }

  g_signal_emit (label, signals[POPULATE_POPUP], 0, menu);

  if (event)
    btk_menu_popup (BTK_MENU (menu), NULL, NULL,
                    NULL, NULL,
                    event->button, event->time);
  else
    {
      btk_menu_popup (BTK_MENU (menu), NULL, NULL,
                      popup_position_func, label,
                      0, btk_get_current_event_time ());
      btk_menu_shell_select_first (BTK_MENU_SHELL (menu), FALSE);
    }
}

static void
btk_label_clear_links (BtkLabel *label)
{
  if (!label->select_info)
    return;

  g_list_foreach (label->select_info->links, (GFunc)link_free, NULL);
  g_list_free (label->select_info->links);
  label->select_info->links = NULL;
  label->select_info->active_link = NULL;
}

static void
btk_label_rescan_links (BtkLabel *label)
{
  BangoLayout *layout = label->layout;
  BangoAttrList *attlist;
  BangoAttrIterator *iter;
  GList *links;

  if (!label->select_info || !label->select_info->links)
    return;

  attlist = bango_layout_get_attributes (layout);

  if (attlist == NULL)
    return;

  iter = bango_attr_list_get_iterator (attlist);

  links = label->select_info->links;

  do
    {
      BangoAttribute *underline;
      BangoAttribute *color;

      underline = bango_attr_iterator_get (iter, BANGO_ATTR_UNDERLINE);
      color = bango_attr_iterator_get (iter, BANGO_ATTR_FOREGROUND);

      if (underline != NULL && color != NULL)
        {
          gint start, end;
          BangoRectangle start_pos;
          BangoRectangle end_pos;
          BtkLabelLink *link;

          bango_attr_iterator_range (iter, &start, &end);
          bango_layout_index_to_pos (layout, start, &start_pos);
          bango_layout_index_to_pos (layout, end, &end_pos);

          if (links == NULL)
            {
              g_warning ("Ran out of links");
              break;
            }
          link = links->data;
          links = links->next;
          link->start = start;
          link->end = end;
        }
      } while (bango_attr_iterator_next (iter));

    bango_attr_iterator_destroy (iter);
}

static gboolean
btk_label_activate_link (BtkLabel    *label,
                         const gchar *uri)
{
  BtkWidget *widget = BTK_WIDGET (label);
  GError *error = NULL;

  if (!btk_show_uri (btk_widget_get_screen (widget),
                     uri, btk_get_current_event_time (), &error))
    {
      g_warning ("Unable to show '%s': %s", uri, error->message);
      g_error_free (error);
    }

  return TRUE;
}

static void
emit_activate_link (BtkLabel     *label,
                    BtkLabelLink *link)
{
  gboolean handled;

  g_signal_emit (label, signals[ACTIVATE_LINK], 0, link->uri, &handled);
  if (handled && label->track_links && !link->visited)
    {
      link->visited = TRUE;
      /* FIXME: shouldn't have to redo everything here */
      btk_label_recalculate (label);
    }
}

static void
btk_label_activate_current_link (BtkLabel *label)
{
  BtkLabelLink *link;
  BtkWidget *widget = BTK_WIDGET (label);

  link = btk_label_get_focus_link (label);

  if (link)
    {
      emit_activate_link (label, link);
    }
  else
    {
      BtkWidget *toplevel;
      BtkWindow *window;

      toplevel = btk_widget_get_toplevel (widget);
      if (BTK_IS_WINDOW (toplevel))
        {
          window = BTK_WINDOW (toplevel);

          if (window &&
              window->default_widget != widget &&
              !(widget == window->focus_widget &&
                (!window->default_widget || !btk_widget_is_sensitive (window->default_widget))))
            btk_window_activate_default (window);
        }
    }
}

static BtkLabelLink *
btk_label_get_current_link (BtkLabel *label)
{
  BtkLabelLink *link;

  if (!label->select_info)
    return NULL;

  if (label->select_info->link_clicked)
    link = label->select_info->active_link;
  else
    link = btk_label_get_focus_link (label);

  return link;
}

/**
 * btk_label_get_current_uri:
 * @label: a #BtkLabel
 *
 * Returns the URI for the currently active link in the label.
 * The active link is the one under the mouse pointer or, in a
 * selectable label, the link in which the text cursor is currently
 * positioned.
 *
 * This function is intended for use in a #BtkLabel::activate-link handler
 * or for use in a #BtkWidget::query-tooltip handler.
 *
 * Returns: the currently active URI. The string is owned by BTK+ and must
 *   not be freed or modified.
 *
 * Since: 2.18
 */
const gchar *
btk_label_get_current_uri (BtkLabel *label)
{
  BtkLabelLink *link;
  g_return_val_if_fail (BTK_IS_LABEL (label), NULL);

  link = btk_label_get_current_link (label);

  if (link)
    return link->uri;

  return NULL;
}

/**
 * btk_label_set_track_visited_links:
 * @label: a #BtkLabel
 * @track_links: %TRUE to track visited links
 *
 * Sets whether the label should keep track of clicked
 * links (and use a different color for them).
 *
 * Since: 2.18
 */
void
btk_label_set_track_visited_links (BtkLabel *label,
                                   gboolean  track_links)
{
  g_return_if_fail (BTK_IS_LABEL (label));

  track_links = track_links != FALSE;

  if (label->track_links != track_links)
    {
      label->track_links = track_links;

      /* FIXME: shouldn't have to redo everything here */
      btk_label_recalculate (label);

      g_object_notify (B_OBJECT (label), "track-visited-links");
    }
}

/**
 * btk_label_get_track_visited_links:
 * @label: a #BtkLabel
 *
 * Returns whether the label is currently keeping track
 * of clicked links.
 *
 * Returns: %TRUE if clicked links are remembered
 *
 * Since: 2.18
 */
gboolean
btk_label_get_track_visited_links (BtkLabel *label)
{
  g_return_val_if_fail (BTK_IS_LABEL (label), FALSE);

  return label->track_links;
}

static gboolean
btk_label_query_tooltip (BtkWidget  *widget,
                         gint        x,
                         gint        y,
                         gboolean    keyboard_tip,
                         BtkTooltip *tooltip)
{
  BtkLabel *label = BTK_LABEL (widget);
  BtkLabelSelectionInfo *info = label->select_info;
  gint index = -1;
  GList *l;

  if (info && info->links)
    {
      if (keyboard_tip)
        {
          if (info->selection_anchor == info->selection_end)
            index = info->selection_anchor;
        }
      else
        {
          if (!get_layout_index (label, x, y, &index))
            index = -1;
        }

      if (index != -1)
        {
          for (l = info->links; l != NULL; l = l->next)
            {
              BtkLabelLink *link = l->data;
              if (index >= link->start && index <= link->end)
                {
                  if (link->title)
                    {
                      btk_tooltip_set_markup (tooltip, link->title);
                      return TRUE;
                    }
                  break;
                }
            }
        }
    }

  return BTK_WIDGET_CLASS (btk_label_parent_class)->query_tooltip (widget,
                                                                   x, y,
                                                                   keyboard_tip,
                                                                   tooltip);
}


#define __BTK_LABEL_C__
#include "btkaliasdef.c"
