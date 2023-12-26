/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * BtkToolbar copyright (C) Federico Mena
 *
 * Copyright (C) 2002 Anders Carlsson <andersca@bunny.org>
 * Copyright (C) 2002 James Henstridge <james@daa.com.au>
 * Copyright (C) 2003, 2004 Soeren Sandmann <sandmann@daimi.au.dk>
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

#undef BTK_DISABLE_DEPRECATED

#include "config.h"

#include <math.h>
#include <string.h>

#include <bdk/bdkkeysyms.h>

#include "btkarrow.h"
#include "btkbindings.h"
#include "btkhbox.h"
#include "btkimage.h"
#include "btklabel.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkmenu.h"
#include "btkorientable.h"
#include "btkradiobutton.h"
#include "btkradiotoolbutton.h"
#include "btkseparatormenuitem.h"
#include "btkseparatortoolitem.h"
#include "btkstock.h"
#include "btktoolbar.h"
#include "btktoolshell.h"
#include "btkvbox.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

typedef struct _ToolbarContent ToolbarContent;

#define DEFAULT_IPADDING    0

#define DEFAULT_SPACE_SIZE  12
#define DEFAULT_SPACE_STYLE BTK_TOOLBAR_SPACE_LINE
#define SPACE_LINE_DIVISION 10.0
#define SPACE_LINE_START    2.0
#define SPACE_LINE_END      8.0

#define DEFAULT_ICON_SIZE BTK_ICON_SIZE_LARGE_TOOLBAR
#define DEFAULT_TOOLBAR_STYLE BTK_TOOLBAR_BOTH
#define DEFAULT_ANIMATION_STATE TRUE

#define MAX_HOMOGENEOUS_N_CHARS 13 /* Items that are wider than this do not participate
				    * in the homogeneous game. In units of
				    * bango_font_get_estimated_char_width().
				    */
#define SLIDE_SPEED 600.0	   /* How fast the items slide, in pixels per second */
#define ACCEL_THRESHOLD 0.18	   /* After how much time in seconds will items start speeding up */

#define MIXED_API_WARNING						\
    "Mixing deprecated and non-deprecated BtkToolbar API is not allowed"


/* Properties */
enum {
  PROP_0,
  PROP_ORIENTATION,
  PROP_TOOLBAR_STYLE,
  PROP_SHOW_ARROW,
  PROP_TOOLTIPS,
  PROP_ICON_SIZE,
  PROP_ICON_SIZE_SET
};

/* Child properties */
enum {
  CHILD_PROP_0,
  CHILD_PROP_EXPAND,
  CHILD_PROP_HOMOGENEOUS
};

/* Signals */
enum {
  ORIENTATION_CHANGED,
  STYLE_CHANGED,
  POPUP_CONTEXT_MENU,
  FOCUS_HOME_OR_END,
  LAST_SIGNAL
};

/* API mode */
typedef enum {
  DONT_KNOW,
  OLD_API,
  NEW_API
} ApiMode;

typedef enum {
  TOOL_ITEM,
  COMPATIBILITY
} ContentType;

typedef enum {
  NOT_ALLOCATED,
  NORMAL,
  HIDDEN,
  OVERFLOWN
} ItemState;

struct _BtkToolbarPrivate
{
  GList	*	content;
  
  BtkWidget *	arrow;
  BtkWidget *	arrow_button;
  BtkMenu *	menu;
  
  BdkWindow *	event_window;
  ApiMode	api_mode;
  BtkSettings *	settings;
  int		idle_id;
  BtkToolItem *	highlight_tool_item;
  gint		max_homogeneous_pixels;
  
  GTimer *	timer;

  gulong        settings_connection;

  guint         show_arrow : 1;
  guint         need_sync : 1;
  guint         is_sliding : 1;
  guint         need_rebuild : 1;  /* whether the overflow menu should be regenerated */
  guint         animation : 1;
};

static void       btk_toolbar_set_property         (BObject             *object,
						    guint                prop_id,
						    const BValue        *value,
						    BParamSpec          *pspec);
static void       btk_toolbar_get_property         (BObject             *object,
						    guint                prop_id,
						    BValue              *value,
						    BParamSpec          *pspec);
static gint       btk_toolbar_expose               (BtkWidget           *widget,
						    BdkEventExpose      *event);
static void       btk_toolbar_realize              (BtkWidget           *widget);
static void       btk_toolbar_unrealize            (BtkWidget           *widget);
static void       btk_toolbar_size_request         (BtkWidget           *widget,
						    BtkRequisition      *requisition);
static void       btk_toolbar_size_allocate        (BtkWidget           *widget,
						    BtkAllocation       *allocation);
static void       btk_toolbar_style_set            (BtkWidget           *widget,
						    BtkStyle            *prev_style);
static gboolean   btk_toolbar_focus                (BtkWidget           *widget,
						    BtkDirectionType     dir);
static void       btk_toolbar_move_focus           (BtkWidget           *widget,
						    BtkDirectionType     dir);
static void       btk_toolbar_screen_changed       (BtkWidget           *widget,
						    BdkScreen           *previous_screen);
static void       btk_toolbar_map                  (BtkWidget           *widget);
static void       btk_toolbar_unmap                (BtkWidget           *widget);
static void       btk_toolbar_set_child_property   (BtkContainer        *container,
						    BtkWidget           *child,
						    guint                property_id,
						    const BValue        *value,
						    BParamSpec          *pspec);
static void       btk_toolbar_get_child_property   (BtkContainer        *container,
						    BtkWidget           *child,
						    guint                property_id,
						    BValue              *value,
						    BParamSpec          *pspec);
static void       btk_toolbar_dispose              (BObject             *object);
static void       btk_toolbar_finalize             (BObject             *object);
static void       btk_toolbar_show_all             (BtkWidget           *widget);
static void       btk_toolbar_hide_all             (BtkWidget           *widget);
static void       btk_toolbar_add                  (BtkContainer        *container,
						    BtkWidget           *widget);
static void       btk_toolbar_remove               (BtkContainer        *container,
						    BtkWidget           *widget);
static void       btk_toolbar_forall               (BtkContainer        *container,
						    gboolean             include_internals,
						    BtkCallback          callback,
						    gpointer             callback_data);
static GType      btk_toolbar_child_type           (BtkContainer        *container);
static void       btk_toolbar_orientation_changed  (BtkToolbar          *toolbar,
						    BtkOrientation       orientation);
static void       btk_toolbar_real_style_changed   (BtkToolbar          *toolbar,
						    BtkToolbarStyle      style);
static gboolean   btk_toolbar_focus_home_or_end    (BtkToolbar          *toolbar,
						    gboolean             focus_home);
static gboolean   btk_toolbar_button_press         (BtkWidget           *toolbar,
						    BdkEventButton      *event);
static gboolean   btk_toolbar_arrow_button_press   (BtkWidget           *button,
						    BdkEventButton      *event,
						    BtkToolbar          *toolbar);
static void       btk_toolbar_arrow_button_clicked (BtkWidget           *button,
						    BtkToolbar          *toolbar);
static void       btk_toolbar_update_button_relief (BtkToolbar          *toolbar);
static gboolean   btk_toolbar_popup_menu           (BtkWidget           *toolbar);
static BtkWidget *internal_insert_element          (BtkToolbar          *toolbar,
						    BtkToolbarChildType  type,
						    BtkWidget           *widget,
						    const char          *text,
						    const char          *tooltip_text,
						    const char          *tooltip_private_text,
						    BtkWidget           *icon,
						    GCallback            callback,
						    gpointer             user_data,
						    gint                 position,
						    gboolean             use_stock);
static void       btk_toolbar_reconfigured         (BtkToolbar          *toolbar);
static gboolean   btk_toolbar_check_new_api        (BtkToolbar          *toolbar);
static gboolean   btk_toolbar_check_old_api        (BtkToolbar          *toolbar);

static BtkReliefStyle       get_button_relief    (BtkToolbar *toolbar);
static gint                 get_internal_padding (BtkToolbar *toolbar);
static gint                 get_max_child_expand (BtkToolbar *toolbar);
static BtkShadowType        get_shadow_type      (BtkToolbar *toolbar);
static gint                 get_space_size       (BtkToolbar *toolbar);
static BtkToolbarSpaceStyle get_space_style      (BtkToolbar *toolbar);

/* methods on ToolbarContent 'class' */
static ToolbarContent *toolbar_content_new_tool_item        (BtkToolbar          *toolbar,
							     BtkToolItem         *item,
							     gboolean             is_placeholder,
							     gint                 pos);
static ToolbarContent *toolbar_content_new_compatibility    (BtkToolbar          *toolbar,
							     BtkToolbarChildType  type,
							     BtkWidget           *widget,
							     BtkWidget           *icon,
							     BtkWidget           *label,
							     gint                 pos);
static void            toolbar_content_remove               (ToolbarContent      *content,
							     BtkToolbar          *toolbar);
static void            toolbar_content_free                 (ToolbarContent      *content);
static void            toolbar_content_expose               (ToolbarContent      *content,
							     BtkContainer        *container,
							     BdkEventExpose      *expose);
static gboolean        toolbar_content_visible              (ToolbarContent      *content,
							     BtkToolbar          *toolbar);
static void            toolbar_content_size_request         (ToolbarContent      *content,
							     BtkToolbar          *toolbar,
							     BtkRequisition      *requisition);
static gboolean        toolbar_content_is_homogeneous       (ToolbarContent      *content,
							     BtkToolbar          *toolbar);
static gboolean        toolbar_content_is_placeholder       (ToolbarContent      *content);
static gboolean        toolbar_content_disappearing         (ToolbarContent      *content);
static ItemState       toolbar_content_get_state            (ToolbarContent      *content);
static gboolean        toolbar_content_child_visible        (ToolbarContent      *content);
static void            toolbar_content_get_goal_allocation  (ToolbarContent      *content,
							     BtkAllocation       *allocation);
static void            toolbar_content_get_allocation       (ToolbarContent      *content,
							     BtkAllocation       *allocation);
static void            toolbar_content_set_start_allocation (ToolbarContent      *content,
							     BtkAllocation       *new_start_allocation);
static void            toolbar_content_get_start_allocation (ToolbarContent      *content,
							     BtkAllocation       *start_allocation);
static gboolean        toolbar_content_get_expand           (ToolbarContent      *content);
static void            toolbar_content_set_goal_allocation  (ToolbarContent      *content,
							     BtkAllocation       *allocation);
static void            toolbar_content_set_child_visible    (ToolbarContent      *content,
							     BtkToolbar          *toolbar,
							     gboolean             visible);
static void            toolbar_content_size_allocate        (ToolbarContent      *content,
							     BtkAllocation       *allocation);
static void            toolbar_content_set_state            (ToolbarContent      *content,
							     ItemState            new_state);
static BtkWidget *     toolbar_content_get_widget           (ToolbarContent      *content);
static void            toolbar_content_set_disappearing     (ToolbarContent      *content,
							     gboolean             disappearing);
static void            toolbar_content_set_size_request     (ToolbarContent      *content,
							     gint                 width,
							     gint                 height);
static void            toolbar_content_toolbar_reconfigured (ToolbarContent      *content,
							     BtkToolbar          *toolbar);
static BtkWidget *     toolbar_content_retrieve_menu_item   (ToolbarContent      *content);
static gboolean        toolbar_content_has_proxy_menu_item  (ToolbarContent	 *content);
static gboolean        toolbar_content_is_separator         (ToolbarContent      *content);
static void            toolbar_content_show_all             (ToolbarContent      *content);
static void            toolbar_content_hide_all             (ToolbarContent      *content);
static void	       toolbar_content_set_expand	    (ToolbarContent      *content,
							     gboolean		  expand);

static void            toolbar_tool_shell_iface_init        (BtkToolShellIface   *iface);
static BtkIconSize     toolbar_get_icon_size                (BtkToolShell        *shell);
static BtkOrientation  toolbar_get_orientation              (BtkToolShell        *shell);
static BtkToolbarStyle toolbar_get_style                    (BtkToolShell        *shell);
static BtkReliefStyle  toolbar_get_relief_style             (BtkToolShell        *shell);
static void            toolbar_rebuild_menu                 (BtkToolShell        *shell);

#define BTK_TOOLBAR_GET_PRIVATE(o)  \
  (B_TYPE_INSTANCE_GET_PRIVATE ((o), BTK_TYPE_TOOLBAR, BtkToolbarPrivate))


G_DEFINE_TYPE_WITH_CODE (BtkToolbar, btk_toolbar, BTK_TYPE_CONTAINER,
                         G_IMPLEMENT_INTERFACE (BTK_TYPE_TOOL_SHELL,
                                                toolbar_tool_shell_iface_init)
                         G_IMPLEMENT_INTERFACE (BTK_TYPE_ORIENTABLE,
                                                NULL))

static guint toolbar_signals[LAST_SIGNAL] = { 0 };


static void
add_arrow_bindings (BtkBindingSet   *binding_set,
		    guint            keysym,
		    BtkDirectionType dir)
{
  guint keypad_keysym = keysym - BDK_Left + BDK_KP_Left;
  
  btk_binding_entry_add_signal (binding_set, keysym, 0,
                                "move-focus", 1,
                                BTK_TYPE_DIRECTION_TYPE, dir);
  btk_binding_entry_add_signal (binding_set, keypad_keysym, 0,
                                "move-focus", 1,
                                BTK_TYPE_DIRECTION_TYPE, dir);
}

static void
add_ctrl_tab_bindings (BtkBindingSet    *binding_set,
		       BdkModifierType   modifiers,
		       BtkDirectionType  direction)
{
  btk_binding_entry_add_signal (binding_set,
				BDK_Tab, BDK_CONTROL_MASK | modifiers,
				"move-focus", 1,
				BTK_TYPE_DIRECTION_TYPE, direction);
  btk_binding_entry_add_signal (binding_set,
				BDK_KP_Tab, BDK_CONTROL_MASK | modifiers,
				"move-focus", 1,
				BTK_TYPE_DIRECTION_TYPE, direction);
}

static void
btk_toolbar_class_init (BtkToolbarClass *klass)
{
  BObjectClass *bobject_class;
  BtkWidgetClass *widget_class;
  BtkContainerClass *container_class;
  BtkBindingSet *binding_set;
  
  bobject_class = (BObjectClass *)klass;
  widget_class = (BtkWidgetClass *)klass;
  container_class = (BtkContainerClass *)klass;
  
  bobject_class->set_property = btk_toolbar_set_property;
  bobject_class->get_property = btk_toolbar_get_property;
  bobject_class->dispose = btk_toolbar_dispose;
  bobject_class->finalize = btk_toolbar_finalize;

  widget_class->button_press_event = btk_toolbar_button_press;
  widget_class->expose_event = btk_toolbar_expose;
  widget_class->size_request = btk_toolbar_size_request;
  widget_class->size_allocate = btk_toolbar_size_allocate;
  widget_class->style_set = btk_toolbar_style_set;
  widget_class->focus = btk_toolbar_focus;

  /* need to override the base class function via override_class_handler,
   * because the signal slot is not available in BtkWidgetClass
   */
  g_signal_override_class_handler ("move-focus",
                                   BTK_TYPE_TOOLBAR,
                                   G_CALLBACK (btk_toolbar_move_focus));

  widget_class->screen_changed = btk_toolbar_screen_changed;
  widget_class->realize = btk_toolbar_realize;
  widget_class->unrealize = btk_toolbar_unrealize;
  widget_class->map = btk_toolbar_map;
  widget_class->unmap = btk_toolbar_unmap;
  widget_class->popup_menu = btk_toolbar_popup_menu;
  widget_class->show_all = btk_toolbar_show_all;
  widget_class->hide_all = btk_toolbar_hide_all;
  
  container_class->add    = btk_toolbar_add;
  container_class->remove = btk_toolbar_remove;
  container_class->forall = btk_toolbar_forall;
  container_class->child_type = btk_toolbar_child_type;
  container_class->get_child_property = btk_toolbar_get_child_property;
  container_class->set_child_property = btk_toolbar_set_child_property;
  
  klass->orientation_changed = btk_toolbar_orientation_changed;
  klass->style_changed = btk_toolbar_real_style_changed;
  
  /**
   * BtkToolbar::orientation-changed:
   * @toolbar: the object which emitted the signal
   * @orientation: the new #BtkOrientation of the toolbar
   *
   * Emitted when the orientation of the toolbar changes.
   */
  toolbar_signals[ORIENTATION_CHANGED] =
    g_signal_new (I_("orientation-changed"),
		  B_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkToolbarClass, orientation_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__ENUM,
		  B_TYPE_NONE, 1,
		  BTK_TYPE_ORIENTATION);
  /**
   * BtkToolbar::style-changed:
   * @toolbar: The #BtkToolbar which emitted the signal
   * @style: the new #BtkToolbarStyle of the toolbar
   *
   * Emitted when the style of the toolbar changes. 
   */
  toolbar_signals[STYLE_CHANGED] =
    g_signal_new (I_("style-changed"),
		  B_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkToolbarClass, style_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__ENUM,
		  B_TYPE_NONE, 1,
		  BTK_TYPE_TOOLBAR_STYLE);
  /**
   * BtkToolbar::popup-context-menu:
   * @toolbar: the #BtkToolbar which emitted the signal
   * @x: the x coordinate of the point where the menu should appear
   * @y: the y coordinate of the point where the menu should appear
   * @button: the mouse button the user pressed, or -1
   *
   * Emitted when the user right-clicks the toolbar or uses the
   * keybinding to display a popup menu.
   *
   * Application developers should handle this signal if they want
   * to display a context menu on the toolbar. The context-menu should
   * appear at the coordinates given by @x and @y. The mouse button
   * number is given by the @button parameter. If the menu was popped
   * up using the keybaord, @button is -1.
   *
   * Return value: return %TRUE if the signal was handled, %FALSE if not
   */
  toolbar_signals[POPUP_CONTEXT_MENU] =
    g_signal_new (I_("popup-context-menu"),
		  B_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkToolbarClass, popup_context_menu),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__INT_INT_INT,
		  B_TYPE_BOOLEAN, 3,
		  B_TYPE_INT, B_TYPE_INT,
		  B_TYPE_INT);

  /**
   * BtkToolbar::focus-home-or-end:
   * @toolbar: the #BtkToolbar which emitted the signal
   * @focus_home: %TRUE if the first item should be focused
   *
   * A keybinding signal used internally by BTK+. This signal can't
   * be used in application code
   *
   * Return value: %TRUE if the signal was handled, %FALSE if not
   */
  toolbar_signals[FOCUS_HOME_OR_END] =
    g_signal_new_class_handler (I_("focus-home-or-end"),
                                B_OBJECT_CLASS_TYPE (klass),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (btk_toolbar_focus_home_or_end),
                                NULL, NULL,
                                _btk_marshal_BOOLEAN__BOOLEAN,
                                B_TYPE_BOOLEAN, 1,
                                B_TYPE_BOOLEAN);

  /* properties */
  g_object_class_override_property (bobject_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  g_object_class_install_property (bobject_class,
				   PROP_TOOLBAR_STYLE,
				   g_param_spec_enum ("toolbar-style",
 						      P_("Toolbar Style"),
 						      P_("How to draw the toolbar"),
 						      BTK_TYPE_TOOLBAR_STYLE,
 						      DEFAULT_TOOLBAR_STYLE,
 						      BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_SHOW_ARROW,
				   g_param_spec_boolean ("show-arrow",
							 P_("Show Arrow"),
							 P_("If an arrow should be shown if the toolbar doesn't fit"),
							 TRUE,
							 BTK_PARAM_READWRITE));
  

  /**
   * BtkToolbar:tooltips:
   * 
   * If the tooltips of the toolbar should be active or not.
   * 
   * Since: 2.8
   */
  g_object_class_install_property (bobject_class,
				   PROP_TOOLTIPS,
				   g_param_spec_boolean ("tooltips",
							 P_("Tooltips"),
							 P_("If the tooltips of the toolbar should be active or not"),
							 TRUE,
							 BTK_PARAM_READWRITE));
  

  /**
   * BtkToolbar:icon-size:
   *
   * The size of the icons in a toolbar is normally determined by
   * the toolbar-icon-size setting. When this property is set, it 
   * overrides the setting. 
   * 
   * This should only be used for special-purpose toolbars, normal
   * application toolbars should respect the user preferences for the
   * size of icons.
   *
   * Since: 2.10
   */
  g_object_class_install_property (bobject_class,
				   PROP_ICON_SIZE,
				   g_param_spec_int ("icon-size",
						     P_("Icon size"),
						     P_("Size of icons in this toolbar"),
						     0, G_MAXINT,
						     DEFAULT_ICON_SIZE,
						     BTK_PARAM_READWRITE));  

  /**
   * BtkToolbar:icon-size-set:
   *
   * Is %TRUE if the icon-size property has been set.
   *
   * Since: 2.10
   */
  g_object_class_install_property (bobject_class,
				   PROP_ICON_SIZE_SET,
				   g_param_spec_boolean ("icon-size-set",
							 P_("Icon size set"),
							 P_("Whether the icon-size property has been set"),
							 FALSE,
							 BTK_PARAM_READWRITE));  

  /* child properties */
  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_EXPAND,
					      g_param_spec_boolean ("expand", 
								    P_("Expand"), 
								    P_("Whether the item should receive extra space when the toolbar grows"),
								    FALSE,
								    BTK_PARAM_READWRITE));
  
  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_HOMOGENEOUS,
					      g_param_spec_boolean ("homogeneous", 
								    P_("Homogeneous"), 
								    P_("Whether the item should be the same size as other homogeneous items"),
								    FALSE,
								    BTK_PARAM_READWRITE));
  
  /* style properties */
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("space-size",
							     P_("Spacer size"),
							     P_("Size of spacers"),
							     0,
							     G_MAXINT,
                                                             DEFAULT_SPACE_SIZE,
							     BTK_PARAM_READABLE));
  
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("internal-padding",
							     P_("Internal padding"),
							     P_("Amount of border space between the toolbar shadow and the buttons"),
							     0,
							     G_MAXINT,
                                                             DEFAULT_IPADDING,
                                                             BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("max-child-expand",
                                                             P_("Maximum child expand"),
                                                             P_("Maximum amount of space an expandable item will be given"),
                                                             0,
                                                             G_MAXINT,
                                                             G_MAXINT,
                                                             BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_enum ("space-style",
							      P_("Space style"),
							      P_("Whether spacers are vertical lines or just blank"),
                                                              BTK_TYPE_TOOLBAR_SPACE_STYLE,
                                                              DEFAULT_SPACE_STYLE,
                                                              BTK_PARAM_READABLE));
  
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_enum ("button-relief",
							      P_("Button relief"),
							      P_("Type of bevel around toolbar buttons"),
                                                              BTK_TYPE_RELIEF_STYLE,
                                                              BTK_RELIEF_NONE,
                                                              BTK_PARAM_READABLE));
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("shadow-type",
                                                              P_("Shadow type"),
                                                              P_("Style of bevel around the toolbar"),
                                                              BTK_TYPE_SHADOW_TYPE,
                                                              BTK_SHADOW_OUT,
                                                              BTK_PARAM_READABLE));

  binding_set = btk_binding_set_by_class (klass);
  
  add_arrow_bindings (binding_set, BDK_Left, BTK_DIR_LEFT);
  add_arrow_bindings (binding_set, BDK_Right, BTK_DIR_RIGHT);
  add_arrow_bindings (binding_set, BDK_Up, BTK_DIR_UP);
  add_arrow_bindings (binding_set, BDK_Down, BTK_DIR_DOWN);
  
  btk_binding_entry_add_signal (binding_set, BDK_KP_Home, 0,
                                "focus-home-or-end", 1,
				B_TYPE_BOOLEAN, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_Home, 0,
                                "focus-home-or-end", 1,
				B_TYPE_BOOLEAN, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_End, 0,
                                "focus-home-or-end", 1,
				B_TYPE_BOOLEAN, FALSE);
  btk_binding_entry_add_signal (binding_set, BDK_End, 0,
                                "focus-home-or-end", 1,
				B_TYPE_BOOLEAN, FALSE);
  
  add_ctrl_tab_bindings (binding_set, 0, BTK_DIR_TAB_FORWARD);
  add_ctrl_tab_bindings (binding_set, BDK_SHIFT_MASK, BTK_DIR_TAB_BACKWARD);
  
  g_type_class_add_private (bobject_class, sizeof (BtkToolbarPrivate));  
}

static void
toolbar_tool_shell_iface_init (BtkToolShellIface *iface)
{
  iface->get_icon_size    = toolbar_get_icon_size;
  iface->get_orientation  = toolbar_get_orientation;
  iface->get_style        = toolbar_get_style;
  iface->get_relief_style = toolbar_get_relief_style;
  iface->rebuild_menu     = toolbar_rebuild_menu;
}

static void
btk_toolbar_init (BtkToolbar *toolbar)
{
  BtkToolbarPrivate *priv;
  
  btk_widget_set_can_focus (BTK_WIDGET (toolbar), FALSE);
  btk_widget_set_has_window (BTK_WIDGET (toolbar), FALSE);
  
  priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  
  toolbar->orientation = BTK_ORIENTATION_HORIZONTAL;
  toolbar->style = DEFAULT_TOOLBAR_STYLE;
  toolbar->icon_size = DEFAULT_ICON_SIZE;
  priv->animation = DEFAULT_ANIMATION_STATE;
  toolbar->tooltips = btk_tooltips_new ();
  g_object_ref_sink (toolbar->tooltips);
  
  priv->arrow_button = btk_toggle_button_new ();
  g_signal_connect (priv->arrow_button, "button-press-event",
		    G_CALLBACK (btk_toolbar_arrow_button_press), toolbar);
  g_signal_connect (priv->arrow_button, "clicked",
		    G_CALLBACK (btk_toolbar_arrow_button_clicked), toolbar);
  btk_button_set_relief (BTK_BUTTON (priv->arrow_button),
			 get_button_relief (toolbar));
  
  priv->api_mode = DONT_KNOW;
  
  btk_button_set_focus_on_click (BTK_BUTTON (priv->arrow_button), FALSE);
  
  priv->arrow = btk_arrow_new (BTK_ARROW_DOWN, BTK_SHADOW_NONE);
  btk_widget_set_name (priv->arrow, "btk-toolbar-arrow");
  btk_widget_show (priv->arrow);
  btk_container_add (BTK_CONTAINER (priv->arrow_button), priv->arrow);
  
  btk_widget_set_parent (priv->arrow_button, BTK_WIDGET (toolbar));
  
  /* which child position a drop will occur at */
  priv->menu = NULL;
  priv->show_arrow = TRUE;
  priv->settings = NULL;
  
  priv->max_homogeneous_pixels = -1;
  
  priv->timer = g_timer_new ();
}

static void
btk_toolbar_set_property (BObject      *object,
			  guint         prop_id,
			  const BValue *value,
			  BParamSpec   *pspec)
{
  BtkToolbar *toolbar = BTK_TOOLBAR (object);
  
  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_signal_emit (toolbar, toolbar_signals[ORIENTATION_CHANGED], 0,
                     b_value_get_enum (value));
      break;
    case PROP_TOOLBAR_STYLE:
      btk_toolbar_set_style (toolbar, b_value_get_enum (value));
      break;
    case PROP_SHOW_ARROW:
      btk_toolbar_set_show_arrow (toolbar, b_value_get_boolean (value));
      break;
    case PROP_TOOLTIPS:
      btk_toolbar_set_tooltips (toolbar, b_value_get_boolean (value));
      break;
    case PROP_ICON_SIZE:
      btk_toolbar_set_icon_size (toolbar, b_value_get_int (value));
      break;
    case PROP_ICON_SIZE_SET:
      if (b_value_get_boolean (value))
	toolbar->icon_size_set = TRUE;
      else
	btk_toolbar_unset_icon_size (toolbar);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_toolbar_get_property (BObject    *object,
			  guint       prop_id,
			  BValue     *value,
			  BParamSpec *pspec)
{
  BtkToolbar *toolbar = BTK_TOOLBAR (object);
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  
  switch (prop_id)
    {
    case PROP_ORIENTATION:
      b_value_set_enum (value, toolbar->orientation);
      break;
    case PROP_TOOLBAR_STYLE:
      b_value_set_enum (value, toolbar->style);
      break;
    case PROP_SHOW_ARROW:
      b_value_set_boolean (value, priv->show_arrow);
      break;
    case PROP_TOOLTIPS:
      b_value_set_boolean (value, btk_toolbar_get_tooltips (toolbar));
      break;
    case PROP_ICON_SIZE:
      b_value_set_int (value, btk_toolbar_get_icon_size (toolbar));
      break;
    case PROP_ICON_SIZE_SET:
      b_value_set_boolean (value, toolbar->icon_size_set);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_toolbar_map (BtkWidget *widget)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (widget);
  
  BTK_WIDGET_CLASS (btk_toolbar_parent_class)->map (widget);
  
  if (priv->event_window)
    bdk_window_show_unraised (priv->event_window);
}

static void
btk_toolbar_unmap (BtkWidget *widget)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (widget);
  
  if (priv->event_window)
    bdk_window_hide (priv->event_window);
  
  BTK_WIDGET_CLASS (btk_toolbar_parent_class)->unmap (widget);
}

static void
btk_toolbar_realize (BtkWidget *widget)
{
  BtkToolbar *toolbar = BTK_TOOLBAR (widget);
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  
  BdkWindowAttr attributes;
  gint attributes_mask;
  gint border_width;
  
  btk_widget_set_realized (widget, TRUE);
  
  border_width = BTK_CONTAINER (widget)->border_width;
  
  attributes.wclass = BDK_INPUT_ONLY;
  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x + border_width;
  attributes.y = widget->allocation.y + border_width;
  attributes.width = widget->allocation.width - border_width * 2;
  attributes.height = widget->allocation.height - border_width * 2;
  attributes.event_mask = btk_widget_get_events (widget);
  attributes.event_mask |= (BDK_BUTTON_PRESS_MASK |
			    BDK_BUTTON_RELEASE_MASK |
			    BDK_ENTER_NOTIFY_MASK |
			    BDK_LEAVE_NOTIFY_MASK);
  
  attributes_mask = BDK_WA_X | BDK_WA_Y;
  
  widget->window = btk_widget_get_parent_window (widget);
  g_object_ref (widget->window);
  widget->style = btk_style_attach (widget->style, widget->window);
  
  priv->event_window = bdk_window_new (btk_widget_get_parent_window (widget),
				       &attributes, attributes_mask);
  bdk_window_set_user_data (priv->event_window, toolbar);
}

static void
btk_toolbar_unrealize (BtkWidget *widget)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (widget);
  
  if (priv->event_window)
    {
      bdk_window_set_user_data (priv->event_window, NULL);
      bdk_window_destroy (priv->event_window);
      priv->event_window = NULL;
    }

  BTK_WIDGET_CLASS (btk_toolbar_parent_class)->unrealize (widget);
}

static gint
btk_toolbar_expose (BtkWidget      *widget,
		    BdkEventExpose *event)
{
  BtkToolbar *toolbar = BTK_TOOLBAR (widget);
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  
  GList *list;
  gint border_width;
  
  border_width = BTK_CONTAINER (widget)->border_width;
  
  if (btk_widget_is_drawable (widget))
    {
      btk_paint_box (widget->style,
		     widget->window,
                     btk_widget_get_state (widget),
                     get_shadow_type (toolbar),
		     &event->area, widget, "toolbar",
		     border_width + widget->allocation.x,
                     border_width + widget->allocation.y,
		     widget->allocation.width - 2 * border_width,
                     widget->allocation.height - 2 * border_width);
    }
  
  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      
      toolbar_content_expose (content, BTK_CONTAINER (widget), event);
    }
  
  btk_container_propagate_expose (BTK_CONTAINER (widget),
				  priv->arrow_button,
				  event);
  
  return FALSE;
}

static void
btk_toolbar_size_request (BtkWidget      *widget,
			  BtkRequisition *requisition)
{
  BtkToolbar *toolbar = BTK_TOOLBAR (widget);
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  GList *list;
  gint max_child_height;
  gint max_child_width;
  gint max_homogeneous_child_width;
  gint max_homogeneous_child_height;
  gint homogeneous_size;
  gint long_req;
  gint pack_front_size;
  gint ipadding;
  BtkRequisition arrow_requisition;
  
  max_homogeneous_child_width = 0;
  max_homogeneous_child_height = 0;
  max_child_width = 0;
  max_child_height = 0;
  for (list = priv->content; list != NULL; list = list->next)
    {
      BtkRequisition requisition;
      ToolbarContent *content = list->data;
      
      if (!toolbar_content_visible (content, toolbar))
	continue;
      
      toolbar_content_size_request (content, toolbar, &requisition);

      max_child_width = MAX (max_child_width, requisition.width);
      max_child_height = MAX (max_child_height, requisition.height);
      
      if (toolbar_content_is_homogeneous (content, toolbar))
	{
	  max_homogeneous_child_width = MAX (max_homogeneous_child_width, requisition.width);
	  max_homogeneous_child_height = MAX (max_homogeneous_child_height, requisition.height);
	}
    }
  
  if (toolbar->orientation == BTK_ORIENTATION_HORIZONTAL)
    homogeneous_size = max_homogeneous_child_width;
  else
    homogeneous_size = max_homogeneous_child_height;
  
  pack_front_size = 0;
  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      guint size;
      
      if (!toolbar_content_visible (content, toolbar))
	continue;

      if (toolbar_content_is_homogeneous (content, toolbar))
	{
	  size = homogeneous_size;
	}
      else
	{
	  BtkRequisition requisition;
	  
	  toolbar_content_size_request (content, toolbar, &requisition);
	  
	  if (toolbar->orientation == BTK_ORIENTATION_HORIZONTAL)
	    size = requisition.width;
	  else
	    size = requisition.height;
	}

      pack_front_size += size;
    }
  
  if (priv->show_arrow && priv->api_mode == NEW_API)
    {
      btk_widget_size_request (priv->arrow_button, &arrow_requisition);
      
      if (toolbar->orientation == BTK_ORIENTATION_HORIZONTAL)
	long_req = arrow_requisition.width;
      else
	long_req = arrow_requisition.height;
      
      /* There is no point requesting space for the arrow if that would take
       * up more space than all the items combined
       */
      long_req = MIN (long_req, pack_front_size);
    }
  else
    {
      arrow_requisition.height = 0;
      arrow_requisition.width = 0;
      
      long_req = pack_front_size;
    }
  
  if (toolbar->orientation == BTK_ORIENTATION_HORIZONTAL)
    {
      requisition->width = long_req;
      requisition->height = MAX (max_child_height, arrow_requisition.height);
    }
  else
    {
      requisition->height = long_req;
      requisition->width = MAX (max_child_width, arrow_requisition.width);
    }
  
  /* Extra spacing */
  ipadding = get_internal_padding (toolbar);
  
  requisition->width += 2 * (ipadding + BTK_CONTAINER (toolbar)->border_width);
  requisition->height += 2 * (ipadding + BTK_CONTAINER (toolbar)->border_width);
  
  if (get_shadow_type (toolbar) != BTK_SHADOW_NONE)
    {
      requisition->width += 2 * widget->style->xthickness;
      requisition->height += 2 * widget->style->ythickness;
    }
  
  toolbar->button_maxw = max_homogeneous_child_width;
  toolbar->button_maxh = max_homogeneous_child_height;
}

static gint
position (BtkToolbar *toolbar,
          gint        from,
          gint        to,
          gdouble     elapsed)
{
  gint n_pixels;

  if (! BTK_TOOLBAR_GET_PRIVATE (toolbar)->animation)
    return to;

  if (elapsed <= ACCEL_THRESHOLD)
    {
      n_pixels = SLIDE_SPEED * elapsed;
    }
  else
    {
      /* The formula is a second degree polynomial in
       * @elapsed that has the line SLIDE_SPEED * @elapsed
       * as tangent for @elapsed == ACCEL_THRESHOLD.
       * This makes @n_pixels a smooth function of elapsed time.
       */
      n_pixels = (SLIDE_SPEED / ACCEL_THRESHOLD) * elapsed * elapsed -
	SLIDE_SPEED * elapsed + SLIDE_SPEED * ACCEL_THRESHOLD;
    }

  if (to > from)
    return MIN (from + n_pixels, to);
  else
    return MAX (from - n_pixels, to);
}

static void
compute_intermediate_allocation (BtkToolbar          *toolbar,
				 const BtkAllocation *start,
				 const BtkAllocation *goal,
				 BtkAllocation       *intermediate)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  gdouble elapsed = g_timer_elapsed (priv->timer, NULL);

  intermediate->x      = position (toolbar, start->x, goal->x, elapsed);
  intermediate->y      = position (toolbar, start->y, goal->y, elapsed);
  intermediate->width  = position (toolbar, start->x + start->width,
                                   goal->x + goal->width,
                                   elapsed) - intermediate->x;
  intermediate->height = position (toolbar, start->y + start->height,
                                   goal->y + goal->height,
                                   elapsed) - intermediate->y;
}

static void
fixup_allocation_for_rtl (gint           total_size,
			  BtkAllocation *allocation)
{
  allocation->x += (total_size - (2 * allocation->x + allocation->width));
}

static void
fixup_allocation_for_vertical (BtkAllocation *allocation)
{
  gint tmp;
  
  tmp = allocation->x;
  allocation->x = allocation->y;
  allocation->y = tmp;
  
  tmp = allocation->width;
  allocation->width = allocation->height;
  allocation->height = tmp;
}

static gint
get_item_size (BtkToolbar     *toolbar,
	       ToolbarContent *content)
{
  BtkRequisition requisition;
  
  toolbar_content_size_request (content, toolbar, &requisition);
  
  if (toolbar->orientation == BTK_ORIENTATION_HORIZONTAL)
    {
      if (toolbar_content_is_homogeneous (content, toolbar))
	return toolbar->button_maxw;
      else
	return requisition.width;
    }
  else
    {
      if (toolbar_content_is_homogeneous (content, toolbar))
	return toolbar->button_maxh;
      else
	return requisition.height;
    }
}

static gboolean
slide_idle_handler (gpointer data)
{
  BtkToolbar *toolbar = data;
  BtkToolbarPrivate *priv;
  GList *list;
  
  priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  
  if (priv->need_sync)
    {
      bdk_flush ();
      priv->need_sync = FALSE;
    }
  
  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      ItemState state;
      BtkAllocation goal_allocation;
      BtkAllocation allocation;
      gboolean cont;

      state = toolbar_content_get_state (content);
      toolbar_content_get_goal_allocation (content, &goal_allocation);
      toolbar_content_get_allocation (content, &allocation);
      
      cont = FALSE;
      
      if (state == NOT_ALLOCATED)
	{
	  /* an unallocated item means that size allocate has to
	   * called at least once more
	   */
	  cont = TRUE;
	}

      /* An invisible item with a goal allocation of
       * 0 is already at its goal.
       */
      if ((state == NORMAL || state == OVERFLOWN) &&
	  ((goal_allocation.width != 0 &&
	    goal_allocation.height != 0) ||
	   toolbar_content_child_visible (content)))
	{
	  if ((goal_allocation.x != allocation.x ||
	       goal_allocation.y != allocation.y ||
	       goal_allocation.width != allocation.width ||
	       goal_allocation.height != allocation.height))
	    {
	      /* An item is not in its right position yet. Note
	       * that OVERFLOWN items do get an allocation in
	       * btk_toolbar_size_allocate(). This way you can see
	       * them slide back in when you drag an item off the
	       * toolbar.
	       */
	      cont = TRUE;
	    }
	}

      if (toolbar_content_is_placeholder (content) &&
	  toolbar_content_disappearing (content) &&
	  toolbar_content_child_visible (content))
	{
	  /* A disappearing placeholder is still visible.
	   */
	     
	  cont = TRUE;
	}
      
      if (cont)
	{
	  btk_widget_queue_resize_no_redraw (BTK_WIDGET (toolbar));
	  
	  return TRUE;
	}
    }
  
  btk_widget_queue_resize_no_redraw (BTK_WIDGET (toolbar));

  priv->is_sliding = FALSE;
  priv->idle_id = 0;

  return FALSE;
}

static gboolean
rect_within (BtkAllocation *a1,
	     BtkAllocation *a2)
{
  return (a1->x >= a2->x                         &&
	  a1->x + a1->width <= a2->x + a2->width &&
	  a1->y >= a2->y			 &&
	  a1->y + a1->height <= a2->y + a2->height);
}

static void
btk_toolbar_begin_sliding (BtkToolbar *toolbar)
{
  BtkWidget *widget = BTK_WIDGET (toolbar);
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  GList *list;
  gint cur_x;
  gint cur_y;
  gint border_width;
  gboolean rtl;
  gboolean vertical;
  
  /* Start the sliding. This function copies the allocation of every
   * item into content->start_allocation. For items that haven't
   * been allocated yet, we calculate their position and save that
   * in start_allocatino along with zero width and zero height.
   *
   * FIXME: It would be nice if we could share this code with
   * the equivalent in btk_widget_size_allocate().
   */
  priv->is_sliding = TRUE;
  
  if (!priv->idle_id)
    priv->idle_id = bdk_threads_add_idle (slide_idle_handler, toolbar);
  
  rtl = (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL);
  vertical = (toolbar->orientation == BTK_ORIENTATION_VERTICAL);
  border_width = get_internal_padding (toolbar) + BTK_CONTAINER (toolbar)->border_width;
  
  if (rtl)
    {
      cur_x = widget->allocation.width - border_width - widget->style->xthickness;
      cur_y = widget->allocation.height - border_width - widget->style->ythickness;
    }
  else
    {
      cur_x = border_width + widget->style->xthickness;
      cur_y = border_width + widget->style->ythickness;
    }
  
  cur_x += widget->allocation.x;
  cur_y += widget->allocation.y;
  
  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      BtkAllocation new_start_allocation;
      BtkAllocation item_allocation;
      ItemState state;
      
      state = toolbar_content_get_state (content);
      toolbar_content_get_allocation (content, &item_allocation);
      
      if ((state == NORMAL &&
	   rect_within (&item_allocation, &(widget->allocation))) ||
	  state == OVERFLOWN)
	{
	  new_start_allocation = item_allocation;
	}
      else
	{
	  new_start_allocation.x = cur_x;
	  new_start_allocation.y = cur_y;
	  
	  if (vertical)
	    {
	      new_start_allocation.width = widget->allocation.width -
		2 * border_width - 2 * widget->style->xthickness;
	      new_start_allocation.height = 0;
	    }
	  else
	    {
	      new_start_allocation.width = 0;
	      new_start_allocation.height = widget->allocation.height -
		2 * border_width - 2 * widget->style->ythickness;
	    }
	}
      
      if (vertical)
	cur_y = new_start_allocation.y + new_start_allocation.height;
      else if (rtl)
	cur_x = new_start_allocation.x;
      else
	cur_x = new_start_allocation.x + new_start_allocation.width;
      
      toolbar_content_set_start_allocation (content, &new_start_allocation);
    }

  /* This resize will run before the first idle handler. This
   * will make sure that items get the right goal allocation
   * so that the idle handler will not immediately return
   * FALSE
   */
  btk_widget_queue_resize_no_redraw (BTK_WIDGET (toolbar));
  g_timer_reset (priv->timer);
}

static void
btk_toolbar_stop_sliding (BtkToolbar *toolbar)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  
  if (priv->is_sliding)
    {
      GList *list;
      
      priv->is_sliding = FALSE;
      
      if (priv->idle_id)
	{
	  g_source_remove (priv->idle_id);
	  priv->idle_id = 0;
	}
      
      list = priv->content;
      while (list)
	{
	  ToolbarContent *content = list->data;
	  list = list->next;

	  if (toolbar_content_is_placeholder (content))
	    {
	      toolbar_content_remove (content, toolbar);
	      toolbar_content_free (content);
	    }
	}
      
      btk_widget_queue_resize_no_redraw (BTK_WIDGET (toolbar));
    }
}

static void
remove_item (BtkWidget *menu_item,
	     gpointer   data)
{
  btk_container_remove (BTK_CONTAINER (menu_item->parent), menu_item);
}

static void
menu_deactivated (BtkWidget  *menu,
		  BtkToolbar *toolbar)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (priv->arrow_button), FALSE);
}

static void
menu_detached (BtkWidget  *toolbar,
	       BtkMenu    *menu)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  priv->menu = NULL;
}

static void
rebuild_menu (BtkToolbar *toolbar)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  GList *list, *children;
  
  if (!priv->menu)
    {
      priv->menu = BTK_MENU (btk_menu_new());
      btk_menu_attach_to_widget (priv->menu,
				 BTK_WIDGET (toolbar),
				 menu_detached);

      g_signal_connect (priv->menu, "deactivate",
                        G_CALLBACK (menu_deactivated), toolbar);
    }

  btk_container_foreach (BTK_CONTAINER (priv->menu), remove_item, NULL);
  
  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      
      if (toolbar_content_get_state (content) == OVERFLOWN &&
	  !toolbar_content_is_placeholder (content))
	{
	  BtkWidget *menu_item = toolbar_content_retrieve_menu_item (content);
	  
	  if (menu_item)
	    {
	      g_assert (BTK_IS_MENU_ITEM (menu_item));
	      btk_menu_shell_append (BTK_MENU_SHELL (priv->menu), menu_item);
	    }
	}
    }

  /* Remove leading and trailing separator items */
  children = btk_container_get_children (BTK_CONTAINER (priv->menu));
  
  list = children;
  while (list && BTK_IS_SEPARATOR_MENU_ITEM (list->data))
    {
      BtkWidget *child = list->data;
      
      btk_container_remove (BTK_CONTAINER (priv->menu), child);
      list = list->next;
    }
  g_list_free (children);

  /* Regenerate the list of children so we don't try to remove items twice */
  children = btk_container_get_children (BTK_CONTAINER (priv->menu));

  list = g_list_last (children);
  while (list && BTK_IS_SEPARATOR_MENU_ITEM (list->data))
    {
      BtkWidget *child = list->data;

      btk_container_remove (BTK_CONTAINER (priv->menu), child);
      list = list->prev;
    }
  g_list_free (children);

  priv->need_rebuild = FALSE;
}

static void
btk_toolbar_size_allocate (BtkWidget     *widget,
			   BtkAllocation *allocation)
{
  BtkToolbar *toolbar = BTK_TOOLBAR (widget);
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  BtkAllocation *allocations;
  ItemState *new_states;
  BtkAllocation arrow_allocation;
  gint arrow_size;
  gint size, pos, short_size;
  GList *list;
  gint i;
  gboolean need_arrow;
  gint n_expand_items;
  gint border_width;
  gint available_size;
  gint n_items;
  gint needed_size;
  BtkRequisition arrow_requisition;
  gboolean overflowing;
  gboolean size_changed;
  gdouble elapsed;
  BtkAllocation item_area;
  BtkShadowType shadow_type;
  
  size_changed = FALSE;
  if (widget->allocation.x != allocation->x		||
      widget->allocation.y != allocation->y		||
      widget->allocation.width != allocation->width	||
      widget->allocation.height != allocation->height)
    {
      size_changed = TRUE;
    }
  
  if (size_changed)
    btk_toolbar_stop_sliding (toolbar);
  
  widget->allocation = *allocation;
  
  border_width = BTK_CONTAINER (toolbar)->border_width;
  
  if (btk_widget_get_realized (widget))
    {
      bdk_window_move_resize (priv->event_window,
                              allocation->x + border_width,
                              allocation->y + border_width,
                              allocation->width - border_width * 2,
                              allocation->height - border_width * 2);
    }
  
  border_width += get_internal_padding (toolbar);
  
  btk_widget_get_child_requisition (BTK_WIDGET (priv->arrow_button),
				    &arrow_requisition);
  
  shadow_type = get_shadow_type (toolbar);

  if (toolbar->orientation == BTK_ORIENTATION_HORIZONTAL)
    {
      available_size = size = allocation->width - 2 * border_width;
      short_size = allocation->height - 2 * border_width;
      arrow_size = arrow_requisition.width;
      
      if (shadow_type != BTK_SHADOW_NONE)
	{
	  available_size -= 2 * widget->style->xthickness;
	  short_size -= 2 * widget->style->ythickness;
	}
    }
  else
    {
      available_size = size = allocation->height - 2 * border_width;
      short_size = allocation->width - 2 * border_width;
      arrow_size = arrow_requisition.height;
      
      if (shadow_type != BTK_SHADOW_NONE)
	{
	  available_size -= 2 * widget->style->ythickness;
	  short_size -= 2 * widget->style->xthickness;
	}
    }
  
  n_items = g_list_length (priv->content);
  allocations = g_new0 (BtkAllocation, n_items);
  new_states = g_new0 (ItemState, n_items);
  
  needed_size = 0;
  need_arrow = FALSE;
  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      
      if (toolbar_content_visible (content, toolbar))
	{
	  needed_size += get_item_size (toolbar, content);

	  /* Do we need an arrow?
	   *
	   * Assume we don't, and see if any non-separator item with a
	   * proxy menu item is then going to overflow.
	   */
	  if (needed_size > available_size			&&
	      !need_arrow					&&
	      priv->show_arrow					&&
	      priv->api_mode == NEW_API				&&
	      toolbar_content_has_proxy_menu_item (content)	&&
	      !toolbar_content_is_separator (content))
	    {
	      need_arrow = TRUE;
	    }
	}
    }
  
  if (need_arrow)
    size = available_size - arrow_size;
  else
    size = available_size;
  
  /* calculate widths and states of items */
  overflowing = FALSE;
  for (list = priv->content, i = 0; list != NULL; list = list->next, ++i)
    {
      ToolbarContent *content = list->data;
      gint item_size;
      
      if (!toolbar_content_visible (content, toolbar))
	{
	  new_states[i] = HIDDEN;
	  continue;
	}
      
      item_size = get_item_size (toolbar, content);
      if (item_size <= size && !overflowing)
	{
	  size -= item_size;
	  allocations[i].width = item_size;
	  new_states[i] = NORMAL;
	}
      else
	{
	  overflowing = TRUE;
	  new_states[i] = OVERFLOWN;
	  allocations[i].width = item_size;
	}
    }
  
  /* calculate width of arrow */  
  if (need_arrow)
    {
      arrow_allocation.width = arrow_size;
      arrow_allocation.height = MAX (short_size, 1);
    }
  
  /* expand expandable items */
  
  /* We don't expand when there is an overflow menu, because that leads to
   * weird jumps when items get moved to the overflow menu and the expanding
   * items suddenly get a lot of extra space
   */
  if (!overflowing)
    {
      gint max_child_expand;
      n_expand_items = 0;
      
      for (i = 0, list = priv->content; list != NULL; list = list->next, ++i)
	{
	  ToolbarContent *content = list->data;
	  
	  if (toolbar_content_get_expand (content) && new_states[i] == NORMAL)
	    n_expand_items++;
	}
      
      max_child_expand = get_max_child_expand (toolbar);
      for (list = priv->content, i = 0; list != NULL; list = list->next, ++i)
	{
	  ToolbarContent *content = list->data;
	  
	  if (toolbar_content_get_expand (content) && new_states[i] == NORMAL)
	    {
	      gint extra = size / n_expand_items;
	      if (size % n_expand_items != 0)
		extra++;

              if (extra > max_child_expand)
                extra = max_child_expand;

	      allocations[i].width += extra;
	      size -= extra;
	      n_expand_items--;
	    }
	}
      
      g_assert (n_expand_items == 0);
    }
  
  /* position items */
  pos = border_width;
  for (list = priv->content, i = 0; list != NULL; list = list->next, ++i)
    {
      /* both NORMAL and OVERFLOWN items get a position. This ensures
       * that sliding will work for OVERFLOWN items too
       */
      if (new_states[i] == NORMAL ||
	  new_states[i] == OVERFLOWN)
	{
	  allocations[i].x = pos;
	  allocations[i].y = border_width;
	  allocations[i].height = short_size;
	  
	  pos += allocations[i].width;
	}
    }
  
  /* position arrow */
  if (need_arrow)
    {
      arrow_allocation.x = available_size - border_width - arrow_allocation.width;
      arrow_allocation.y = border_width;
    }
  
  item_area.x = border_width;
  item_area.y = border_width;
  item_area.width = available_size - (need_arrow? arrow_size : 0);
  item_area.height = short_size;

  /* fix up allocations in the vertical or RTL cases */
  if (toolbar->orientation == BTK_ORIENTATION_VERTICAL)
    {
      for (i = 0; i < n_items; ++i)
	fixup_allocation_for_vertical (&(allocations[i]));
      
      if (need_arrow)
	fixup_allocation_for_vertical (&arrow_allocation);

      fixup_allocation_for_vertical (&item_area);
    }
  else if (btk_widget_get_direction (BTK_WIDGET (toolbar)) == BTK_TEXT_DIR_RTL)
    {
      for (i = 0; i < n_items; ++i)
	fixup_allocation_for_rtl (available_size, &(allocations[i]));
      
      if (need_arrow)
	fixup_allocation_for_rtl (available_size, &arrow_allocation);

      fixup_allocation_for_rtl (available_size, &item_area);
    }
  
  /* translate the items by allocation->(x,y) */
  for (i = 0; i < n_items; ++i)
    {
      allocations[i].x += allocation->x;
      allocations[i].y += allocation->y;
      
      if (shadow_type != BTK_SHADOW_NONE)
	{
	  allocations[i].x += widget->style->xthickness;
	  allocations[i].y += widget->style->ythickness;
	}
    }
  
  if (need_arrow)
    {
      arrow_allocation.x += allocation->x;
      arrow_allocation.y += allocation->y;
      
      if (shadow_type != BTK_SHADOW_NONE)
	{
	  arrow_allocation.x += widget->style->xthickness;
	  arrow_allocation.y += widget->style->ythickness;
	}
    }

  item_area.x += allocation->x;
  item_area.y += allocation->y;
  if (shadow_type != BTK_SHADOW_NONE)
    {
      item_area.x += widget->style->xthickness;
      item_area.y += widget->style->ythickness;
    }

  /* did anything change? */
  for (list = priv->content, i = 0; list != NULL; list = list->next, i++)
    {
      ToolbarContent *content = list->data;
      
      if (toolbar_content_get_state (content) == NORMAL &&
	  new_states[i] != NORMAL)
	{
	  /* an item disappeared and we didn't change size, so begin sliding */
	  if (!size_changed && priv->api_mode == NEW_API)
	    btk_toolbar_begin_sliding (toolbar);
	}
    }
  
  /* finally allocate the items */
  if (priv->is_sliding)
    {
      for (list = priv->content, i = 0; list != NULL; list = list->next, i++)
	{
	  ToolbarContent *content = list->data;
	  
	  toolbar_content_set_goal_allocation (content, &(allocations[i]));
	}
    }

  elapsed = g_timer_elapsed (priv->timer, NULL);
  for (list = priv->content, i = 0; list != NULL; list = list->next, ++i)
    {
      ToolbarContent *content = list->data;

      if (new_states[i] == OVERFLOWN ||
	  new_states[i] == NORMAL)
	{
	  BtkAllocation alloc;
	  BtkAllocation start_allocation = { 0, };
	  BtkAllocation goal_allocation;

	  if (priv->is_sliding)
	    {
	      toolbar_content_get_start_allocation (content, &start_allocation);
	      toolbar_content_get_goal_allocation (content, &goal_allocation);
	      
	      compute_intermediate_allocation (toolbar,
					       &start_allocation,
					       &goal_allocation,
					       &alloc);

	      priv->need_sync = TRUE;
	    }
	  else
	    {
	      alloc = allocations[i];
	    }

	  if (alloc.width <= 0 || alloc.height <= 0)
	    {
	      toolbar_content_set_child_visible (content, toolbar, FALSE);
	    }
	  else
	    {
	      if (!rect_within (&alloc, &item_area))
		{
		  toolbar_content_set_child_visible (content, toolbar, FALSE);
		  toolbar_content_size_allocate (content, &alloc);
		}
	      else
		{
		  toolbar_content_set_child_visible (content, toolbar, TRUE);
		  toolbar_content_size_allocate (content, &alloc);
		}
	    }
	}
      else
	{
	  toolbar_content_set_child_visible (content, toolbar, FALSE);
	}
	  
      toolbar_content_set_state (content, new_states[i]);
    }
  
  if (priv->menu && priv->need_rebuild)
    rebuild_menu (toolbar);
  
  if (need_arrow)
    {
      btk_widget_size_allocate (BTK_WIDGET (priv->arrow_button),
				&arrow_allocation);
      btk_widget_show (BTK_WIDGET (priv->arrow_button));
    }
  else
    {
      btk_widget_hide (BTK_WIDGET (priv->arrow_button));

      if (priv->menu && btk_widget_get_visible (BTK_WIDGET (priv->menu)))
	btk_menu_shell_deactivate (BTK_MENU_SHELL (priv->menu));
    }

  g_free (allocations);
  g_free (new_states);
}

static void
btk_toolbar_update_button_relief (BtkToolbar *toolbar)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  BtkReliefStyle relief;

  relief = get_button_relief (toolbar);

  if (relief != btk_button_get_relief (BTK_BUTTON (priv->arrow_button)))
    {
      btk_toolbar_reconfigured (toolbar);
  
      btk_button_set_relief (BTK_BUTTON (priv->arrow_button), relief);
    }
}

static void
btk_toolbar_style_set (BtkWidget *widget,
		       BtkStyle  *prev_style)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (widget);
  
  priv->max_homogeneous_pixels = -1;

  if (btk_widget_get_realized (widget))
    btk_style_set_background (widget->style, widget->window, widget->state);
  
  if (prev_style)
    btk_toolbar_update_button_relief (BTK_TOOLBAR (widget));
}

static GList *
btk_toolbar_list_children_in_focus_order (BtkToolbar       *toolbar,
					  BtkDirectionType  dir)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  GList *result = NULL;
  GList *list;
  gboolean rtl;
  
  /* generate list of children in reverse logical order */
  
  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      BtkWidget *widget;
      
      widget = toolbar_content_get_widget (content);
      
      if (widget)
	result = g_list_prepend (result, widget);
    }
  
  result = g_list_prepend (result, priv->arrow_button);
  
  rtl = (btk_widget_get_direction (BTK_WIDGET (toolbar)) == BTK_TEXT_DIR_RTL);
  
  /* move in logical order when
   *
   *	- dir is TAB_FORWARD
   *
   *	- in RTL mode and moving left or up
   *
   *    - in LTR mode and moving right or down
   */
  if (dir == BTK_DIR_TAB_FORWARD                                        ||
      (rtl  && (dir == BTK_DIR_UP   || dir == BTK_DIR_LEFT))		||
      (!rtl && (dir == BTK_DIR_DOWN || dir == BTK_DIR_RIGHT)))
    {
      result = g_list_reverse (result);
    }
  
  return result;
}

static gboolean
btk_toolbar_focus_home_or_end (BtkToolbar *toolbar,
			       gboolean    focus_home)
{
  GList *children, *list;
  BtkDirectionType dir = focus_home? BTK_DIR_RIGHT : BTK_DIR_LEFT;
  
  children = btk_toolbar_list_children_in_focus_order (toolbar, dir);
  
  if (btk_widget_get_direction (BTK_WIDGET (toolbar)) == BTK_TEXT_DIR_RTL)
    {
      children = g_list_reverse (children);
      
      dir = (dir == BTK_DIR_RIGHT)? BTK_DIR_LEFT : BTK_DIR_RIGHT;
    }
  
  for (list = children; list != NULL; list = list->next)
    {
      BtkWidget *child = list->data;
      
      if (BTK_CONTAINER (toolbar)->focus_child == child)
	break;
      
      if (btk_widget_get_mapped (child) && btk_widget_child_focus (child, dir))
	break;
    }
  
  g_list_free (children);
  
  return TRUE;
}   

/* Keybinding handler. This function is called when the user presses
 * Ctrl TAB or an arrow key.
 */
static void
btk_toolbar_move_focus (BtkWidget        *widget,
			BtkDirectionType  dir)
{
  BtkToolbar *toolbar = BTK_TOOLBAR (widget);
  BtkContainer *container = BTK_CONTAINER (toolbar);
  GList *list;
  gboolean try_focus = FALSE;
  GList *children;

  if (container->focus_child &&
      btk_widget_child_focus (container->focus_child, dir))
    {
      return;
    }
  
  children = btk_toolbar_list_children_in_focus_order (toolbar, dir);
  
  for (list = children; list != NULL; list = list->next)
    {
      BtkWidget *child = list->data;
      
      if (try_focus && btk_widget_get_mapped (child) && btk_widget_child_focus (child, dir))
	break;
      
      if (child == BTK_CONTAINER (toolbar)->focus_child)
	try_focus = TRUE;
    }
  
  g_list_free (children);
}

/* The focus handler for the toolbar. It called when the user presses
 * TAB or otherwise tries to focus the toolbar.
 */
static gboolean
btk_toolbar_focus (BtkWidget        *widget,
		   BtkDirectionType  dir)
{
  BtkToolbar *toolbar = BTK_TOOLBAR (widget);
  GList *children, *list;
  gboolean result = FALSE;

  /* if focus is already somewhere inside the toolbar then return FALSE.
   * The only way focus can stay inside the toolbar is when the user presses
   * arrow keys or Ctrl TAB (both of which are handled by the
   * btk_toolbar_move_focus() keybinding function.
   */
  if (BTK_CONTAINER (widget)->focus_child)
    return FALSE;

  children = btk_toolbar_list_children_in_focus_order (toolbar, dir);

  for (list = children; list != NULL; list = list->next)
    {
      BtkWidget *child = list->data;
      
      if (btk_widget_get_mapped (child) && btk_widget_child_focus (child, dir))
	{
	  result = TRUE;
	  break;
	}
    }

  g_list_free (children);

  return result;
}

static BtkSettings *
toolbar_get_settings (BtkToolbar *toolbar)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  return priv->settings;
}

static void
style_change_notify (BtkToolbar *toolbar)
{
  if (!toolbar->style_set)
    {
      /* pretend it was set, then unset, thus reverting to new default */
      toolbar->style_set = TRUE;
      btk_toolbar_unset_style (toolbar);
    }
}

static void
icon_size_change_notify (BtkToolbar *toolbar)
{
  if (!toolbar->icon_size_set)
    {
      /* pretend it was set, then unset, thus reverting to new default */
      toolbar->icon_size_set = TRUE;
      btk_toolbar_unset_icon_size (toolbar);
    }
}

static void
animation_change_notify (BtkToolbar *toolbar)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  BtkSettings *settings = toolbar_get_settings (toolbar);
  gboolean animation;

  if (settings)
    g_object_get (settings,
                  "btk-enable-animations", &animation,
                  NULL);
  else
    animation = DEFAULT_ANIMATION_STATE;

  priv->animation = animation;
}

static void
settings_change_notify (BtkSettings      *settings,
                        const BParamSpec *pspec,
                        BtkToolbar       *toolbar)
{
  if (! strcmp (pspec->name, "btk-toolbar-style"))
    style_change_notify (toolbar);
  else if (! strcmp (pspec->name, "btk-toolbar-icon-size"))
    icon_size_change_notify (toolbar);
  else if (! strcmp (pspec->name, "btk-enable-animations"))
    animation_change_notify (toolbar);
}

static void
btk_toolbar_screen_changed (BtkWidget *widget,
			    BdkScreen *previous_screen)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (widget);
  BtkToolbar *toolbar = BTK_TOOLBAR (widget);
  BtkSettings *old_settings = toolbar_get_settings (toolbar);
  BtkSettings *settings;
  
  if (btk_widget_has_screen (BTK_WIDGET (toolbar)))
    settings = btk_widget_get_settings (BTK_WIDGET (toolbar));
  else
    settings = NULL;
  
  if (settings == old_settings)
    return;
  
  if (old_settings)
    {
      g_signal_handler_disconnect (old_settings, priv->settings_connection);

      g_object_unref (old_settings);
    }

  if (settings)
    {
      priv->settings_connection =
	g_signal_connect (settings, "notify",
                          G_CALLBACK (settings_change_notify),
                          toolbar);

      priv->settings = g_object_ref (settings);
    }
  else
    priv->settings = NULL;

  style_change_notify (toolbar);
  icon_size_change_notify (toolbar);
  animation_change_notify (toolbar);
}

static int
find_drop_index (BtkToolbar *toolbar,
		 gint        x,
		 gint        y)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  GList *interesting_content;
  GList *list;
  BtkOrientation orientation;
  BtkTextDirection direction;
  gint best_distance = G_MAXINT;
  gint distance;
  gint cursor;
  gint pos;
  ToolbarContent *best_content;
  BtkAllocation allocation;
  
  /* list items we care about wrt. drag and drop */
  interesting_content = NULL;
  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      
      if (toolbar_content_get_state (content) == NORMAL)
	interesting_content = g_list_prepend (interesting_content, content);
    }
  interesting_content = g_list_reverse (interesting_content);
  
  if (!interesting_content)
    return 0;
  
  orientation = toolbar->orientation;
  direction = btk_widget_get_direction (BTK_WIDGET (toolbar));
  
  /* distance to first interesting item */
  best_content = interesting_content->data;
  toolbar_content_get_allocation (best_content, &allocation);
  
  if (orientation == BTK_ORIENTATION_HORIZONTAL)
    {
      cursor = x;
      
      if (direction == BTK_TEXT_DIR_LTR)
	pos = allocation.x;
      else
	pos = allocation.x + allocation.width;
    }
  else
    {
      cursor = y;
      pos = allocation.y;
    }
  
  best_content = NULL;
  best_distance = ABS (pos - cursor);
  
  /* distance to far end of each item */
  for (list = interesting_content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      
      toolbar_content_get_allocation (content, &allocation);
      
      if (orientation == BTK_ORIENTATION_HORIZONTAL)
	{
	  if (direction == BTK_TEXT_DIR_LTR)
	    pos = allocation.x + allocation.width;
	  else
	    pos = allocation.x;
	}
      else
	{
	  pos = allocation.y + allocation.height;
	}
      
      distance = ABS (pos - cursor);
      
      if (distance < best_distance)
	{
	  best_distance = distance;
	  best_content = content;
	}
    }
  
  g_list_free (interesting_content);
  
  if (!best_content)
    return 0;
  else
    return g_list_index (priv->content, best_content) + 1;
}

static void
reset_all_placeholders (BtkToolbar *toolbar)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  GList *list;
  
  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      if (toolbar_content_is_placeholder (content))
	toolbar_content_set_disappearing (content, TRUE);
    }
}

static gint
physical_to_logical (BtkToolbar *toolbar,
		     gint        physical)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  GList *list;
  int logical;
  
  g_assert (physical >= 0);
  
  logical = 0;
  for (list = priv->content; list && physical > 0; list = list->next)
    {
      ToolbarContent *content = list->data;
      
      if (!toolbar_content_is_placeholder (content))
	logical++;
      physical--;
    }
  
  g_assert (physical == 0);
  
  return logical;
}

static gint
logical_to_physical (BtkToolbar *toolbar,
		     gint        logical)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  GList *list;
  gint physical;
  
  g_assert (logical >= 0);
  
  physical = 0;
  for (list = priv->content; list; list = list->next)
    {
      ToolbarContent *content = list->data;
      
      if (!toolbar_content_is_placeholder (content))
	{
	  if (logical == 0)
	    break;
	  logical--;
	}
      
      physical++;
    }
  
  g_assert (logical == 0);
  
  return physical;
}

/**
 * btk_toolbar_set_drop_highlight_item:
 * @toolbar: a #BtkToolbar
 * @tool_item: (allow-none): a #BtkToolItem, or %NULL to turn of highlighting
 * @index_: a position on @toolbar
 *
 * Highlights @toolbar to give an idea of what it would look like
 * if @item was added to @toolbar at the position indicated by @index_.
 * If @item is %NULL, highlighting is turned off. In that case @index_ 
 * is ignored.
 *
 * The @tool_item passed to this function must not be part of any widget
 * hierarchy. When an item is set as drop highlight item it can not
 * added to any widget hierarchy or used as highlight item for another
 * toolbar.
 * 
 * Since: 2.4
 **/
void
btk_toolbar_set_drop_highlight_item (BtkToolbar  *toolbar,
				     BtkToolItem *tool_item,
				     gint         index_)
{
  ToolbarContent *content;
  BtkToolbarPrivate *priv;
  gint n_items;
  BtkRequisition requisition;
  BtkRequisition old_requisition;
  gboolean restart_sliding;
  
  g_return_if_fail (BTK_IS_TOOLBAR (toolbar));
  g_return_if_fail (tool_item == NULL || BTK_IS_TOOL_ITEM (tool_item));
  
  btk_toolbar_check_new_api (toolbar);
  
  priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  
  if (!tool_item)
    {
      if (priv->highlight_tool_item)
	{
	  btk_widget_unparent (BTK_WIDGET (priv->highlight_tool_item));
	  g_object_unref (priv->highlight_tool_item);
	  priv->highlight_tool_item = NULL;
	}
      
      reset_all_placeholders (toolbar);
      btk_toolbar_begin_sliding (toolbar);
      return;
    }
  
  n_items = btk_toolbar_get_n_items (toolbar);
  if (index_ < 0 || index_ > n_items)
    index_ = n_items;
  
  if (tool_item != priv->highlight_tool_item)
    {
      if (priv->highlight_tool_item)
	g_object_unref (priv->highlight_tool_item);
      
      g_object_ref_sink (tool_item);
      
      priv->highlight_tool_item = tool_item;
      
      btk_widget_set_parent (BTK_WIDGET (priv->highlight_tool_item),
			     BTK_WIDGET (toolbar));
    }
  
  index_ = logical_to_physical (toolbar, index_);
  
  content = g_list_nth_data (priv->content, index_);
  
  if (index_ > 0)
    {
      ToolbarContent *prev_content;
      
      prev_content = g_list_nth_data (priv->content, index_ - 1);
      
      if (prev_content && toolbar_content_is_placeholder (prev_content))
	content = prev_content;
    }
  
  if (!content || !toolbar_content_is_placeholder (content))
    {
      BtkWidget *placeholder;
      
      placeholder = BTK_WIDGET (btk_separator_tool_item_new ());

      content = toolbar_content_new_tool_item (toolbar,
					       BTK_TOOL_ITEM (placeholder),
					       TRUE, index_);
      btk_widget_show (placeholder);
    }
  
  g_assert (content);
  g_assert (toolbar_content_is_placeholder (content));
  
  btk_widget_size_request (BTK_WIDGET (priv->highlight_tool_item),
			   &requisition);

  toolbar_content_set_expand (content, btk_tool_item_get_expand (tool_item));
  
  restart_sliding = FALSE;
  toolbar_content_size_request (content, toolbar, &old_requisition);
  if (toolbar->orientation == BTK_ORIENTATION_HORIZONTAL)
    {
      requisition.height = -1;
      if (requisition.width != old_requisition.width)
	restart_sliding = TRUE;
    }
  else
    {
      requisition.width = -1;
      if (requisition.height != old_requisition.height)
	restart_sliding = TRUE;
    }

  if (toolbar_content_disappearing (content))
    restart_sliding = TRUE;
  
  reset_all_placeholders (toolbar);
  toolbar_content_set_disappearing (content, FALSE);
  
  toolbar_content_set_size_request (content,
				    requisition.width, requisition.height);
  
  if (restart_sliding)
    btk_toolbar_begin_sliding (toolbar);
}

static void
btk_toolbar_get_child_property (BtkContainer *container,
				BtkWidget    *child,
				guint         property_id,
				BValue       *value,
				BParamSpec   *pspec)
{
  BtkToolItem *item = BTK_TOOL_ITEM (child);
  
  switch (property_id)
    {
    case CHILD_PROP_HOMOGENEOUS:
      b_value_set_boolean (value, btk_tool_item_get_homogeneous (item));
      break;
      
    case CHILD_PROP_EXPAND:
      b_value_set_boolean (value, btk_tool_item_get_expand (item));
      break;
      
    default:
      BTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
btk_toolbar_set_child_property (BtkContainer *container,
				BtkWidget    *child,
				guint         property_id,
				const BValue *value,
				BParamSpec   *pspec)
{
  switch (property_id)
    {
    case CHILD_PROP_HOMOGENEOUS:
      btk_tool_item_set_homogeneous (BTK_TOOL_ITEM (child), b_value_get_boolean (value));
      break;
      
    case CHILD_PROP_EXPAND:
      btk_tool_item_set_expand (BTK_TOOL_ITEM (child), b_value_get_boolean (value));
      break;
      
    default:
      BTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
btk_toolbar_show_all (BtkWidget *widget)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (widget);
  GList *list;

  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      
      toolbar_content_show_all (content);
    }
  
  btk_widget_show (widget);
}

static void
btk_toolbar_hide_all (BtkWidget *widget)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (widget);
  GList *list;

  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      
      toolbar_content_hide_all (content);
    }

  btk_widget_hide (widget);
}

static void
btk_toolbar_add (BtkContainer *container,
		 BtkWidget    *widget)
{
  BtkToolbar *toolbar = BTK_TOOLBAR (container);

  if (BTK_IS_TOOL_ITEM (widget))
    btk_toolbar_insert (toolbar, BTK_TOOL_ITEM (widget), -1);
  else
    btk_toolbar_append_widget (toolbar, widget, NULL, NULL);
}

static void
btk_toolbar_remove (BtkContainer *container,
		    BtkWidget    *widget)
{
  BtkToolbar *toolbar = BTK_TOOLBAR (container);
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  ToolbarContent *content_to_remove;
  GList *list;

  content_to_remove = NULL;
  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      BtkWidget *child;
      
      child = toolbar_content_get_widget (content);
      if (child && child == widget)
	{
	  content_to_remove = content;
	  break;
	}
    }
  
  g_return_if_fail (content_to_remove != NULL);
  
  toolbar_content_remove (content_to_remove, toolbar);
  toolbar_content_free (content_to_remove);
}

static void
btk_toolbar_forall (BtkContainer *container,
		    gboolean	  include_internals,
		    BtkCallback   callback,
		    gpointer      callback_data)
{
  BtkToolbar *toolbar = BTK_TOOLBAR (container);
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  GList *list;
  
  g_return_if_fail (callback != NULL);
  
  list = priv->content;
  while (list)
    {
      ToolbarContent *content = list->data;
      GList *next = list->next;
      
      if (include_internals || !toolbar_content_is_placeholder (content))
	{
	  BtkWidget *child = toolbar_content_get_widget (content);
	  
	  if (child)
	    callback (child, callback_data);
	}
      
      list = next;
    }
  
  if (include_internals && priv->arrow_button)
    callback (priv->arrow_button, callback_data);
}

static GType
btk_toolbar_child_type (BtkContainer *container)
{
  return BTK_TYPE_TOOL_ITEM;
}

static void
btk_toolbar_reconfigured (BtkToolbar *toolbar)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  GList *list;
  
  list = priv->content;
  while (list)
    {
      ToolbarContent *content = list->data;
      GList *next = list->next;
      
      toolbar_content_toolbar_reconfigured (content, toolbar);
      
      list = next;
    }
}

static void
btk_toolbar_orientation_changed (BtkToolbar    *toolbar,
				 BtkOrientation orientation)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  if (toolbar->orientation != orientation)
    {
      toolbar->orientation = orientation;
      
      if (orientation == BTK_ORIENTATION_HORIZONTAL)
	btk_arrow_set (BTK_ARROW (priv->arrow), BTK_ARROW_DOWN, BTK_SHADOW_NONE);
      else
	btk_arrow_set (BTK_ARROW (priv->arrow), BTK_ARROW_RIGHT, BTK_SHADOW_NONE);
      
      btk_toolbar_reconfigured (toolbar);
      
      btk_widget_queue_resize (BTK_WIDGET (toolbar));
      g_object_notify (B_OBJECT (toolbar), "orientation");
    }
}

static void
btk_toolbar_real_style_changed (BtkToolbar     *toolbar,
				BtkToolbarStyle style)
{
  if (toolbar->style != style)
    {
      toolbar->style = style;
      
      btk_toolbar_reconfigured (toolbar);
      
      btk_widget_queue_resize (BTK_WIDGET (toolbar));
      g_object_notify (B_OBJECT (toolbar), "toolbar-style");
    }
}

static void
menu_position_func (BtkMenu  *menu,
		    gint     *x,
		    gint     *y,
		    gboolean *push_in,
		    gpointer  user_data)
{
  BtkToolbar *toolbar = BTK_TOOLBAR (user_data);
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  BtkRequisition req;
  BtkRequisition menu_req;
  BdkRectangle monitor;
  gint monitor_num;
  BdkScreen *screen;
  
  btk_widget_size_request (priv->arrow_button, &req);
  btk_widget_size_request (BTK_WIDGET (menu), &menu_req);
  
  screen = btk_widget_get_screen (BTK_WIDGET (menu));
  monitor_num = bdk_screen_get_monitor_at_window (screen, priv->arrow_button->window);
  if (monitor_num < 0)
    monitor_num = 0;
  bdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

  bdk_window_get_origin (BTK_BUTTON (priv->arrow_button)->event_window, x, y);
  if (toolbar->orientation == BTK_ORIENTATION_HORIZONTAL)
    {
      if (btk_widget_get_direction (BTK_WIDGET (toolbar)) == BTK_TEXT_DIR_LTR) 
	*x += priv->arrow_button->allocation.width - req.width;
      else 
	*x += req.width - menu_req.width;

      if ((*y + priv->arrow_button->allocation.height + menu_req.height) <= monitor.y + monitor.height)
	*y += priv->arrow_button->allocation.height;
      else if ((*y - menu_req.height) >= monitor.y)
	*y -= menu_req.height;
      else if (monitor.y + monitor.height - (*y + priv->arrow_button->allocation.height) > *y)
	*y += priv->arrow_button->allocation.height;
      else
	*y -= menu_req.height;
    }
  else 
    {
      if (btk_widget_get_direction (BTK_WIDGET (toolbar)) == BTK_TEXT_DIR_LTR) 
	*x += priv->arrow_button->allocation.width;
      else 
	*x -= menu_req.width;

      if (*y + menu_req.height > monitor.y + monitor.height &&
	  *y + priv->arrow_button->allocation.height - monitor.y > monitor.y + monitor.height - *y)
	*y += priv->arrow_button->allocation.height - menu_req.height;
    }

  *push_in = FALSE;
}

static void
show_menu (BtkToolbar     *toolbar,
	   BdkEventButton *event)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);

  rebuild_menu (toolbar);

  btk_widget_show_all (BTK_WIDGET (priv->menu));

  btk_menu_popup (priv->menu, NULL, NULL,
		  menu_position_func, toolbar,
		  event? event->button : 0,
		  event? event->time : btk_get_current_event_time());
}

static void
btk_toolbar_arrow_button_clicked (BtkWidget  *button,
				  BtkToolbar *toolbar)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);  
  
  if (btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (priv->arrow_button)) &&
      (!priv->menu || !btk_widget_get_visible (BTK_WIDGET (priv->menu))))
    {
      /* We only get here when the button is clicked with the keyboard,
       * because mouse button presses result in the menu being shown so
       * that priv->menu would be non-NULL and visible.
       */
      show_menu (toolbar, NULL);
      btk_menu_shell_select_first (BTK_MENU_SHELL (priv->menu), FALSE);
    }
}

static gboolean
btk_toolbar_arrow_button_press (BtkWidget      *button,
				BdkEventButton *event,
				BtkToolbar     *toolbar)
{
  show_menu (toolbar, event);
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (button), TRUE);
  
  return TRUE;
}

static gboolean
btk_toolbar_button_press (BtkWidget      *toolbar,
    			  BdkEventButton *event)
{
  if (_btk_button_event_triggers_context_menu (event))
    {
      gboolean return_value;
      
      g_signal_emit (toolbar, toolbar_signals[POPUP_CONTEXT_MENU], 0,
		     (int)event->x_root, (int)event->y_root, event->button,
		     &return_value);
      
      return return_value;
    }
  
  return FALSE;
}

static gboolean
btk_toolbar_popup_menu (BtkWidget *toolbar)
{
  gboolean return_value;
  /* This function is the handler for the "popup menu" keybinding,
   * ie., it is called when the user presses Shift F10
   */
  g_signal_emit (toolbar, toolbar_signals[POPUP_CONTEXT_MENU], 0,
		 -1, -1, -1, &return_value);
  
  return return_value;
}

/**
 * btk_toolbar_new:
 * 
 * Creates a new toolbar. 
 
 * Return Value: the newly-created toolbar.
 **/
BtkWidget *
btk_toolbar_new (void)
{
  BtkToolbar *toolbar;
  
  toolbar = g_object_new (BTK_TYPE_TOOLBAR, NULL);
  
  return BTK_WIDGET (toolbar);
}

/**
 * btk_toolbar_insert:
 * @toolbar: a #BtkToolbar
 * @item: a #BtkToolItem
 * @pos: the position of the new item
 *
 * Insert a #BtkToolItem into the toolbar at position @pos. If @pos is
 * 0 the item is prepended to the start of the toolbar. If @pos is
 * negative, the item is appended to the end of the toolbar.
 *
 * Since: 2.4
 **/
void
btk_toolbar_insert (BtkToolbar  *toolbar,
		    BtkToolItem *item,
		    gint         pos)
{
  g_return_if_fail (BTK_IS_TOOLBAR (toolbar));
  g_return_if_fail (BTK_IS_TOOL_ITEM (item));
  
  if (!btk_toolbar_check_new_api (toolbar))
    return;
  
  if (pos >= 0)
    pos = logical_to_physical (toolbar, pos);

  toolbar_content_new_tool_item (toolbar, item, FALSE, pos);
}

/**
 * btk_toolbar_get_item_index:
 * @toolbar: a #BtkToolbar
 * @item: a #BtkToolItem that is a child of @toolbar
 * 
 * Returns the position of @item on the toolbar, starting from 0.
 * It is an error if @item is not a child of the toolbar.
 * 
 * Return value: the position of item on the toolbar.
 * 
 * Since: 2.4
 **/
gint
btk_toolbar_get_item_index (BtkToolbar  *toolbar,
			    BtkToolItem *item)
{
  BtkToolbarPrivate *priv;
  GList *list;
  int n;
  
  g_return_val_if_fail (BTK_IS_TOOLBAR (toolbar), -1);
  g_return_val_if_fail (BTK_IS_TOOL_ITEM (item), -1);
  g_return_val_if_fail (BTK_WIDGET (item)->parent == BTK_WIDGET (toolbar), -1);
  
  if (!btk_toolbar_check_new_api (toolbar))
    return -1;
  
  priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  
  n = 0;
  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;
      BtkWidget *widget;
      
      widget = toolbar_content_get_widget (content);
      
      if (item == BTK_TOOL_ITEM (widget))
	break;
      
      ++n;
    }
  
  return physical_to_logical (toolbar, n);
}

/**
 * btk_toolbar_set_orientation:
 * @toolbar: a #BtkToolbar.
 * @orientation: a new #BtkOrientation.
 *
 * Sets whether a toolbar should appear horizontally or vertically.
 *
 * Deprecated: 2.16: Use btk_orientable_set_orientation() instead.
 **/
void
btk_toolbar_set_orientation (BtkToolbar     *toolbar,
			     BtkOrientation  orientation)
{
  g_return_if_fail (BTK_IS_TOOLBAR (toolbar));
  
  g_signal_emit (toolbar, toolbar_signals[ORIENTATION_CHANGED], 0, orientation);
}

/**
 * btk_toolbar_get_orientation:
 * @toolbar: a #BtkToolbar
 *
 * Retrieves the current orientation of the toolbar. See
 * btk_toolbar_set_orientation().
 *
 * Return value: the orientation
 *
 * Deprecated: 2.16: Use btk_orientable_get_orientation() instead.
 **/
BtkOrientation
btk_toolbar_get_orientation (BtkToolbar *toolbar)
{
  g_return_val_if_fail (BTK_IS_TOOLBAR (toolbar), BTK_ORIENTATION_HORIZONTAL);
  
  return toolbar->orientation;
}

/**
 * btk_toolbar_set_style:
 * @toolbar: a #BtkToolbar.
 * @style: the new style for @toolbar.
 * 
 * Alters the view of @toolbar to display either icons only, text only, or both.
 **/
void
btk_toolbar_set_style (BtkToolbar      *toolbar,
		       BtkToolbarStyle  style)
{
  g_return_if_fail (BTK_IS_TOOLBAR (toolbar));
  
  toolbar->style_set = TRUE;  
  g_signal_emit (toolbar, toolbar_signals[STYLE_CHANGED], 0, style);
}

/**
 * btk_toolbar_get_style:
 * @toolbar: a #BtkToolbar
 *
 * Retrieves whether the toolbar has text, icons, or both . See
 * btk_toolbar_set_style().
 
 * Return value: the current style of @toolbar
 **/
BtkToolbarStyle
btk_toolbar_get_style (BtkToolbar *toolbar)
{
  g_return_val_if_fail (BTK_IS_TOOLBAR (toolbar), DEFAULT_TOOLBAR_STYLE);
  
  return toolbar->style;
}

/**
 * btk_toolbar_unset_style:
 * @toolbar: a #BtkToolbar
 * 
 * Unsets a toolbar style set with btk_toolbar_set_style(), so that
 * user preferences will be used to determine the toolbar style.
 **/
void
btk_toolbar_unset_style (BtkToolbar *toolbar)
{
  BtkToolbarStyle style;
  
  g_return_if_fail (BTK_IS_TOOLBAR (toolbar));
  
  if (toolbar->style_set)
    {
      BtkSettings *settings = toolbar_get_settings (toolbar);
      
      if (settings)
	g_object_get (settings,
		      "btk-toolbar-style", &style,
		      NULL);
      else
	style = DEFAULT_TOOLBAR_STYLE;
      
      if (style != toolbar->style)
	g_signal_emit (toolbar, toolbar_signals[STYLE_CHANGED], 0, style);
      
      toolbar->style_set = FALSE;
    }
}

/**
 * btk_toolbar_set_tooltips:
 * @toolbar: a #BtkToolbar.
 * @enable: set to %FALSE to disable the tooltips, or %TRUE to enable them.
 * 
 * Sets if the tooltips of a toolbar should be active or not.
 *
 * Deprecated: 2.14: The toolkit-wide #BtkSettings:btk-enable-tooltips property
 * is now used instead.
 **/
void
btk_toolbar_set_tooltips (BtkToolbar *toolbar,
			  gboolean    enable)
{
  g_return_if_fail (BTK_IS_TOOLBAR (toolbar));
  
  if (enable)
    btk_tooltips_enable (toolbar->tooltips);
  else
    btk_tooltips_disable (toolbar->tooltips);

  g_object_notify (B_OBJECT (toolbar), "tooltips");
}

/**
 * btk_toolbar_get_tooltips:
 * @toolbar: a #BtkToolbar
 *
 * Retrieves whether tooltips are enabled. See
 * btk_toolbar_set_tooltips().
 *
 * Return value: %TRUE if tooltips are enabled
 *
 * Deprecated: 2.14: The toolkit-wide #BtkSettings:btk-enable-tooltips property
 * is now used instead.
 **/
gboolean
btk_toolbar_get_tooltips (BtkToolbar *toolbar)
{
  g_return_val_if_fail (BTK_IS_TOOLBAR (toolbar), FALSE);
  
  return TRUE;
}

/**
 * btk_toolbar_get_n_items:
 * @toolbar: a #BtkToolbar
 * 
 * Returns the number of items on the toolbar.
 * 
 * Return value: the number of items on the toolbar
 * 
 * Since: 2.4
 **/
gint
btk_toolbar_get_n_items (BtkToolbar *toolbar)
{
  BtkToolbarPrivate *priv;
  
  g_return_val_if_fail (BTK_IS_TOOLBAR (toolbar), -1);
  
  if (!btk_toolbar_check_new_api (toolbar))
    return -1;
  
  priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  
  return physical_to_logical (toolbar, g_list_length (priv->content));
}

/**
 * btk_toolbar_get_nth_item:
 * @toolbar: a #BtkToolbar
 * @n: A position on the toolbar
 *
 * Returns the @n<!-- -->'th item on @toolbar, or %NULL if the
 * toolbar does not contain an @n<!-- -->'th item.
 *
 * Return value: (transfer none): The @n<!-- -->'th #BtkToolItem on @toolbar,
 *     or %NULL if there isn't an @n<!-- -->'th item.
 *
 * Since: 2.4
 **/
BtkToolItem *
btk_toolbar_get_nth_item (BtkToolbar *toolbar,
			  gint        n)
{
  BtkToolbarPrivate *priv;
  ToolbarContent *content;
  gint n_items;
  
  g_return_val_if_fail (BTK_IS_TOOLBAR (toolbar), NULL);
  
  if (!btk_toolbar_check_new_api (toolbar))
    return NULL;
  
  n_items = btk_toolbar_get_n_items (toolbar);
  
  if (n < 0 || n >= n_items)
    return NULL;
  
  priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  
  content = g_list_nth_data (priv->content, logical_to_physical (toolbar, n));
  
  g_assert (content);
  g_assert (!toolbar_content_is_placeholder (content));
  
  return BTK_TOOL_ITEM (toolbar_content_get_widget (content));
}

/**
 * btk_toolbar_get_icon_size:
 * @toolbar: a #BtkToolbar
 *
 * Retrieves the icon size for the toolbar. See btk_toolbar_set_icon_size().
 *
 * Return value: (type int): the current icon size for the icons on
 * the toolbar.
 **/
BtkIconSize
btk_toolbar_get_icon_size (BtkToolbar *toolbar)
{
  g_return_val_if_fail (BTK_IS_TOOLBAR (toolbar), DEFAULT_ICON_SIZE);
  
  return toolbar->icon_size;
}

/**
 * btk_toolbar_get_relief_style:
 * @toolbar: a #BtkToolbar
 * 
 * Returns the relief style of buttons on @toolbar. See
 * btk_button_set_relief().
 * 
 * Return value: The relief style of buttons on @toolbar.
 * 
 * Since: 2.4
 **/
BtkReliefStyle
btk_toolbar_get_relief_style (BtkToolbar *toolbar)
{
  g_return_val_if_fail (BTK_IS_TOOLBAR (toolbar), BTK_RELIEF_NONE);
  
  return get_button_relief (toolbar);
}

/**
 * btk_toolbar_set_show_arrow:
 * @toolbar: a #BtkToolbar
 * @show_arrow: Whether to show an overflow menu
 * 
 * Sets whether to show an overflow menu when @toolbar isn’t allocated enough
 * size to show all of its items. If %TRUE, items which can’t fit in @toolbar,
 * and which have a proxy menu item set by btk_tool_item_set_proxy_menu_item()
 * or #BtkToolItem::create-menu-proxy, will be available in an overflow menu,
 * which can be opened by an added arrow button. If %FALSE, @toolbar will
 * request enough size to fit all of its child items without any overflow.
 * 
 * Since: 2.4
 **/
void
btk_toolbar_set_show_arrow (BtkToolbar *toolbar,
			    gboolean    show_arrow)
{
  BtkToolbarPrivate *priv;
  
  g_return_if_fail (BTK_IS_TOOLBAR (toolbar));
  
  priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  show_arrow = show_arrow != FALSE;
  
  if (priv->show_arrow != show_arrow)
    {
      priv->show_arrow = show_arrow;
      
      if (!priv->show_arrow)
	btk_widget_hide (priv->arrow_button);
      
      btk_widget_queue_resize (BTK_WIDGET (toolbar));      
      g_object_notify (B_OBJECT (toolbar), "show-arrow");
    }
}

/**
 * btk_toolbar_get_show_arrow:
 * @toolbar: a #BtkToolbar
 * 
 * Returns whether the toolbar has an overflow menu.
 * See btk_toolbar_set_show_arrow().
 * 
 * Return value: %TRUE if the toolbar has an overflow menu.
 * 
 * Since: 2.4
 **/
gboolean
btk_toolbar_get_show_arrow (BtkToolbar *toolbar)
{
  BtkToolbarPrivate *priv;
  
  g_return_val_if_fail (BTK_IS_TOOLBAR (toolbar), FALSE);
  
  if (!btk_toolbar_check_new_api (toolbar))
    return FALSE;
  
  priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  
  return priv->show_arrow;
}

/**
 * btk_toolbar_get_drop_index:
 * @toolbar: a #BtkToolbar
 * @x: x coordinate of a point on the toolbar
 * @y: y coordinate of a point on the toolbar
 *
 * Returns the position corresponding to the indicated point on
 * @toolbar. This is useful when dragging items to the toolbar:
 * this function returns the position a new item should be
 * inserted.
 *
 * @x and @y are in @toolbar coordinates.
 * 
 * Return value: The position corresponding to the point (@x, @y) on the toolbar.
 * 
 * Since: 2.4
 **/
gint
btk_toolbar_get_drop_index (BtkToolbar *toolbar,
			    gint        x,
			    gint        y)
{
  g_return_val_if_fail (BTK_IS_TOOLBAR (toolbar), -1);
  
  if (!btk_toolbar_check_new_api (toolbar))
    return -1;
  
  return physical_to_logical (toolbar, find_drop_index (toolbar, x, y));
}

static void
btk_toolbar_dispose (BObject *object)
{
  BtkToolbar *toolbar = BTK_TOOLBAR (object);
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);

  if (priv->arrow_button)
    {
      btk_widget_unparent (priv->arrow_button);
      priv->arrow_button = NULL;
    }

  if (priv->menu)
    btk_widget_destroy (BTK_WIDGET (priv->menu));

  B_OBJECT_CLASS (btk_toolbar_parent_class)->dispose (object);
}

static void
btk_toolbar_finalize (BObject *object)
{
  GList *list;
  BtkToolbar *toolbar = BTK_TOOLBAR (object);
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  
  if (toolbar->tooltips)
    g_object_unref (toolbar->tooltips);

  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;

      toolbar_content_free (content);
    }
  
  g_list_free (priv->content);
  g_list_free (toolbar->children);
  
  g_timer_destroy (priv->timer);

  if (priv->idle_id)
    g_source_remove (priv->idle_id);

  B_OBJECT_CLASS (btk_toolbar_parent_class)->finalize (object);
}

/**
 * btk_toolbar_set_icon_size:
 * @toolbar: A #BtkToolbar
 * @icon_size: (type int): The #BtkIconSize that stock icons in the
 *     toolbar shall have.
 *
 * This function sets the size of stock icons in the toolbar. You
 * can call it both before you add the icons and after they've been
 * added. The size you set will override user preferences for the default
 * icon size.
 * 
 * This should only be used for special-purpose toolbars, normal
 * application toolbars should respect the user preferences for the
 * size of icons.
 **/
void
btk_toolbar_set_icon_size (BtkToolbar  *toolbar,
			   BtkIconSize  icon_size)
{
  g_return_if_fail (BTK_IS_TOOLBAR (toolbar));
  g_return_if_fail (icon_size != BTK_ICON_SIZE_INVALID);
  
  if (!toolbar->icon_size_set)
    {
      toolbar->icon_size_set = TRUE;  
      g_object_notify (B_OBJECT (toolbar), "icon-size-set");
    }

  if (toolbar->icon_size == icon_size)
    return;
  
  toolbar->icon_size = icon_size;
  g_object_notify (B_OBJECT (toolbar), "icon-size");
  
  btk_toolbar_reconfigured (toolbar);
  
  btk_widget_queue_resize (BTK_WIDGET (toolbar));
}

/**
 * btk_toolbar_unset_icon_size:
 * @toolbar: a #BtkToolbar
 * 
 * Unsets toolbar icon size set with btk_toolbar_set_icon_size(), so that
 * user preferences will be used to determine the icon size.
 **/
void
btk_toolbar_unset_icon_size (BtkToolbar *toolbar)
{
  BtkIconSize size;
  
  g_return_if_fail (BTK_IS_TOOLBAR (toolbar));
  
  if (toolbar->icon_size_set)
    {
      BtkSettings *settings = toolbar_get_settings (toolbar);
      
      if (settings)
	{
	  g_object_get (settings,
			"btk-toolbar-icon-size", &size,
			NULL);
	}
      else
	size = DEFAULT_ICON_SIZE;
      
      if (size != toolbar->icon_size)
	{
	  btk_toolbar_set_icon_size (toolbar, size);
	  g_object_notify (B_OBJECT (toolbar), "icon-size");	  
	}
      
      toolbar->icon_size_set = FALSE;
      g_object_notify (B_OBJECT (toolbar), "icon-size-set");      
    }
}

/*
 * Deprecated API
 */

/**
 * btk_toolbar_append_item:
 * @toolbar: a #BtkToolbar.
 * @text: give your toolbar button a label.
 * @tooltip_text: a string that appears when the user holds the mouse over this item.
 * @tooltip_private_text: use with #BtkTipsQuery.
 * @icon: a #BtkWidget that should be used as the button's icon.
 * @callback: the function to be executed when the button is pressed.
 * @user_data: a pointer to any data you wish to be passed to the callback.
 *
 * Inserts a new item into the toolbar. You must specify the position
 * in the toolbar where it will be inserted.
 *
 * @callback must be a pointer to a function taking a #BtkWidget and a gpointer as
 * arguments. Use G_CALLBACK() to cast the function to #GCallback.
 *
 * Return value: the new toolbar item as a #BtkWidget.
 *
 * Deprecated: 2.4: Use btk_toolbar_insert() instead.
 **/
BtkWidget *
btk_toolbar_append_item (BtkToolbar    *toolbar,
			 const char    *text,
			 const char    *tooltip_text,
			 const char    *tooltip_private_text,
			 BtkWidget     *icon,
			 GCallback      callback,
			 gpointer       user_data)
{
  return btk_toolbar_insert_element (toolbar, BTK_TOOLBAR_CHILD_BUTTON,
				     NULL, text,
				     tooltip_text, tooltip_private_text,
				     icon, callback, user_data,
				     toolbar->num_children);
}

/**
 * btk_toolbar_prepend_item:
 * @toolbar: a #BtkToolbar.
 * @text: give your toolbar button a label.
 * @tooltip_text: a string that appears when the user holds the mouse over this item.
 * @tooltip_private_text: use with #BtkTipsQuery.
 * @icon: a #BtkWidget that should be used as the button's icon.
 * @callback: the function to be executed when the button is pressed.
 * @user_data: a pointer to any data you wish to be passed to the callback.
 *
 * Adds a new button to the beginning (top or left edges) of the given toolbar.
 *
 * @callback must be a pointer to a function taking a #BtkWidget and a gpointer as
 * arguments. Use G_CALLBACK() to cast the function to #GCallback.
 *
 * Return value: the new toolbar item as a #BtkWidget.
 *
 * Deprecated: 2.4: Use btk_toolbar_insert() instead.
 **/
BtkWidget *
btk_toolbar_prepend_item (BtkToolbar    *toolbar,
			  const char    *text,
			  const char    *tooltip_text,
			  const char    *tooltip_private_text,
			  BtkWidget     *icon,
			  GCallback      callback,
			  gpointer       user_data)
{
  return btk_toolbar_insert_element (toolbar, BTK_TOOLBAR_CHILD_BUTTON,
				     NULL, text,
				     tooltip_text, tooltip_private_text,
				     icon, callback, user_data,
				     0);
}

/**
 * btk_toolbar_insert_item:
 * @toolbar: a #BtkToolbar.
 * @text: give your toolbar button a label.
 * @tooltip_text: a string that appears when the user holds the mouse over this item.
 * @tooltip_private_text: use with #BtkTipsQuery.
 * @icon: a #BtkWidget that should be used as the button's icon.
 * @callback: the function to be executed when the button is pressed.
 * @user_data: a pointer to any data you wish to be passed to the callback.
 * @position: the number of widgets to insert this item after.
 *
 * Inserts a new item into the toolbar. You must specify the position in the
 * toolbar where it will be inserted.
 *
 * @callback must be a pointer to a function taking a #BtkWidget and a gpointer as
 * arguments. Use G_CALLBACK() to cast the function to #GCallback.
 *
 * Return value: the new toolbar item as a #BtkWidget.
 *
 * Deprecated: 2.4: Use btk_toolbar_insert() instead.
 **/
BtkWidget *
btk_toolbar_insert_item (BtkToolbar    *toolbar,
			 const char    *text,
			 const char    *tooltip_text,
			 const char    *tooltip_private_text,
			 BtkWidget     *icon,
			 GCallback      callback,
			 gpointer       user_data,
			 gint           position)
{
  return btk_toolbar_insert_element (toolbar, BTK_TOOLBAR_CHILD_BUTTON,
				     NULL, text,
				     tooltip_text, tooltip_private_text,
				     icon, callback, user_data,
				     position);
}

/**
 * btk_toolbar_insert_stock:
 * @toolbar: A #BtkToolbar
 * @stock_id: The id of the stock item you want to insert
 * @tooltip_text: The text in the tooltip of the toolbar button
 * @tooltip_private_text: The private text of the tooltip
 * @callback: The callback called when the toolbar button is clicked.
 * @user_data: user data passed to callback
 * @position: The position the button shall be inserted at.
 *            -1 means at the end.
 *
 * Inserts a stock item at the specified position of the toolbar.  If
 * @stock_id is not a known stock item ID, it's inserted verbatim,
 * except that underscores used to mark mnemonics are removed.
 *
 * @callback must be a pointer to a function taking a #BtkWidget and a gpointer as
 * arguments. Use G_CALLBACK() to cast the function to #GCallback.
 *
 * Returns: the inserted widget
 *
 * Deprecated: 2.4: Use btk_toolbar_insert() instead.
 */
BtkWidget*
btk_toolbar_insert_stock (BtkToolbar      *toolbar,
			  const gchar     *stock_id,
			  const char      *tooltip_text,
			  const char      *tooltip_private_text,
			  GCallback        callback,
			  gpointer         user_data,
			  gint             position)
{
  return internal_insert_element (toolbar, BTK_TOOLBAR_CHILD_BUTTON,
				  NULL, stock_id,
				  tooltip_text, tooltip_private_text,
				  NULL, callback, user_data,
				  position, TRUE);
}

/**
 * btk_toolbar_append_space:
 * @toolbar: a #BtkToolbar.
 *
 * Adds a new space to the end of the toolbar.
 *
 * Deprecated: 2.4: Use btk_toolbar_insert() instead.
 **/
void
btk_toolbar_append_space (BtkToolbar *toolbar)
{
  btk_toolbar_insert_element (toolbar, BTK_TOOLBAR_CHILD_SPACE,
			      NULL, NULL,
			      NULL, NULL,
			      NULL, NULL, NULL,
			      toolbar->num_children);
}

/**
 * btk_toolbar_prepend_space:
 * @toolbar: a #BtkToolbar.
 *
 * Adds a new space to the beginning of the toolbar.
 *
 * Deprecated: 2.4: Use btk_toolbar_insert() instead.
 **/
void
btk_toolbar_prepend_space (BtkToolbar *toolbar)
{
  btk_toolbar_insert_element (toolbar, BTK_TOOLBAR_CHILD_SPACE,
			      NULL, NULL,
			      NULL, NULL,
			      NULL, NULL, NULL,
			      0);
}

/**
 * btk_toolbar_insert_space:
 * @toolbar: a #BtkToolbar
 * @position: the number of widgets after which a space should be inserted.
 *
 * Inserts a new space in the toolbar at the specified position.
 *
 * Deprecated: 2.4: Use btk_toolbar_insert() instead.
 **/
void
btk_toolbar_insert_space (BtkToolbar *toolbar,
			  gint        position)
{
  btk_toolbar_insert_element (toolbar, BTK_TOOLBAR_CHILD_SPACE,
			      NULL, NULL,
			      NULL, NULL,
			      NULL, NULL, NULL,
			      position);
}

/**
 * btk_toolbar_remove_space:
 * @toolbar: a #BtkToolbar.
 * @position: the index of the space to remove.
 *
 * Removes a space from the specified position.
 *
 * Deprecated: 2.4: Use btk_toolbar_insert() instead.
 **/
void
btk_toolbar_remove_space (BtkToolbar *toolbar,
			  gint        position)
{
  BtkToolbarPrivate *priv;
  ToolbarContent *content;
  
  g_return_if_fail (BTK_IS_TOOLBAR (toolbar));
  
  if (!btk_toolbar_check_old_api (toolbar))
    return;
  
  priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  
  content = g_list_nth_data (priv->content, position);
  
  if (!content)
    {
      g_warning ("Toolbar position %d doesn't exist", position);
      return;
    }
  
  if (!toolbar_content_is_separator (content))
    {
      g_warning ("Toolbar position %d is not a space", position);
      return;
    }
  
  toolbar_content_remove (content, toolbar);
  toolbar_content_free (content);
}

/**
 * btk_toolbar_append_widget:
 * @toolbar: a #BtkToolbar.
 * @widget: a #BtkWidget to add to the toolbar.
 * @tooltip_text: (allow-none): the element's tooltip.
 * @tooltip_private_text: (allow-none): used for context-sensitive help about this toolbar element.
 *
 * Adds a widget to the end of the given toolbar.
 *
 * Deprecated: 2.4: Use btk_toolbar_insert() instead.
 **/
void
btk_toolbar_append_widget (BtkToolbar  *toolbar,
			   BtkWidget   *widget,
			   const gchar *tooltip_text,
			   const gchar *tooltip_private_text)
{
  btk_toolbar_insert_element (toolbar, BTK_TOOLBAR_CHILD_WIDGET,
			      widget, NULL,
			      tooltip_text, tooltip_private_text,
			      NULL, NULL, NULL,
			      toolbar->num_children);
}

/**
 * btk_toolbar_prepend_widget:
 * @toolbar: a #BtkToolbar.
 * @widget: a #BtkWidget to add to the toolbar.
 * @tooltip_text: (allow-none): the element's tooltip.
 * @tooltip_private_text: (allow-none): used for context-sensitive help about this toolbar element.
 *
 * Adds a widget to the beginning of the given toolbar.
 *
 * Deprecated: 2.4: Use btk_toolbar_insert() instead.
 **/
void
btk_toolbar_prepend_widget (BtkToolbar  *toolbar,
			    BtkWidget   *widget,
			    const gchar *tooltip_text,
			    const gchar *tooltip_private_text)
{
  btk_toolbar_insert_element (toolbar, BTK_TOOLBAR_CHILD_WIDGET,
			      widget, NULL,
			      tooltip_text, tooltip_private_text,
			      NULL, NULL, NULL,
			      0);
}

/**
 * btk_toolbar_insert_widget:
 * @toolbar: a #BtkToolbar.
 * @widget: a #BtkWidget to add to the toolbar.
 * @tooltip_text: (allow-none): the element's tooltip.
 * @tooltip_private_text: (allow-none): used for context-sensitive help about this toolbar element.
 * @position: the number of widgets to insert this widget after.
 *
 * Inserts a widget in the toolbar at the given position.
 *
 * Deprecated: 2.4: Use btk_toolbar_insert() instead.
 **/ 
void
btk_toolbar_insert_widget (BtkToolbar *toolbar,
			   BtkWidget  *widget,
			   const char *tooltip_text,
			   const char *tooltip_private_text,
			   gint        position)
{
  btk_toolbar_insert_element (toolbar, BTK_TOOLBAR_CHILD_WIDGET,
			      widget, NULL,
			      tooltip_text, tooltip_private_text,
			      NULL, NULL, NULL,
			      position);
}

/**
 * btk_toolbar_append_element:
 * @toolbar: a #BtkToolbar.
 * @type: a value of type #BtkToolbarChildType that determines what @widget will be.
 * @widget: (allow-none): a #BtkWidget, or %NULL.
 * @text: the element's label.
 * @tooltip_text: the element's tooltip.
 * @tooltip_private_text: used for context-sensitive help about this toolbar element.
 * @icon: a #BtkWidget that provides pictorial representation of the element's function.
 * @callback: the function to be executed when the button is pressed.
 * @user_data: any data you wish to pass to the callback.
 * 
 * Adds a new element to the end of a toolbar.
 * 
 * If @type == %BTK_TOOLBAR_CHILD_WIDGET, @widget is used as the new element.
 * If @type == %BTK_TOOLBAR_CHILD_RADIOBUTTON, @widget is used to determine
 * the radio group for the new element. In all other cases, @widget must
 * be %NULL.
 * 
 * @callback must be a pointer to a function taking a #BtkWidget and a gpointer as
 * arguments. Use G_CALLBACK() to cast the function to #GCallback.
 *
 * Return value: the new toolbar element as a #BtkWidget.
 *
 * Deprecated: 2.4: Use btk_toolbar_insert() instead.
 **/
BtkWidget*
btk_toolbar_append_element (BtkToolbar          *toolbar,
			    BtkToolbarChildType  type,
			    BtkWidget           *widget,
			    const char          *text,
			    const char          *tooltip_text,
			    const char          *tooltip_private_text,
			    BtkWidget           *icon,
			    GCallback            callback,
			    gpointer             user_data)
{
  return btk_toolbar_insert_element (toolbar, type, widget, text,
				     tooltip_text, tooltip_private_text,
				     icon, callback, user_data,
				     toolbar->num_children);
}

/**
 * btk_toolbar_prepend_element:
 * @toolbar: a #BtkToolbar.
 * @type: a value of type #BtkToolbarChildType that determines what @widget will be.
 * @widget: (allow-none): a #BtkWidget, or %NULL
 * @text: the element's label.
 * @tooltip_text: the element's tooltip.
 * @tooltip_private_text: used for context-sensitive help about this toolbar element.
 * @icon: a #BtkWidget that provides pictorial representation of the element's function.
 * @callback: the function to be executed when the button is pressed.
 * @user_data: any data you wish to pass to the callback.
 *  
 * Adds a new element to the beginning of a toolbar.
 * 
 * If @type == %BTK_TOOLBAR_CHILD_WIDGET, @widget is used as the new element.
 * If @type == %BTK_TOOLBAR_CHILD_RADIOBUTTON, @widget is used to determine
 * the radio group for the new element. In all other cases, @widget must
 * be %NULL.
 * 
 * @callback must be a pointer to a function taking a #BtkWidget and a gpointer as
 * arguments. Use G_CALLBACK() to cast the function to #GCallback.
 *
 * Return value: the new toolbar element as a #BtkWidget.
 *
 * Deprecated: 2.4: Use btk_toolbar_insert() instead.
 **/
BtkWidget *
btk_toolbar_prepend_element (BtkToolbar          *toolbar,
			     BtkToolbarChildType  type,
			     BtkWidget           *widget,
			     const char          *text,
			     const char          *tooltip_text,
			     const char          *tooltip_private_text,
			     BtkWidget           *icon,
			     GCallback            callback,
			     gpointer             user_data)
{
  return btk_toolbar_insert_element (toolbar, type, widget, text,
				     tooltip_text, tooltip_private_text,
				     icon, callback, user_data, 0);
}

/**
 * btk_toolbar_insert_element:
 * @toolbar: a #BtkToolbar.
 * @type: a value of type #BtkToolbarChildType that determines what @widget
 *   will be.
 * @widget: (allow-none): a #BtkWidget, or %NULL. 
 * @text: the element's label.
 * @tooltip_text: the element's tooltip.
 * @tooltip_private_text: used for context-sensitive help about this toolbar element.
 * @icon: a #BtkWidget that provides pictorial representation of the element's function.
 * @callback: the function to be executed when the button is pressed.
 * @user_data: any data you wish to pass to the callback.
 * @position: the number of widgets to insert this element after.
 *
 * Inserts a new element in the toolbar at the given position. 
 *
 * If @type == %BTK_TOOLBAR_CHILD_WIDGET, @widget is used as the new element.
 * If @type == %BTK_TOOLBAR_CHILD_RADIOBUTTON, @widget is used to determine
 * the radio group for the new element. In all other cases, @widget must
 * be %NULL.
 *
 * @callback must be a pointer to a function taking a #BtkWidget and a gpointer as
 * arguments. Use G_CALLBACK() to cast the function to #GCallback.
 *
 * Return value: the new toolbar element as a #BtkWidget.
 *
 * Deprecated: 2.4: Use btk_toolbar_insert() instead.
 **/
BtkWidget *
btk_toolbar_insert_element (BtkToolbar          *toolbar,
			    BtkToolbarChildType  type,
			    BtkWidget           *widget,
			    const char          *text,
			    const char          *tooltip_text,
			    const char          *tooltip_private_text,
			    BtkWidget           *icon,
			    GCallback            callback,
			    gpointer             user_data,
			    gint                 position)
{
  return internal_insert_element (toolbar, type, widget, text,
				  tooltip_text, tooltip_private_text,
				  icon, callback, user_data, position, FALSE);
}

static void
set_child_packing_and_visibility(BtkToolbar      *toolbar,
                                 BtkToolbarChild *child)
{
  BtkWidget *box;
  gboolean   expand;

  box = btk_bin_get_child (BTK_BIN (child->widget));
  
  g_return_if_fail (BTK_IS_BOX (box));
  
  if (child->label)
    {
      expand = (toolbar->style != BTK_TOOLBAR_BOTH);
      
      btk_box_set_child_packing (BTK_BOX (box), child->label,
                                 expand, expand, 0, BTK_PACK_END);
      
      if (toolbar->style != BTK_TOOLBAR_ICONS)
        btk_widget_show (child->label);
      else
        btk_widget_hide (child->label);
    }
  
  if (child->icon)
    {
      expand = (toolbar->style != BTK_TOOLBAR_BOTH_HORIZ);
      
      btk_box_set_child_packing (BTK_BOX (box), child->icon,
                                 expand, expand, 0, BTK_PACK_END);
      
      if (toolbar->style != BTK_TOOLBAR_TEXT)
        btk_widget_show (child->icon);
      else
        btk_widget_hide (child->icon);
    }
}

static BtkWidget *
internal_insert_element (BtkToolbar          *toolbar,
			 BtkToolbarChildType  type,
			 BtkWidget           *widget,
			 const char          *text,
			 const char          *tooltip_text,
			 const char          *tooltip_private_text,
			 BtkWidget           *icon,
			 GCallback            callback,
			 gpointer             user_data,
			 gint                 position,
			 gboolean             use_stock)
{
  BtkWidget *box;
  ToolbarContent *content;
  char *free_me = NULL;

  BtkWidget *child_widget;
  BtkWidget *child_label;
  BtkWidget *child_icon;

  g_return_val_if_fail (BTK_IS_TOOLBAR (toolbar), NULL);
  if (type == BTK_TOOLBAR_CHILD_WIDGET)
    g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);
  else if (type != BTK_TOOLBAR_CHILD_RADIOBUTTON)
    g_return_val_if_fail (widget == NULL, NULL);
  if (BTK_IS_TOOL_ITEM (widget))
    g_warning (MIXED_API_WARNING);
  
  if (!btk_toolbar_check_old_api (toolbar))
    return NULL;
  
  child_widget = NULL;
  child_label = NULL;
  child_icon = NULL;
  
  switch (type)
    {
    case BTK_TOOLBAR_CHILD_SPACE:
      break;
      
    case BTK_TOOLBAR_CHILD_WIDGET:
      child_widget = widget;
      break;
      
    case BTK_TOOLBAR_CHILD_BUTTON:
    case BTK_TOOLBAR_CHILD_TOGGLEBUTTON:
    case BTK_TOOLBAR_CHILD_RADIOBUTTON:
      if (type == BTK_TOOLBAR_CHILD_BUTTON)
	{
	  child_widget = btk_button_new ();
	}
      else if (type == BTK_TOOLBAR_CHILD_TOGGLEBUTTON)
	{
	  child_widget = btk_toggle_button_new ();
	  btk_toggle_button_set_mode (BTK_TOGGLE_BUTTON (child_widget), FALSE);
	}
      else /* type == BTK_TOOLBAR_CHILD_RADIOBUTTON */
	{
	  GSList *group = NULL;

	  if (widget)
	    group = btk_radio_button_get_group (BTK_RADIO_BUTTON (widget));
	  
	  child_widget = btk_radio_button_new (group);
	  btk_toggle_button_set_mode (BTK_TOGGLE_BUTTON (child_widget), FALSE);
	}

      btk_button_set_relief (BTK_BUTTON (child_widget), get_button_relief (toolbar));
      btk_button_set_focus_on_click (BTK_BUTTON (child_widget), FALSE);
      
      if (callback)
	{
	  g_signal_connect (child_widget, "clicked",
			    callback, user_data);
	}
      
      if (toolbar->style == BTK_TOOLBAR_BOTH_HORIZ)
	box = btk_hbox_new (FALSE, 0);
      else
	box = btk_vbox_new (FALSE, 0);

      btk_container_add (BTK_CONTAINER (child_widget), box);
      btk_widget_show (box);
      
      if (text && use_stock)
	{
	  BtkStockItem stock_item;
	  if (btk_stock_lookup (text, &stock_item))
	    {
	      if (!icon)
		icon = btk_image_new_from_stock (text, toolbar->icon_size);
	  
	      text = free_me = _btk_toolbar_elide_underscores (stock_item.label);
	    }
	}
      
      if (text)
	{
	  child_label = btk_label_new (text);
	  
	  btk_container_add (BTK_CONTAINER (box), child_label);
	}
      
      if (icon)
	{
	  child_icon = BTK_WIDGET (icon);
          btk_container_add (BTK_CONTAINER (box), child_icon);
	}
      
      btk_widget_show (child_widget);
      break;
      
    default:
      g_assert_not_reached ();
      break;
    }
  
  if ((type != BTK_TOOLBAR_CHILD_SPACE) && tooltip_text)
    {
      btk_tooltips_set_tip (toolbar->tooltips, child_widget,
			    tooltip_text, tooltip_private_text);
    }
  
  content = toolbar_content_new_compatibility (toolbar, type, child_widget,
					       child_icon, child_label, position);
  
  g_free (free_me);
  
  return child_widget;
}

/*
 * ToolbarContent methods
 */
typedef enum {
  UNKNOWN,
  YES,
  NO
} TriState;

struct _ToolbarContent
{
  ContentType	type;
  ItemState	state;
  
  union
  {
    struct
    {
      BtkToolItem *	item;
      BtkAllocation	start_allocation;
      BtkAllocation	goal_allocation;
      guint		is_placeholder : 1;
      guint		disappearing : 1;
      guint		has_menu : 2;
    } tool_item;
    
    struct
    {
      BtkToolbarChild	child;
      BtkAllocation	space_allocation;
      guint		space_visible : 1;
    } compatibility;
  } u;
};

static ToolbarContent *
toolbar_content_new_tool_item (BtkToolbar  *toolbar,
			       BtkToolItem *item,
			       gboolean     is_placeholder,
			       gint	    pos)
{
  ToolbarContent *content;
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  
  content = g_slice_new0 (ToolbarContent);
  
  content->type = TOOL_ITEM;
  content->state = NOT_ALLOCATED;
  content->u.tool_item.item = item;
  content->u.tool_item.is_placeholder = is_placeholder;
  
  btk_widget_set_parent (BTK_WIDGET (item), BTK_WIDGET (toolbar));

  priv->content = g_list_insert (priv->content, content, pos);
  
  if (!is_placeholder)
    {
      toolbar->num_children++;

      btk_toolbar_stop_sliding (toolbar);
    }

  btk_widget_queue_resize (BTK_WIDGET (toolbar));
  priv->need_rebuild = TRUE;
  
  return content;
}

static ToolbarContent *
toolbar_content_new_compatibility (BtkToolbar          *toolbar,
				   BtkToolbarChildType  type,
				   BtkWidget		*widget,
				   BtkWidget		*icon,
				   BtkWidget		*label,
				   gint			 pos)
{
  ToolbarContent *content;
  BtkToolbarChild *child;
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  
  content = g_slice_new0 (ToolbarContent);

  child = &(content->u.compatibility.child);
  
  content->type = COMPATIBILITY;
  child->type = type;
  child->widget = widget;
  child->icon = icon;
  child->label = label;
  
  if (type != BTK_TOOLBAR_CHILD_SPACE)
    {
      btk_widget_set_parent (child->widget, BTK_WIDGET (toolbar));
    }
  else
    {
      content->u.compatibility.space_visible = TRUE;
      btk_widget_queue_resize (BTK_WIDGET (toolbar));
    }
 
  if (type == BTK_TOOLBAR_CHILD_BUTTON ||
      type == BTK_TOOLBAR_CHILD_TOGGLEBUTTON ||
      type == BTK_TOOLBAR_CHILD_RADIOBUTTON)
    {
      set_child_packing_and_visibility (toolbar, child);
    }

  priv->content = g_list_insert (priv->content, content, pos);
  toolbar->children = g_list_insert (toolbar->children, child, pos);
  priv->need_rebuild = TRUE;
  
  toolbar->num_children++;
  
  return content;
}

static void
toolbar_content_remove (ToolbarContent *content,
			BtkToolbar     *toolbar)
{
  BtkToolbarChild *child;
  BtkToolbarPrivate *priv;

  priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  
  switch (content->type)
    {
    case TOOL_ITEM:
      btk_widget_unparent (BTK_WIDGET (content->u.tool_item.item));
      break;
      
    case COMPATIBILITY:
      child = &(content->u.compatibility.child);
      
      if (child->type != BTK_TOOLBAR_CHILD_SPACE)
	{
	  g_object_ref (child->widget);
	  btk_widget_unparent (child->widget);
	  btk_widget_destroy (child->widget);
	  g_object_unref (child->widget);
	}
      
      toolbar->children = g_list_remove (toolbar->children, child);
      break;
    }

  priv->content = g_list_remove (priv->content, content);

  if (!toolbar_content_is_placeholder (content))
    toolbar->num_children--;

  btk_widget_queue_resize (BTK_WIDGET (toolbar));
  priv->need_rebuild = TRUE;
}

static void
toolbar_content_free (ToolbarContent *content)
{
  g_slice_free (ToolbarContent, content);
}

static gint
calculate_max_homogeneous_pixels (BtkWidget *widget)
{
  BangoContext *context;
  BangoFontMetrics *metrics;
  gint char_width;
  
  context = btk_widget_get_bango_context (widget);
  metrics = bango_context_get_metrics (context,
				       widget->style->font_desc,
				       bango_context_get_language (context));
  char_width = bango_font_metrics_get_approximate_char_width (metrics);
  bango_font_metrics_unref (metrics);
  
  return BANGO_PIXELS (MAX_HOMOGENEOUS_N_CHARS * char_width);
}

static void
toolbar_content_expose (ToolbarContent *content,
			BtkContainer   *container,
			BdkEventExpose *expose)
{
  BtkToolbar *toolbar = BTK_TOOLBAR (container);
  BtkToolbarChild *child;
  BtkWidget *widget = NULL; /* quiet gcc */
  
  switch (content->type)
    {
    case TOOL_ITEM:
      if (!content->u.tool_item.is_placeholder)
	widget = BTK_WIDGET (content->u.tool_item.item);
      break;
      
    case COMPATIBILITY:
      child = &(content->u.compatibility.child);
      
      if (child->type == BTK_TOOLBAR_CHILD_SPACE)
	{
	  if (content->u.compatibility.space_visible &&
              get_space_style (toolbar) == BTK_TOOLBAR_SPACE_LINE)
	     _btk_toolbar_paint_space_line (BTK_WIDGET (toolbar), toolbar,
					    &expose->area,
					    &content->u.compatibility.space_allocation);
	  return;
	}
      
      widget = child->widget;
      break;
    }
  
  if (widget)
    btk_container_propagate_expose (container, widget, expose);
}

static gboolean
toolbar_content_visible (ToolbarContent *content,
			 BtkToolbar     *toolbar)
{
  BtkToolItem *item;
  
  switch (content->type)
    {
    case TOOL_ITEM:
      item = content->u.tool_item.item;
      
      if (!btk_widget_get_visible (BTK_WIDGET (item)))
	return FALSE;
      
      if (toolbar->orientation == BTK_ORIENTATION_HORIZONTAL &&
	  btk_tool_item_get_visible_horizontal (item))
	return TRUE;
      
      if ((toolbar->orientation == BTK_ORIENTATION_VERTICAL &&
	   btk_tool_item_get_visible_vertical (item)))
	return TRUE;
      
      return FALSE;
      break;
      
    case COMPATIBILITY:
      if (content->u.compatibility.child.type != BTK_TOOLBAR_CHILD_SPACE)
	return btk_widget_get_visible (content->u.compatibility.child.widget);
      else
	return TRUE;
      break;
    }
  
  g_assert_not_reached ();
  return FALSE;
}

static void
toolbar_content_size_request (ToolbarContent *content,
			      BtkToolbar     *toolbar,
			      BtkRequisition *requisition)
{
  gint space_size;
  
  switch (content->type)
    {
    case TOOL_ITEM:
      btk_widget_size_request (BTK_WIDGET (content->u.tool_item.item),
			       requisition);
      if (content->u.tool_item.is_placeholder &&
	  content->u.tool_item.disappearing)
	{
	  requisition->width = 0;
	  requisition->height = 0;
	}
      break;
      
    case COMPATIBILITY:
      space_size = get_space_size (toolbar);
      
      if (content->u.compatibility.child.type != BTK_TOOLBAR_CHILD_SPACE)
	{
	  btk_widget_size_request (content->u.compatibility.child.widget,
				   requisition);
	}
      else
	{
	  if (toolbar->orientation == BTK_ORIENTATION_HORIZONTAL)
	    {
	      requisition->width = space_size;
	      requisition->height = 0;
	    }
	  else
	    {
	      requisition->height = space_size;
	      requisition->width = 0;
	    }
	}
      
      break;
    }
}

static gboolean
toolbar_content_is_homogeneous (ToolbarContent *content,
				BtkToolbar     *toolbar)
{
  gboolean result = FALSE;	/* quiet gcc */
  BtkRequisition requisition;
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  
  if (priv->max_homogeneous_pixels < 0)
    {
      priv->max_homogeneous_pixels =
	calculate_max_homogeneous_pixels (BTK_WIDGET (toolbar));
    }
  
  toolbar_content_size_request (content, toolbar, &requisition);
  
  if (requisition.width > priv->max_homogeneous_pixels)
    return FALSE;
  
  switch (content->type)
    {
    case TOOL_ITEM:
      result = btk_tool_item_get_homogeneous (content->u.tool_item.item) &&
	!BTK_IS_SEPARATOR_TOOL_ITEM (content->u.tool_item.item);
      
      if (btk_tool_item_get_is_important (content->u.tool_item.item) &&
	  toolbar->style == BTK_TOOLBAR_BOTH_HORIZ &&
	  toolbar->orientation == BTK_ORIENTATION_HORIZONTAL)
	{
	  result = FALSE;
	}
      break;
      
    case COMPATIBILITY:
      if (content->u.compatibility.child.type == BTK_TOOLBAR_CHILD_BUTTON ||
	  content->u.compatibility.child.type == BTK_TOOLBAR_CHILD_RADIOBUTTON ||
	  content->u.compatibility.child.type == BTK_TOOLBAR_CHILD_TOGGLEBUTTON)
	{
	  result = TRUE;
	}
      else
	{
	  result = FALSE;
	}
      break;
    }
  
  return result;
}

static gboolean
toolbar_content_is_placeholder (ToolbarContent *content)
{
  if (content->type == TOOL_ITEM && content->u.tool_item.is_placeholder)
    return TRUE;
  
  return FALSE;
}

static gboolean
toolbar_content_disappearing (ToolbarContent *content)
{
  if (content->type == TOOL_ITEM && content->u.tool_item.disappearing)
    return TRUE;
  
  return FALSE;
}

static ItemState
toolbar_content_get_state (ToolbarContent *content)
{
  return content->state;
}

static gboolean
toolbar_content_child_visible (ToolbarContent *content)
{
  switch (content->type)
    {
    case TOOL_ITEM:
      return BTK_WIDGET_CHILD_VISIBLE (content->u.tool_item.item);
      break;
      
    case COMPATIBILITY:
      if (content->u.compatibility.child.type != BTK_TOOLBAR_CHILD_SPACE)
	{
	  return BTK_WIDGET_CHILD_VISIBLE (content->u.compatibility.child.widget);
	}
      else
	{
	  return content->u.compatibility.space_visible;
	}
      break;
    }
  
  return FALSE; /* quiet gcc */
}

static void
toolbar_content_get_goal_allocation (ToolbarContent *content,
				     BtkAllocation  *allocation)
{
  switch (content->type)
    {
    case TOOL_ITEM:
      *allocation = content->u.tool_item.goal_allocation;
      break;
      
    case COMPATIBILITY:
      /* Goal allocations are only relevant when we are
       * using the new API, so we should never get here
       */
      g_assert_not_reached ();
      break;

    default:
      g_assert_not_reached ();
      break;
    }
}

static void
toolbar_content_get_allocation (ToolbarContent *content,
				BtkAllocation  *allocation)
{
  BtkToolbarChild *child;

  switch (content->type)
    {
    case TOOL_ITEM:
      *allocation = BTK_WIDGET (content->u.tool_item.item)->allocation;
      break;
      
    case COMPATIBILITY:
      child = &(content->u.compatibility.child);
      
      if (child->type == BTK_TOOLBAR_CHILD_SPACE)
	*allocation = content->u.compatibility.space_allocation;
      else
	*allocation = child->widget->allocation;
      break;

    default:
      g_assert_not_reached ();
      break;
    }
}

static void
toolbar_content_set_start_allocation (ToolbarContent *content,
				      BtkAllocation  *allocation)
{
  switch (content->type)
    {
    case TOOL_ITEM:
      content->u.tool_item.start_allocation = *allocation;
      break;
      
    case COMPATIBILITY:
      /* start_allocation is only relevant when using the new API */
      g_assert_not_reached ();
      break;
    }
}

static gboolean
toolbar_content_get_expand (ToolbarContent *content)
{
  if (content->type == TOOL_ITEM &&
      btk_tool_item_get_expand (content->u.tool_item.item) &&
      !content->u.tool_item.disappearing)
    {
      return TRUE;
    }
  
  return FALSE;
}

static void
toolbar_content_set_goal_allocation (ToolbarContent *content,
				     BtkAllocation  *allocation)
{
  switch (content->type)
    {
    case TOOL_ITEM:
      content->u.tool_item.goal_allocation = *allocation;
      break;
      
    case COMPATIBILITY:
      /* Only relevant when using new API */
      g_assert_not_reached ();
      break;
    }
}

static void
toolbar_content_set_child_visible (ToolbarContent *content,
				   BtkToolbar     *toolbar,
				   gboolean        visible)
{
  BtkToolbarChild *child;
  
  switch (content->type)
    {
    case TOOL_ITEM:
      btk_widget_set_child_visible (BTK_WIDGET (content->u.tool_item.item),
				    visible);
      break;
      
    case COMPATIBILITY:
      child = &(content->u.compatibility.child);
      
      if (child->type != BTK_TOOLBAR_CHILD_SPACE)
	{
	  btk_widget_set_child_visible (child->widget, visible);
	}
      else
	{
	  if (content->u.compatibility.space_visible != visible)
	    {
	      content->u.compatibility.space_visible = visible;
	      btk_widget_queue_draw (BTK_WIDGET (toolbar));
	    }
	}
      break;
    }
}

static void
toolbar_content_get_start_allocation (ToolbarContent *content,
				      BtkAllocation  *start_allocation)
{
  switch (content->type)
    {
    case TOOL_ITEM:
      *start_allocation = content->u.tool_item.start_allocation;
      break;
      
    case COMPATIBILITY:
      /* Only relevant for new API */
      g_assert_not_reached ();
      break;
    }
}

static void
toolbar_content_size_allocate (ToolbarContent *content,
			       BtkAllocation  *allocation)
{
  switch (content->type)
    {
    case TOOL_ITEM:
      btk_widget_size_allocate (BTK_WIDGET (content->u.tool_item.item),
				allocation);
      break;
      
    case COMPATIBILITY:
      if (content->u.compatibility.child.type != BTK_TOOLBAR_CHILD_SPACE)
	{
	  btk_widget_size_allocate (content->u.compatibility.child.widget,
				    allocation);
	}
      else
	{
	  content->u.compatibility.space_allocation = *allocation;
	}
      break;
    }
}

static void
toolbar_content_set_state (ToolbarContent *content,
			   ItemState       state)
{
  content->state = state;
}

static BtkWidget *
toolbar_content_get_widget (ToolbarContent *content)
{
  BtkToolbarChild *child;
  
  switch (content->type)
    {
    case TOOL_ITEM:
      return BTK_WIDGET (content->u.tool_item.item);
      break;
      
    case COMPATIBILITY:
      child = &(content->u.compatibility.child);
      if (child->type != BTK_TOOLBAR_CHILD_SPACE)
	return child->widget;
      else
	return NULL;
      break;
    }
  
  return NULL;
}

static void
toolbar_content_set_disappearing (ToolbarContent *content,
				  gboolean        disappearing)
{
  switch (content->type)
    {
    case TOOL_ITEM:
      content->u.tool_item.disappearing = disappearing;
      break;
      
    case COMPATIBILITY:
      /* Only relevant for new API */
      g_assert_not_reached ();
      break;
    }
}

static void
toolbar_content_set_size_request (ToolbarContent *content,
				  gint            width,
				  gint            height)
{
  switch (content->type)
    {
    case TOOL_ITEM:
      btk_widget_set_size_request (BTK_WIDGET (content->u.tool_item.item),
				   width, height);
      break;
      
    case COMPATIBILITY:
      /* Setting size requests only happens with sliding,
       * so not relevant here
       */
      g_assert_not_reached ();
      break;
    }
}

static void
toolbar_child_reconfigure (BtkToolbar      *toolbar,
			   BtkToolbarChild *child)
{
  BtkWidget *box;
  BtkImage *image;
  BtkToolbarStyle style;
  BtkIconSize icon_size;
  BtkReliefStyle relief;
  gchar *stock_id;
  
  style = btk_toolbar_get_style (toolbar);
  icon_size = btk_toolbar_get_icon_size (toolbar);
  relief = btk_toolbar_get_relief_style (toolbar);
  
  /* style */
  if (child->type == BTK_TOOLBAR_CHILD_BUTTON ||
      child->type == BTK_TOOLBAR_CHILD_RADIOBUTTON ||
      child->type == BTK_TOOLBAR_CHILD_TOGGLEBUTTON)
    {
      box = btk_bin_get_child (BTK_BIN (child->widget));
      
      if (style == BTK_TOOLBAR_BOTH && BTK_IS_HBOX (box))
	{
	  BtkWidget *vbox;
	  
	  vbox = btk_vbox_new (FALSE, 0);
	  
	  if (child->label)
	    btk_widget_reparent (child->label, vbox);
	  if (child->icon)
	    btk_widget_reparent (child->icon, vbox);
	  
	  btk_widget_destroy (box);
	  btk_container_add (BTK_CONTAINER (child->widget), vbox);
	  
	  btk_widget_show (vbox);
	}
      else if (style == BTK_TOOLBAR_BOTH_HORIZ && BTK_IS_VBOX (box))
	{
	  BtkWidget *hbox;
	  
	  hbox = btk_hbox_new (FALSE, 0);
	  
	  if (child->label)
	    btk_widget_reparent (child->label, hbox);
	  if (child->icon)
	    btk_widget_reparent (child->icon, hbox);
	  
	  btk_widget_destroy (box);
	  btk_container_add (BTK_CONTAINER (child->widget), hbox);
	  
	  btk_widget_show (hbox);
	}

      set_child_packing_and_visibility (toolbar, child);
    }
  
  /* icon size */
  
  if ((child->type == BTK_TOOLBAR_CHILD_BUTTON ||
       child->type == BTK_TOOLBAR_CHILD_TOGGLEBUTTON ||
       child->type == BTK_TOOLBAR_CHILD_RADIOBUTTON) &&
      BTK_IS_IMAGE (child->icon))
    {
      image = BTK_IMAGE (child->icon);
      if (btk_image_get_storage_type (image) == BTK_IMAGE_STOCK)
	{
	  btk_image_get_stock (image, &stock_id, NULL);
	  stock_id = g_strdup (stock_id);
	  btk_image_set_from_stock (image,
				    stock_id,
				    icon_size);
	  g_free (stock_id);
	}
    }
  
  /* relief */
  if (child->type == BTK_TOOLBAR_CHILD_BUTTON ||
      child->type == BTK_TOOLBAR_CHILD_RADIOBUTTON ||
      child->type == BTK_TOOLBAR_CHILD_TOGGLEBUTTON)
    {
      btk_button_set_relief (BTK_BUTTON (child->widget), relief);
    }
}

static void
toolbar_content_toolbar_reconfigured (ToolbarContent *content,
				      BtkToolbar     *toolbar)
{
  switch (content->type)
    {
    case TOOL_ITEM:
      btk_tool_item_toolbar_reconfigured (content->u.tool_item.item);
      break;
      
    case COMPATIBILITY:
      toolbar_child_reconfigure (toolbar, &(content->u.compatibility.child));
      break;
    }
}

static BtkWidget *
toolbar_content_retrieve_menu_item (ToolbarContent *content)
{
  if (content->type == TOOL_ITEM)
    return btk_tool_item_retrieve_proxy_menu_item (content->u.tool_item.item);
  
  /* FIXME - we might actually be able to do something meaningful here */
  return NULL; 
}

static gboolean
toolbar_content_has_proxy_menu_item (ToolbarContent *content)
{
  if (content->type == TOOL_ITEM)
    {
      BtkWidget *menu_item;

      if (content->u.tool_item.has_menu == YES)
	return TRUE;
      else if (content->u.tool_item.has_menu == NO)
	return FALSE;

      menu_item = toolbar_content_retrieve_menu_item (content);

      content->u.tool_item.has_menu = menu_item? YES : NO;
      
      return menu_item != NULL;
    }
  else
    {
      return FALSE;
    }
}

static void
toolbar_content_set_unknown_menu_status (ToolbarContent *content)
{
  if (content->type == TOOL_ITEM)
    content->u.tool_item.has_menu = UNKNOWN;
}

static gboolean
toolbar_content_is_separator (ToolbarContent *content)
{
  BtkToolbarChild *child;
  
  switch (content->type)
    {
    case TOOL_ITEM:
      return BTK_IS_SEPARATOR_TOOL_ITEM (content->u.tool_item.item);
      break;
      
    case COMPATIBILITY:
      child = &(content->u.compatibility.child);
      return (child->type == BTK_TOOLBAR_CHILD_SPACE);
      break;
    }
  
  return FALSE;
}

static void
toolbar_content_set_expand (ToolbarContent *content,
			    gboolean        expand)
{
  if (content->type == TOOL_ITEM)
    btk_tool_item_set_expand (content->u.tool_item.item, expand);
}

static gboolean
ignore_show_and_hide_all (ToolbarContent *content)
{
  if (content->type == COMPATIBILITY)
    {
      BtkToolbarChildType type = content->u.compatibility.child.type;
      
      if (type == BTK_TOOLBAR_CHILD_BUTTON ||
	  type == BTK_TOOLBAR_CHILD_TOGGLEBUTTON ||
	  type == BTK_TOOLBAR_CHILD_RADIOBUTTON)
	{
	  return TRUE;
	}
    }
  
  return FALSE;
}

static void
toolbar_content_show_all (ToolbarContent  *content)
{
  BtkWidget *widget;
  
  if (ignore_show_and_hide_all (content))
    return;

  widget = toolbar_content_get_widget (content);
  if (widget)
    btk_widget_show_all (widget);
}

static void
toolbar_content_hide_all (ToolbarContent  *content)
{
  BtkWidget *widget;
  
  if (ignore_show_and_hide_all (content))
    return;

  widget = toolbar_content_get_widget (content);
  if (widget)
    btk_widget_hide_all (widget);
}

/*
 * Getters
 */
static gint
get_space_size (BtkToolbar *toolbar)
{
  gint space_size = DEFAULT_SPACE_SIZE;
  
  if (toolbar)
    {
      btk_widget_style_get (BTK_WIDGET (toolbar),
			    "space-size", &space_size,
			    NULL);
    }
  
  return space_size;
}

static BtkToolbarSpaceStyle
get_space_style (BtkToolbar *toolbar)
{
  BtkToolbarSpaceStyle space_style = DEFAULT_SPACE_STYLE;

  if (toolbar)
    {
      btk_widget_style_get (BTK_WIDGET (toolbar),
			    "space-style", &space_style,
			    NULL);
    }
  
  return space_style;  
}

static BtkReliefStyle
get_button_relief (BtkToolbar *toolbar)
{
  BtkReliefStyle button_relief = BTK_RELIEF_NORMAL;
  
  btk_widget_ensure_style (BTK_WIDGET (toolbar));
  
  btk_widget_style_get (BTK_WIDGET (toolbar),
                        "button-relief", &button_relief,
                        NULL);
  
  return button_relief;
}

static gint
get_internal_padding (BtkToolbar *toolbar)
{
  gint ipadding = 0;
  
  btk_widget_style_get (BTK_WIDGET (toolbar),
			"internal-padding", &ipadding,
			NULL);
  
  return ipadding;
}

static gint
get_max_child_expand (BtkToolbar *toolbar)
{
  gint mexpand = G_MAXINT;

  btk_widget_style_get (BTK_WIDGET (toolbar),
                        "max-child-expand", &mexpand,
                        NULL);
  return mexpand;
}

static BtkShadowType
get_shadow_type (BtkToolbar *toolbar)
{
  BtkShadowType shadow_type;
  
  btk_widget_style_get (BTK_WIDGET (toolbar),
			"shadow-type", &shadow_type,
			NULL);
  
  return shadow_type;
}

/*
 * API checks
 */
static gboolean
btk_toolbar_check_old_api (BtkToolbar *toolbar)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  
  if (priv->api_mode == NEW_API)
    {
      g_warning (MIXED_API_WARNING);
      return FALSE;
    }
  
  priv->api_mode = OLD_API;
  return TRUE;
}

static gboolean
btk_toolbar_check_new_api (BtkToolbar *toolbar)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (toolbar);
  
  if (priv->api_mode == OLD_API)
    {
      g_warning (MIXED_API_WARNING);
      return FALSE;
    }
  
  priv->api_mode = NEW_API;
  return TRUE;
}

/* BTK+ internal methods */

gint
_btk_toolbar_get_default_space_size (void)
{
  return DEFAULT_SPACE_SIZE;
}

void
_btk_toolbar_paint_space_line (BtkWidget           *widget,
			       BtkToolbar          *toolbar,
			       const BdkRectangle  *area,
			       const BtkAllocation *allocation)
{
  const double start_fraction = (SPACE_LINE_START / SPACE_LINE_DIVISION);
  const double end_fraction = (SPACE_LINE_END / SPACE_LINE_DIVISION);
  
  BtkOrientation orientation;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  
  orientation = toolbar? toolbar->orientation : BTK_ORIENTATION_HORIZONTAL;

  if (orientation == BTK_ORIENTATION_HORIZONTAL)
    {
      gboolean wide_separators;
      gint     separator_width;

      btk_widget_style_get (widget,
                            "wide-separators", &wide_separators,
                            "separator-width", &separator_width,
                            NULL);

      if (wide_separators)
        btk_paint_box (widget->style, widget->window,
                       btk_widget_get_state (widget), BTK_SHADOW_ETCHED_OUT,
                       area, widget, "vseparator",
                       allocation->x + (allocation->width - separator_width) / 2,
                       allocation->y + allocation->height * start_fraction,
                       separator_width,
                       allocation->height * (end_fraction - start_fraction));
      else
        btk_paint_vline (widget->style, widget->window,
                         btk_widget_get_state (widget), area, widget,
                         "toolbar",
                         allocation->y + allocation->height * start_fraction,
                         allocation->y + allocation->height * end_fraction,
                         allocation->x + (allocation->width - widget->style->xthickness) / 2);
    }
  else
    {
      gboolean wide_separators;
      gint     separator_height;

      btk_widget_style_get (widget,
                            "wide-separators",  &wide_separators,
                            "separator-height", &separator_height,
                            NULL);

      if (wide_separators)
        btk_paint_box (widget->style, widget->window,
                       btk_widget_get_state (widget), BTK_SHADOW_ETCHED_OUT,
                       area, widget, "hseparator",
                       allocation->x + allocation->width * start_fraction,
                       allocation->y + (allocation->height - separator_height) / 2,
                       allocation->width * (end_fraction - start_fraction),
                       separator_height);
      else
        btk_paint_hline (widget->style, widget->window,
                         btk_widget_get_state (widget), area, widget,
                         "toolbar",
                         allocation->x + allocation->width * start_fraction,
                         allocation->x + allocation->width * end_fraction,
                         allocation->y + (allocation->height - widget->style->ythickness) / 2);
    }
}

gchar *
_btk_toolbar_elide_underscores (const gchar *original)
{
  gchar *q, *result;
  const gchar *p, *end;
  gsize len;
  gboolean last_underscore;
  
  if (!original)
    return NULL;

  len = strlen (original);
  q = result = g_malloc (len + 1);
  last_underscore = FALSE;
  
  end = original + len;
  for (p = original; p < end; p++)
    {
      if (!last_underscore && *p == '_')
	last_underscore = TRUE;
      else
	{
	  last_underscore = FALSE;
	  if (original + 2 <= p && p + 1 <= end && 
              p[-2] == '(' && p[-1] == '_' && p[0] != '_' && p[1] == ')')
	    {
	      q--;
	      *q = '\0';
	      p++;
	    }
	  else
	    *q++ = *p;
	}
    }

  if (last_underscore)
    *q++ = '_';
  
  *q = '\0';
  
  return result;
}

static BtkIconSize
toolbar_get_icon_size (BtkToolShell *shell)
{
  return BTK_TOOLBAR (shell)->icon_size;
}

static BtkOrientation
toolbar_get_orientation (BtkToolShell *shell)
{
  return BTK_TOOLBAR (shell)->orientation;
}

static BtkToolbarStyle
toolbar_get_style (BtkToolShell *shell)
{
  return BTK_TOOLBAR (shell)->style;
}

static BtkReliefStyle
toolbar_get_relief_style (BtkToolShell *shell)
{
  return get_button_relief (BTK_TOOLBAR (shell));
}

static void
toolbar_rebuild_menu (BtkToolShell *shell)
{
  BtkToolbarPrivate *priv = BTK_TOOLBAR_GET_PRIVATE (shell);
  GList *list;

  priv->need_rebuild = TRUE;

  for (list = priv->content; list != NULL; list = list->next)
    {
      ToolbarContent *content = list->data;

      toolbar_content_set_unknown_menu_status (content);
    }
  
  btk_widget_queue_resize (BTK_WIDGET (shell));
}

#define __BTK_TOOLBAR_C__
#include "btkaliasdef.c"
