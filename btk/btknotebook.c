/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
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

#include "config.h"

#include <stdio.h>
#include <string.h>

#include <bdk/bdkkeysyms.h>

#undef BTK_DISABLE_DEPRECATED

#include "btknotebook.h"
#include "btkmain.h"
#include "btkmenu.h"
#include "btkmenuitem.h"
#include "btklabel.h"
#include "btkintl.h"
#include "btkmarshalers.h"
#include "btkbindings.h"
#include "btkprivate.h"
#include "btkdnd.h"
#include "btkbuildable.h"

#include "btkalias.h"

#define SCROLL_DELAY_FACTOR   5
#define SCROLL_THRESHOLD      12
#define DND_THRESHOLD_MULTIPLIER 4
#define FRAMES_PER_SECOND     45
#define MSECS_BETWEEN_UPDATES (1000 / FRAMES_PER_SECOND)

enum {
  SWITCH_PAGE,
  FOCUS_TAB,
  SELECT_PAGE,
  CHANGE_CURRENT_PAGE,
  MOVE_FOCUS_OUT,
  REORDER_TAB,
  PAGE_REORDERED,
  PAGE_REMOVED,
  PAGE_ADDED,
  CREATE_WINDOW,
  LAST_SIGNAL
};

enum {
  STEP_PREV,
  STEP_NEXT
};

typedef enum
{
  ARROW_NONE,
  ARROW_LEFT_BEFORE,
  ARROW_RIGHT_BEFORE,
  ARROW_LEFT_AFTER,
  ARROW_RIGHT_AFTER
} BtkNotebookArrow;

typedef enum
{
  POINTER_BEFORE,
  POINTER_AFTER,
  POINTER_BETWEEN
} BtkNotebookPointerPosition;

typedef enum
{
  DRAG_OPERATION_NONE,
  DRAG_OPERATION_REORDER,
  DRAG_OPERATION_DETACH
} BtkNotebookDragOperation;

#define ARROW_IS_LEFT(arrow)  ((arrow) == ARROW_LEFT_BEFORE || (arrow) == ARROW_LEFT_AFTER)
#define ARROW_IS_BEFORE(arrow) ((arrow) == ARROW_LEFT_BEFORE || (arrow) == ARROW_RIGHT_BEFORE)

enum {
  PROP_0,
  PROP_TAB_POS,
  PROP_SHOW_TABS,
  PROP_SHOW_BORDER,
  PROP_SCROLLABLE,
  PROP_TAB_BORDER,
  PROP_TAB_HBORDER,
  PROP_TAB_VBORDER,
  PROP_PAGE,
  PROP_ENABLE_POPUP,
  PROP_GROUP_ID,
  PROP_GROUP,
  PROP_GROUP_NAME,
  PROP_HOMOGENEOUS
};

enum {
  CHILD_PROP_0,
  CHILD_PROP_TAB_LABEL,
  CHILD_PROP_MENU_LABEL,
  CHILD_PROP_POSITION,
  CHILD_PROP_TAB_EXPAND,
  CHILD_PROP_TAB_FILL,
  CHILD_PROP_TAB_PACK,
  CHILD_PROP_REORDERABLE,
  CHILD_PROP_DETACHABLE
};

enum {
  ACTION_WIDGET_START,
  ACTION_WIDGET_END,
  N_ACTION_WIDGETS
};

#define BTK_NOTEBOOK_PAGE(_glist_)         ((BtkNotebookPage *)((GList *)(_glist_))->data)

/* some useful defines for calculating coords */
#define PAGE_LEFT_X(_page_)   (((BtkNotebookPage *) (_page_))->allocation.x)
#define PAGE_RIGHT_X(_page_)  (((BtkNotebookPage *) (_page_))->allocation.x + ((BtkNotebookPage *) (_page_))->allocation.width)
#define PAGE_MIDDLE_X(_page_) (((BtkNotebookPage *) (_page_))->allocation.x + ((BtkNotebookPage *) (_page_))->allocation.width / 2)
#define PAGE_TOP_Y(_page_)    (((BtkNotebookPage *) (_page_))->allocation.y)
#define PAGE_BOTTOM_Y(_page_) (((BtkNotebookPage *) (_page_))->allocation.y + ((BtkNotebookPage *) (_page_))->allocation.height)
#define PAGE_MIDDLE_Y(_page_) (((BtkNotebookPage *) (_page_))->allocation.y + ((BtkNotebookPage *) (_page_))->allocation.height / 2)
#define NOTEBOOK_IS_TAB_LABEL_PARENT(_notebook_,_page_) (((BtkNotebookPage *) (_page_))->tab_label->parent == ((BtkWidget *) (_notebook_)))

struct _BtkNotebookPage
{
  BtkWidget *child;
  BtkWidget *tab_label;
  BtkWidget *menu_label;
  BtkWidget *last_focus_child;	/* Last descendant of the page that had focus */

  buint default_menu : 1;	/* If true, we create the menu label ourself */
  buint default_tab  : 1;	/* If true, we create the tab label ourself */
  buint expand       : 1;
  buint fill         : 1;
  buint pack         : 1;
  buint reorderable  : 1;
  buint detachable   : 1;

  /* if true, the tab label was visible on last allocation; we track this so
   * that we know to redraw the tab area if a tab label was hidden then shown
   * without changing position */
  buint tab_allocated_visible : 1;

  BtkRequisition requisition;
  BtkAllocation allocation;

  bulong mnemonic_activate_signal;
  bulong notify_visible_handler;
};

#define BTK_NOTEBOOK_GET_PRIVATE(obj) (B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_NOTEBOOK, BtkNotebookPrivate))

typedef struct _BtkNotebookPrivate BtkNotebookPrivate;

struct _BtkNotebookPrivate
{
  bpointer group;
  bint  mouse_x;
  bint  mouse_y;
  bint  pressed_button;
  buint dnd_timer;
  buint switch_tab_timer;

  bint  drag_begin_x;
  bint  drag_begin_y;

  bint  drag_offset_x;
  bint  drag_offset_y;

  BtkWidget *dnd_window;
  BtkTargetList *source_targets;
  BtkNotebookDragOperation operation;
  BdkWindow *drag_window;
  bint drag_window_x;
  bint drag_window_y;
  BtkNotebookPage *detached_tab;

  buint32 timestamp;

  BtkWidget *action_widget[N_ACTION_WIDGETS];

  buint during_reorder : 1;
  buint during_detach  : 1;
  buint has_scrolled   : 1;
};

static const BtkTargetEntry notebook_targets [] = {
  { "BTK_NOTEBOOK_TAB", BTK_TARGET_SAME_APP, 0 },
};

#ifdef G_DISABLE_CHECKS
#define CHECK_FIND_CHILD(notebook, child)                           \
 btk_notebook_find_child (notebook, child, B_STRLOC)
#else
#define CHECK_FIND_CHILD(notebook, child)                           \
 btk_notebook_find_child (notebook, child, NULL)
#endif
 
/*** BtkNotebook Methods ***/
static bboolean btk_notebook_select_page         (BtkNotebook      *notebook,
						  bboolean          move_focus);
static bboolean btk_notebook_focus_tab           (BtkNotebook      *notebook,
						  BtkNotebookTab    type);
static bboolean btk_notebook_change_current_page (BtkNotebook      *notebook,
						  bint              offset);
static void     btk_notebook_move_focus_out      (BtkNotebook      *notebook,
						  BtkDirectionType  direction_type);
static bboolean btk_notebook_reorder_tab         (BtkNotebook      *notebook,
						  BtkDirectionType  direction_type,
						  bboolean          move_to_last);
static void     btk_notebook_remove_tab_label    (BtkNotebook      *notebook,
						  BtkNotebookPage  *page);

/*** BtkObject Methods ***/
static void btk_notebook_destroy             (BtkObject        *object);
static void btk_notebook_set_property	     (BObject         *object,
					      buint            prop_id,
					      const BValue    *value,
					      BParamSpec      *pspec);
static void btk_notebook_get_property	     (BObject         *object,
					      buint            prop_id,
					      BValue          *value,
					      BParamSpec      *pspec);

/*** BtkWidget Methods ***/
static void btk_notebook_map                 (BtkWidget        *widget);
static void btk_notebook_unmap               (BtkWidget        *widget);
static void btk_notebook_realize             (BtkWidget        *widget);
static void btk_notebook_unrealize           (BtkWidget        *widget);
static void btk_notebook_size_request        (BtkWidget        *widget,
					      BtkRequisition   *requisition);
static void btk_notebook_size_allocate       (BtkWidget        *widget,
					      BtkAllocation    *allocation);
static bint btk_notebook_expose              (BtkWidget        *widget,
					      BdkEventExpose   *event);
static bboolean btk_notebook_scroll          (BtkWidget        *widget,
                                              BdkEventScroll   *event);
static bint btk_notebook_button_press        (BtkWidget        *widget,
					      BdkEventButton   *event);
static bint btk_notebook_button_release      (BtkWidget        *widget,
					      BdkEventButton   *event);
static bboolean btk_notebook_popup_menu      (BtkWidget        *widget);
static bint btk_notebook_leave_notify        (BtkWidget        *widget,
					      BdkEventCrossing *event);
static bint btk_notebook_motion_notify       (BtkWidget        *widget,
					      BdkEventMotion   *event);
static bint btk_notebook_focus_in            (BtkWidget        *widget,
					      BdkEventFocus    *event);
static bint btk_notebook_focus_out           (BtkWidget        *widget,
					      BdkEventFocus    *event);
static void btk_notebook_grab_notify         (BtkWidget          *widget,
					      bboolean            was_grabbed);
static void btk_notebook_state_changed       (BtkWidget          *widget,
					      BtkStateType        previous_state);
static void btk_notebook_draw_focus          (BtkWidget        *widget,
					      BdkEventExpose   *event);
static bint btk_notebook_focus               (BtkWidget        *widget,
					      BtkDirectionType  direction);
static void btk_notebook_style_set           (BtkWidget        *widget,
					      BtkStyle         *previous);

/*** Drag and drop Methods ***/
static void btk_notebook_drag_begin          (BtkWidget        *widget,
					      BdkDragContext   *context);
static void btk_notebook_drag_end            (BtkWidget        *widget,
					      BdkDragContext   *context);
static bboolean btk_notebook_drag_failed     (BtkWidget        *widget,
					      BdkDragContext   *context,
					      BtkDragResult     result,
					      bpointer          data);
static bboolean btk_notebook_drag_motion     (BtkWidget        *widget,
					      BdkDragContext   *context,
					      bint              x,
					      bint              y,
					      buint             time);
static void btk_notebook_drag_leave          (BtkWidget        *widget,
					      BdkDragContext   *context,
					      buint             time);
static bboolean btk_notebook_drag_drop       (BtkWidget        *widget,
					      BdkDragContext   *context,
					      bint              x,
					      bint              y,
					      buint             time);
static void btk_notebook_drag_data_get       (BtkWidget        *widget,
					      BdkDragContext   *context,
					      BtkSelectionData *data,
					      buint             info,
					      buint             time);
static void btk_notebook_drag_data_received  (BtkWidget        *widget,
					      BdkDragContext   *context,
					      bint              x,
					      bint              y,
					      BtkSelectionData *data,
					      buint             info,
					      buint             time);

/*** BtkContainer Methods ***/
static void btk_notebook_set_child_property  (BtkContainer     *container,
					      BtkWidget        *child,
					      buint             property_id,
					      const BValue     *value,
					      BParamSpec       *pspec);
static void btk_notebook_get_child_property  (BtkContainer     *container,
					      BtkWidget        *child,
					      buint             property_id,
					      BValue           *value,
					      BParamSpec       *pspec);
static void btk_notebook_add                 (BtkContainer     *container,
					      BtkWidget        *widget);
static void btk_notebook_remove              (BtkContainer     *container,
					      BtkWidget        *widget);
static void btk_notebook_set_focus_child     (BtkContainer     *container,
					      BtkWidget        *child);
static GType btk_notebook_child_type       (BtkContainer     *container);
static void btk_notebook_forall              (BtkContainer     *container,
					      bboolean		include_internals,
					      BtkCallback       callback,
					      bpointer          callback_data);

/*** BtkNotebook Methods ***/
static bint btk_notebook_real_insert_page    (BtkNotebook      *notebook,
					      BtkWidget        *child,
					      BtkWidget        *tab_label,
					      BtkWidget        *menu_label,
					      bint              position);

static BtkNotebook *btk_notebook_create_window (BtkNotebook    *notebook,
                                                BtkWidget      *page,
                                                bint            x,
                                                bint            y);

/*** BtkNotebook Private Functions ***/
static void btk_notebook_redraw_tabs         (BtkNotebook      *notebook);
static void btk_notebook_redraw_arrows       (BtkNotebook      *notebook);
static void btk_notebook_real_remove         (BtkNotebook      *notebook,
					      GList            *list);
static void btk_notebook_update_labels       (BtkNotebook      *notebook);
static bint btk_notebook_timer               (BtkNotebook      *notebook);
static void btk_notebook_set_scroll_timer    (BtkNotebook *notebook);
static bint btk_notebook_page_compare        (gconstpointer     a,
					      gconstpointer     b);
static GList* btk_notebook_find_child        (BtkNotebook      *notebook,
					      BtkWidget        *child,
					      const bchar      *function);
static bint  btk_notebook_real_page_position (BtkNotebook      *notebook,
					      GList            *list);
static GList * btk_notebook_search_page      (BtkNotebook      *notebook,
					      GList            *list,
					      bint              direction,
					      bboolean          find_visible);
static void  btk_notebook_child_reordered    (BtkNotebook      *notebook,
			                      BtkNotebookPage  *page);

/*** BtkNotebook Drawing Functions ***/
static void btk_notebook_paint               (BtkWidget        *widget,
					      BdkRectangle     *area);
static void btk_notebook_draw_tab            (BtkNotebook      *notebook,
					      BtkNotebookPage  *page,
					      BdkRectangle     *area);
static void btk_notebook_draw_arrow          (BtkNotebook      *notebook,
					      BtkNotebookArrow  arrow);

/*** BtkNotebook Size Allocate Functions ***/
static void btk_notebook_pages_allocate      (BtkNotebook      *notebook);
static bboolean btk_notebook_page_allocate   (BtkNotebook      *notebook,
					      BtkNotebookPage  *page);
static void btk_notebook_calc_tabs           (BtkNotebook      *notebook,
			                      GList            *start,
					      GList           **end,
					      bint             *tab_space,
					      buint             direction);

/*** BtkNotebook Page Switch Methods ***/
static void btk_notebook_real_switch_page    (BtkNotebook      *notebook,
					      BtkNotebookPage  *child,
					      buint             page_num);

/*** BtkNotebook Page Switch Functions ***/
static void btk_notebook_switch_page         (BtkNotebook      *notebook,
					      BtkNotebookPage  *page);
static bint btk_notebook_page_select         (BtkNotebook      *notebook,
					      bboolean          move_focus);
static void btk_notebook_switch_focus_tab    (BtkNotebook      *notebook,
                                              GList            *new_child);
static void btk_notebook_menu_switch_page    (BtkWidget        *widget,
					      BtkNotebookPage  *page);

/*** BtkNotebook Menu Functions ***/
static void btk_notebook_menu_item_create    (BtkNotebook      *notebook,
					      GList            *list);
static void btk_notebook_menu_label_unparent (BtkWidget        *widget,
					      bpointer          data);
static void btk_notebook_menu_detacher       (BtkWidget        *widget,
					      BtkMenu          *menu);

/*** BtkNotebook Private Setters ***/
static void btk_notebook_set_homogeneous_tabs_internal (BtkNotebook *notebook,
							bboolean     homogeneous);
static void btk_notebook_set_tab_border_internal       (BtkNotebook *notebook,
							buint        border_width);
static void btk_notebook_set_tab_hborder_internal      (BtkNotebook *notebook,
							buint        tab_hborder);
static void btk_notebook_set_tab_vborder_internal      (BtkNotebook *notebook,
							buint        tab_vborder);

static void btk_notebook_update_tab_states             (BtkNotebook *notebook);
static bboolean btk_notebook_mnemonic_activate_switch_page (BtkWidget *child,
							    bboolean overload,
							    bpointer data);

static bboolean focus_tabs_in  (BtkNotebook      *notebook);
static bboolean focus_child_in (BtkNotebook      *notebook,
				BtkDirectionType  direction);

static void stop_scrolling (BtkNotebook *notebook);
static void do_detach_tab  (BtkNotebook *from,
			    BtkNotebook *to,
			    BtkWidget   *child,
			    bint         x,
			    bint         y);

/* BtkBuildable */
static void btk_notebook_buildable_init           (BtkBuildableIface *iface);
static void btk_notebook_buildable_add_child      (BtkBuildable *buildable,
						   BtkBuilder   *builder,
						   BObject      *child,
						   const bchar  *type);

static BtkNotebookWindowCreationFunc window_creation_hook = NULL;
static bpointer window_creation_hook_data;
static GDestroyNotify window_creation_hook_destroy = NULL;

static buint notebook_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (BtkNotebook, btk_notebook, BTK_TYPE_CONTAINER,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
						btk_notebook_buildable_init))

static void
add_tab_bindings (BtkBindingSet    *binding_set,
		  BdkModifierType   modifiers,
		  BtkDirectionType  direction)
{
  btk_binding_entry_add_signal (binding_set, BDK_Tab, modifiers,
                                "move_focus_out", 1,
                                BTK_TYPE_DIRECTION_TYPE, direction);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Tab, modifiers,
                                "move_focus_out", 1,
                                BTK_TYPE_DIRECTION_TYPE, direction);
}

static void
add_arrow_bindings (BtkBindingSet    *binding_set,
		    buint             keysym,
		    BtkDirectionType  direction)
{
  buint keypad_keysym = keysym - BDK_Left + BDK_KP_Left;
  
  btk_binding_entry_add_signal (binding_set, keysym, BDK_CONTROL_MASK,
                                "move_focus_out", 1,
                                BTK_TYPE_DIRECTION_TYPE, direction);
  btk_binding_entry_add_signal (binding_set, keypad_keysym, BDK_CONTROL_MASK,
                                "move_focus_out", 1,
                                BTK_TYPE_DIRECTION_TYPE, direction);
}

static void
add_reorder_bindings (BtkBindingSet    *binding_set,
		      buint             keysym,
		      BtkDirectionType  direction,
		      bboolean          move_to_last)
{
  buint keypad_keysym = keysym - BDK_Left + BDK_KP_Left;

  btk_binding_entry_add_signal (binding_set, keysym, BDK_MOD1_MASK,
				"reorder_tab", 2,
				BTK_TYPE_DIRECTION_TYPE, direction,
				B_TYPE_BOOLEAN, move_to_last);
  btk_binding_entry_add_signal (binding_set, keypad_keysym, BDK_MOD1_MASK,
				"reorder_tab", 2,
				BTK_TYPE_DIRECTION_TYPE, direction,
				B_TYPE_BOOLEAN, move_to_last);
}

static bboolean
btk_object_handled_accumulator (GSignalInvocationHint *ihint,
                                BValue                *return_accu,
                                const BValue          *handler_return,
                                bpointer               dummy)
{
  bboolean continue_emission;
  BObject *object;

  object = b_value_get_object (handler_return);
  b_value_set_object (return_accu, object);
  continue_emission = !object;

  return continue_emission;
}

static void
btk_notebook_class_init (BtkNotebookClass *class)
{
  BObjectClass   *bobject_class = B_OBJECT_CLASS (class);
  BtkObjectClass *object_class = BTK_OBJECT_CLASS (class);
  BtkWidgetClass *widget_class = BTK_WIDGET_CLASS (class);
  BtkContainerClass *container_class = BTK_CONTAINER_CLASS (class);
  BtkBindingSet *binding_set;
  
  bobject_class->set_property = btk_notebook_set_property;
  bobject_class->get_property = btk_notebook_get_property;
  object_class->destroy = btk_notebook_destroy;

  widget_class->map = btk_notebook_map;
  widget_class->unmap = btk_notebook_unmap;
  widget_class->realize = btk_notebook_realize;
  widget_class->unrealize = btk_notebook_unrealize;
  widget_class->size_request = btk_notebook_size_request;
  widget_class->size_allocate = btk_notebook_size_allocate;
  widget_class->expose_event = btk_notebook_expose;
  widget_class->scroll_event = btk_notebook_scroll;
  widget_class->button_press_event = btk_notebook_button_press;
  widget_class->button_release_event = btk_notebook_button_release;
  widget_class->popup_menu = btk_notebook_popup_menu;
  widget_class->leave_notify_event = btk_notebook_leave_notify;
  widget_class->motion_notify_event = btk_notebook_motion_notify;
  widget_class->grab_notify = btk_notebook_grab_notify;
  widget_class->state_changed = btk_notebook_state_changed;
  widget_class->focus_in_event = btk_notebook_focus_in;
  widget_class->focus_out_event = btk_notebook_focus_out;
  widget_class->focus = btk_notebook_focus;
  widget_class->style_set = btk_notebook_style_set;
  widget_class->drag_begin = btk_notebook_drag_begin;
  widget_class->drag_end = btk_notebook_drag_end;
  widget_class->drag_motion = btk_notebook_drag_motion;
  widget_class->drag_leave = btk_notebook_drag_leave;
  widget_class->drag_drop = btk_notebook_drag_drop;
  widget_class->drag_data_get = btk_notebook_drag_data_get;
  widget_class->drag_data_received = btk_notebook_drag_data_received;

  container_class->add = btk_notebook_add;
  container_class->remove = btk_notebook_remove;
  container_class->forall = btk_notebook_forall;
  container_class->set_focus_child = btk_notebook_set_focus_child;
  container_class->get_child_property = btk_notebook_get_child_property;
  container_class->set_child_property = btk_notebook_set_child_property;
  container_class->child_type = btk_notebook_child_type;

  class->switch_page = btk_notebook_real_switch_page;
  class->insert_page = btk_notebook_real_insert_page;

  class->focus_tab = btk_notebook_focus_tab;
  class->select_page = btk_notebook_select_page;
  class->change_current_page = btk_notebook_change_current_page;
  class->move_focus_out = btk_notebook_move_focus_out;
  class->reorder_tab = btk_notebook_reorder_tab;
  class->create_window = btk_notebook_create_window;
  
  g_object_class_install_property (bobject_class,
				   PROP_PAGE,
				   g_param_spec_int ("page",
 						     P_("Page"),
 						     P_("The index of the current page"),
 						     -1,
 						     B_MAXINT,
 						     -1,
 						     BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_TAB_POS,
				   g_param_spec_enum ("tab-pos",
 						      P_("Tab Position"),
 						      P_("Which side of the notebook holds the tabs"),
 						      BTK_TYPE_POSITION_TYPE,
 						      BTK_POS_TOP,
 						      BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_TAB_BORDER,
				   g_param_spec_uint ("tab-border",
 						      P_("Tab Border"),
 						      P_("Width of the border around the tab labels"),
 						      0,
 						      B_MAXUINT,
 						      2,
 						      BTK_PARAM_WRITABLE));
  g_object_class_install_property (bobject_class,
				   PROP_TAB_HBORDER,
				   g_param_spec_uint ("tab-hborder",
 						      P_("Horizontal Tab Border"),
 						      P_("Width of the horizontal border of tab labels"),
 						      0,
 						      B_MAXUINT,
 						      2,
 						      BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_TAB_VBORDER,
				   g_param_spec_uint ("tab-vborder",
 						      P_("Vertical Tab Border"),
 						      P_("Width of the vertical border of tab labels"),
 						      0,
 						      B_MAXUINT,
 						      2,
 						      BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_SHOW_TABS,
				   g_param_spec_boolean ("show-tabs",
 							 P_("Show Tabs"),
 							 P_("Whether tabs should be shown or not"),
 							 TRUE,
 							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_SHOW_BORDER,
				   g_param_spec_boolean ("show-border",
 							 P_("Show Border"),
 							 P_("Whether the border should be shown or not"),
 							 TRUE,
 							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_SCROLLABLE,
				   g_param_spec_boolean ("scrollable",
 							 P_("Scrollable"),
 							 P_("If TRUE, scroll arrows are added if there are too many tabs to fit"),
 							 FALSE,
 							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_ENABLE_POPUP,
				   g_param_spec_boolean ("enable-popup",
 							 P_("Enable Popup"),
 							 P_("If TRUE, pressing the right mouse button on the notebook pops up a menu that you can use to go to a page"),
 							 FALSE,
 							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_HOMOGENEOUS,
				   g_param_spec_boolean ("homogeneous",
 							 P_("Homogeneous"),
 							 P_("Whether tabs should have homogeneous sizes"),
 							 FALSE,
							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_GROUP_ID,
				   g_param_spec_int ("group-id",
						     P_("Group ID"),
						     P_("Group ID for tabs drag and drop"),
						     -1,
						     B_MAXINT,
						     -1,
						     BTK_PARAM_READWRITE|G_PARAM_DEPRECATED));

  /**
   * BtkNotebook:group:
   *  
   * Group for tabs drag and drop.
   *
   * Since: 2.12
   *
   * Deprecated: 2.24: Use #BtkNotebook:group-name instead
   */    
  g_object_class_install_property (bobject_class,
				   PROP_GROUP,
				   g_param_spec_pointer ("group",
							 P_("Group"),
							 P_("Group for tabs drag and drop"),
							 BTK_PARAM_READWRITE|G_PARAM_DEPRECATED));

  /**
   * BtkNotebook:group-name:
   *
   * Group name for tabs drag and drop.
   *
   * Since: 2.24
   */
  g_object_class_install_property (bobject_class,
                                   PROP_GROUP_NAME,
                                   g_param_spec_string ("group-name",
                                   P_("Group Name"),
                                   P_("Group name for tabs drag and drop"),
                                   NULL,
                                   BTK_PARAM_READWRITE));

  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_TAB_LABEL,
					      g_param_spec_string ("tab-label", 
								   P_("Tab label"),
								   P_("The string displayed on the child's tab label"),
								   NULL,
								   BTK_PARAM_READWRITE));
  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_MENU_LABEL,
					      g_param_spec_string ("menu-label", 
								   P_("Menu label"), 
								   P_("The string displayed in the child's menu entry"),
								   NULL,
								   BTK_PARAM_READWRITE));
  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_POSITION,
					      g_param_spec_int ("position", 
								P_("Position"), 
								P_("The index of the child in the parent"),
								-1, B_MAXINT, 0,
								BTK_PARAM_READWRITE));
  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_TAB_EXPAND,
					      g_param_spec_boolean ("tab-expand", 
								    P_("Tab expand"), 
								    P_("Whether to expand the child's tab or not"),
								    FALSE,
								    BTK_PARAM_READWRITE));
  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_TAB_FILL,
					      g_param_spec_boolean ("tab-fill", 
								    P_("Tab fill"), 
								    P_("Whether the child's tab should fill the allocated area or not"),
								    TRUE,
								    BTK_PARAM_READWRITE));
  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_TAB_PACK,
					      g_param_spec_enum ("tab-pack", 
								 P_("Tab pack type"),
								 P_("A BtkPackType indicating whether the child is packed with reference to the start or end of the parent"),
								 BTK_TYPE_PACK_TYPE, BTK_PACK_START,
								 BTK_PARAM_READWRITE));
  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_REORDERABLE,
					      g_param_spec_boolean ("reorderable",
								    P_("Tab reorderable"),
								    P_("Whether the tab is reorderable by user action or not"),
								    FALSE,
								    BTK_PARAM_READWRITE));
  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_DETACHABLE,
					      g_param_spec_boolean ("detachable",
								    P_("Tab detachable"),
								    P_("Whether the tab is detachable"),
								    FALSE,
								    BTK_PARAM_READWRITE));

/**
 * BtkNotebook:has-secondary-backward-stepper:
 *
 * The "has-secondary-backward-stepper" property determines whether 
 * a second backward arrow button is displayed on the opposite end 
 * of the tab area.
 *
 * Since: 2.4
 */  
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean ("has-secondary-backward-stepper",
								 P_("Secondary backward stepper"),
								 P_("Display a second backward arrow button on the opposite end of the tab area"),
								 FALSE,
								 BTK_PARAM_READABLE));

/**
 * BtkNotebook:has-secondary-forward-stepper:
 *
 * The "has-secondary-forward-stepper" property determines whether 
 * a second forward arrow button is displayed on the opposite end 
 * of the tab area.
 *
 * Since: 2.4
 */  
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean ("has-secondary-forward-stepper",
								 P_("Secondary forward stepper"),
								 P_("Display a second forward arrow button on the opposite end of the tab area"),
								 FALSE,
								 BTK_PARAM_READABLE));

/**
 * BtkNotebook:has-backward-stepper:
 *
 * The "has-backward-stepper" property determines whether 
 * the standard backward arrow button is displayed.
 *
 * Since: 2.4
 */  
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean ("has-backward-stepper",
								 P_("Backward stepper"),
								 P_("Display the standard backward arrow button"),
								 TRUE,
								 BTK_PARAM_READABLE));

/**
 * BtkNotebook:has-forward-stepper:
 *
 * The "has-forward-stepper" property determines whether 
 * the standard forward arrow button is displayed.
 *
 * Since: 2.4
 */  
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean ("has-forward-stepper",
								 P_("Forward stepper"),
								 P_("Display the standard forward arrow button"),
								 TRUE,
								 BTK_PARAM_READABLE));
  
/**
 * BtkNotebook:tab-overlap:
 *
 * The "tab-overlap" property defines size of tab overlap
 * area.
 *
 * Since: 2.10
 */  
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("tab-overlap",
							     P_("Tab overlap"),
							     P_("Size of tab overlap area"),
							     B_MININT,
							     B_MAXINT,
							     2,
							     BTK_PARAM_READABLE));

/**
 * BtkNotebook:tab-curvature:
 *
 * The "tab-curvature" property defines size of tab curvature.
 *
 * Since: 2.10
 */  
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("tab-curvature",
							     P_("Tab curvature"),
							     P_("Size of tab curvature"),
							     0,
							     B_MAXINT,
							     1,
							     BTK_PARAM_READABLE));

  /**
   * BtkNotebook:arrow-spacing:
   *
   * The "arrow-spacing" property defines the spacing between the scroll
   * arrows and the tabs.
   *
   * Since: 2.10
   */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("arrow-spacing",
                                                             P_("Arrow spacing"),
                                                             P_("Scroll arrow spacing"),
                                                             0,
                                                             B_MAXINT,
                                                             0,
                                                             BTK_PARAM_READABLE));

  notebook_signals[SWITCH_PAGE] =
    g_signal_new (I_("switch-page"),
		  B_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkNotebookClass, switch_page),
		  NULL, NULL,
		  _btk_marshal_VOID__POINTER_UINT,
		  B_TYPE_NONE, 2,
		  B_TYPE_POINTER,
		  B_TYPE_UINT);
  notebook_signals[FOCUS_TAB] = 
    g_signal_new (I_("focus-tab"),
                  B_TYPE_FROM_CLASS (bobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BtkNotebookClass, focus_tab),
                  NULL, NULL,
                  _btk_marshal_BOOLEAN__ENUM,
                  B_TYPE_BOOLEAN, 1,
                  BTK_TYPE_NOTEBOOK_TAB);
  notebook_signals[SELECT_PAGE] = 
    g_signal_new (I_("select-page"),
                  B_TYPE_FROM_CLASS (bobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BtkNotebookClass, select_page),
                  NULL, NULL,
                  _btk_marshal_BOOLEAN__BOOLEAN,
                  B_TYPE_BOOLEAN, 1,
                  B_TYPE_BOOLEAN);
  notebook_signals[CHANGE_CURRENT_PAGE] = 
    g_signal_new (I_("change-current-page"),
                  B_TYPE_FROM_CLASS (bobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BtkNotebookClass, change_current_page),
                  NULL, NULL,
                  _btk_marshal_BOOLEAN__INT,
                  B_TYPE_BOOLEAN, 1,
                  B_TYPE_INT);
  notebook_signals[MOVE_FOCUS_OUT] =
    g_signal_new (I_("move-focus-out"),
                  B_TYPE_FROM_CLASS (bobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BtkNotebookClass, move_focus_out),
                  NULL, NULL,
                  _btk_marshal_VOID__ENUM,
                  B_TYPE_NONE, 1,
                  BTK_TYPE_DIRECTION_TYPE);
  notebook_signals[REORDER_TAB] =
    g_signal_new (I_("reorder-tab"),
                  B_TYPE_FROM_CLASS (bobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BtkNotebookClass, reorder_tab),
                  NULL, NULL,
                  _btk_marshal_BOOLEAN__ENUM_BOOLEAN,
                  B_TYPE_BOOLEAN, 2,
                  BTK_TYPE_DIRECTION_TYPE,
		  B_TYPE_BOOLEAN);
  /**
   * BtkNotebook::page-reordered:
   * @notebook: the #BtkNotebook
   * @child: the child #BtkWidget affected
   * @page_num: the new page number for @child
   *
   * the ::page-reordered signal is emitted in the notebook
   * right after a page has been reordered.
   *
   * Since: 2.10
   **/
  notebook_signals[PAGE_REORDERED] =
    g_signal_new (I_("page-reordered"),
                  B_TYPE_FROM_CLASS (bobject_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
		  _btk_marshal_VOID__OBJECT_UINT,
                  B_TYPE_NONE, 2,
		  BTK_TYPE_WIDGET,
		  B_TYPE_UINT);
  /**
   * BtkNotebook::page-removed:
   * @notebook: the #BtkNotebook
   * @child: the child #BtkWidget affected
   * @page_num: the @child page number
   *
   * the ::page-removed signal is emitted in the notebook
   * right after a page is removed from the notebook.
   *
   * Since: 2.10
   **/
  notebook_signals[PAGE_REMOVED] =
    g_signal_new (I_("page-removed"),
                  B_TYPE_FROM_CLASS (bobject_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
		  _btk_marshal_VOID__OBJECT_UINT,
                  B_TYPE_NONE, 2,
		  BTK_TYPE_WIDGET,
		  B_TYPE_UINT);
  /**
   * BtkNotebook::page-added:
   * @notebook: the #BtkNotebook
   * @child: the child #BtkWidget affected
   * @page_num: the new page number for @child
   *
   * the ::page-added signal is emitted in the notebook
   * right after a page is added to the notebook.
   *
   * Since: 2.10
   **/
  notebook_signals[PAGE_ADDED] =
    g_signal_new (I_("page-added"),
                  B_TYPE_FROM_CLASS (bobject_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
		  _btk_marshal_VOID__OBJECT_UINT,
                  B_TYPE_NONE, 2,
		  BTK_TYPE_WIDGET,
		  B_TYPE_UINT);

  /**
   * BtkNotebook::create-window:
   * @notebook: the #BtkNotebook emitting the signal
   * @page: the tab of @notebook that is being detached
   * @x: the X coordinate where the drop happens
   * @y: the Y coordinate where the drop happens
   *
   * The ::create-window signal is emitted when a detachable
   * tab is dropped on the root window. 
   *
   * A handler for this signal can create a window containing 
   * a notebook where the tab will be attached. It is also 
   * responsible for moving/resizing the window and adding the 
   * necessary properties to the notebook (e.g. the 
   * #BtkNotebook:group ).
   *
   * The default handler uses the global window creation hook,
   * if one has been set with btk_notebook_set_window_creation_hook().
   *
   * Returns: (transfer none): a #BtkNotebook that @page should be
   *     added to, or %NULL.
   *
   * Since: 2.12
   */
  notebook_signals[CREATE_WINDOW] = 
    g_signal_new (I_("create-window"),
                  B_TYPE_FROM_CLASS (bobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkNotebookClass, create_window),
                  btk_object_handled_accumulator, NULL,
                  _btk_marshal_OBJECT__OBJECT_INT_INT,
                  BTK_TYPE_NOTEBOOK, 3,
                  BTK_TYPE_WIDGET, B_TYPE_INT, B_TYPE_INT);
 
  binding_set = btk_binding_set_by_class (class);
  btk_binding_entry_add_signal (binding_set,
                                BDK_space, 0,
                                "select-page", 1, 
                                B_TYPE_BOOLEAN, FALSE);
  btk_binding_entry_add_signal (binding_set,
                                BDK_KP_Space, 0,
                                "select-page", 1, 
                                B_TYPE_BOOLEAN, FALSE);
  
  btk_binding_entry_add_signal (binding_set,
                                BDK_Home, 0,
                                "focus-tab", 1, 
                                BTK_TYPE_NOTEBOOK_TAB, BTK_NOTEBOOK_TAB_FIRST);
  btk_binding_entry_add_signal (binding_set,
                                BDK_KP_Home, 0,
                                "focus-tab", 1, 
                                BTK_TYPE_NOTEBOOK_TAB, BTK_NOTEBOOK_TAB_FIRST);
  btk_binding_entry_add_signal (binding_set,
                                BDK_End, 0,
                                "focus-tab", 1, 
                                BTK_TYPE_NOTEBOOK_TAB, BTK_NOTEBOOK_TAB_LAST);
  btk_binding_entry_add_signal (binding_set,
                                BDK_KP_End, 0,
                                "focus-tab", 1, 
                                BTK_TYPE_NOTEBOOK_TAB, BTK_NOTEBOOK_TAB_LAST);

  btk_binding_entry_add_signal (binding_set,
                                BDK_Page_Up, BDK_CONTROL_MASK,
                                "change-current-page", 1,
                                B_TYPE_INT, -1);
  btk_binding_entry_add_signal (binding_set,
                                BDK_Page_Down, BDK_CONTROL_MASK,
                                "change-current-page", 1,
                                B_TYPE_INT, 1);

  btk_binding_entry_add_signal (binding_set,
                                BDK_Page_Up, BDK_CONTROL_MASK | BDK_MOD1_MASK,
                                "change-current-page", 1,
                                B_TYPE_INT, -1);
  btk_binding_entry_add_signal (binding_set,
                                BDK_Page_Down, BDK_CONTROL_MASK | BDK_MOD1_MASK,
                                "change-current-page", 1,
                                B_TYPE_INT, 1);

  add_arrow_bindings (binding_set, BDK_Up, BTK_DIR_UP);
  add_arrow_bindings (binding_set, BDK_Down, BTK_DIR_DOWN);
  add_arrow_bindings (binding_set, BDK_Left, BTK_DIR_LEFT);
  add_arrow_bindings (binding_set, BDK_Right, BTK_DIR_RIGHT);

  add_reorder_bindings (binding_set, BDK_Up, BTK_DIR_UP, FALSE);
  add_reorder_bindings (binding_set, BDK_Down, BTK_DIR_DOWN, FALSE);
  add_reorder_bindings (binding_set, BDK_Left, BTK_DIR_LEFT, FALSE);
  add_reorder_bindings (binding_set, BDK_Right, BTK_DIR_RIGHT, FALSE);
  add_reorder_bindings (binding_set, BDK_Home, BTK_DIR_LEFT, TRUE);
  add_reorder_bindings (binding_set, BDK_Home, BTK_DIR_UP, TRUE);
  add_reorder_bindings (binding_set, BDK_End, BTK_DIR_RIGHT, TRUE);
  add_reorder_bindings (binding_set, BDK_End, BTK_DIR_DOWN, TRUE);

  add_tab_bindings (binding_set, BDK_CONTROL_MASK, BTK_DIR_TAB_FORWARD);
  add_tab_bindings (binding_set, BDK_CONTROL_MASK | BDK_SHIFT_MASK, BTK_DIR_TAB_BACKWARD);

  g_type_class_add_private (class, sizeof (BtkNotebookPrivate));
}

static void
btk_notebook_init (BtkNotebook *notebook)
{
  BtkNotebookPrivate *priv;

  btk_widget_set_can_focus (BTK_WIDGET (notebook), TRUE);
  btk_widget_set_has_window (BTK_WIDGET (notebook), FALSE);

  priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);

  notebook->cur_page = NULL;
  notebook->children = NULL;
  notebook->first_tab = NULL;
  notebook->focus_tab = NULL;
  notebook->event_window = NULL;
  notebook->menu = NULL;

  notebook->tab_hborder = 2;
  notebook->tab_vborder = 2;

  notebook->show_tabs = TRUE;
  notebook->show_border = TRUE;
  notebook->tab_pos = BTK_POS_TOP;
  notebook->scrollable = FALSE;
  notebook->in_child = 0;
  notebook->click_child = 0;
  notebook->button = 0;
  notebook->need_timer = 0;
  notebook->child_has_focus = FALSE;
  notebook->have_visible_child = FALSE;
  notebook->focus_out = FALSE;

  notebook->has_before_previous = 1;
  notebook->has_before_next     = 0;
  notebook->has_after_previous  = 0;
  notebook->has_after_next      = 1;

  priv->group = NULL;
  priv->pressed_button = -1;
  priv->dnd_timer = 0;
  priv->switch_tab_timer = 0;
  priv->source_targets = btk_target_list_new (notebook_targets,
					      G_N_ELEMENTS (notebook_targets));
  priv->operation = DRAG_OPERATION_NONE;
  priv->detached_tab = NULL;
  priv->during_detach = FALSE;
  priv->has_scrolled = FALSE;

  btk_drag_dest_set (BTK_WIDGET (notebook), 0,
		     notebook_targets, G_N_ELEMENTS (notebook_targets),
                     BDK_ACTION_MOVE);

  g_signal_connect (B_OBJECT (notebook), "drag-failed",
		    G_CALLBACK (btk_notebook_drag_failed), NULL);

  btk_drag_dest_set_track_motion (BTK_WIDGET (notebook), TRUE);
}

static void
btk_notebook_buildable_init (BtkBuildableIface *iface)
{
  iface->add_child = btk_notebook_buildable_add_child;
}

static void
btk_notebook_buildable_add_child (BtkBuildable  *buildable,
				  BtkBuilder    *builder,
				  BObject       *child,
				  const bchar   *type)
{
  BtkNotebook *notebook = BTK_NOTEBOOK (buildable);

  if (type && strcmp (type, "tab") == 0)
    {
      BtkWidget * page;

      page = btk_notebook_get_nth_page (notebook, -1);
      /* To set the tab label widget, we must have already a child
       * inside the tab container. */
      g_assert (page != NULL);
      btk_notebook_set_tab_label (notebook, page, BTK_WIDGET (child));
    }
  else if (type && strcmp (type, "action-start") == 0)
    {
      btk_notebook_set_action_widget (notebook, BTK_WIDGET (child), BTK_PACK_START);
    }
  else if (type && strcmp (type, "action-end") == 0)
    {
      btk_notebook_set_action_widget (notebook, BTK_WIDGET (child), BTK_PACK_END);
    }
  else if (!type)
    btk_notebook_append_page (notebook, BTK_WIDGET (child), NULL);
  else
    BTK_BUILDER_WARN_INVALID_CHILD_TYPE (notebook, type);
}

static bboolean
btk_notebook_select_page (BtkNotebook *notebook,
                          bboolean     move_focus)
{
  if (btk_widget_is_focus (BTK_WIDGET (notebook)) && notebook->show_tabs)
    {
      btk_notebook_page_select (notebook, move_focus);
      return TRUE;
    }
  else
    return FALSE;
}

static bboolean
btk_notebook_focus_tab (BtkNotebook       *notebook,
                        BtkNotebookTab     type)
{
  GList *list;

  if (btk_widget_is_focus (BTK_WIDGET (notebook)) && notebook->show_tabs)
    {
      switch (type)
	{
	case BTK_NOTEBOOK_TAB_FIRST:
	  list = btk_notebook_search_page (notebook, NULL, STEP_NEXT, TRUE);
	  if (list)
	    btk_notebook_switch_focus_tab (notebook, list);
	  break;
	case BTK_NOTEBOOK_TAB_LAST:
	  list = btk_notebook_search_page (notebook, NULL, STEP_PREV, TRUE);
	  if (list)
	    btk_notebook_switch_focus_tab (notebook, list);
	  break;
	}

      return TRUE;
    }
  else
    return FALSE;
}

static bboolean
btk_notebook_change_current_page (BtkNotebook *notebook,
				  bint         offset)
{
  GList *current = NULL;

  if (!notebook->show_tabs)
    return FALSE;

  if (notebook->cur_page)
    current = g_list_find (notebook->children, notebook->cur_page);

  while (offset != 0)
    {
      current = btk_notebook_search_page (notebook, current,
                                          offset < 0 ? STEP_PREV : STEP_NEXT,
                                          TRUE);

      if (!current)
        {
          bboolean wrap_around;

          g_object_get (btk_widget_get_settings (BTK_WIDGET (notebook)),
                        "btk-keynav-wrap-around", &wrap_around,
                        NULL);

          if (wrap_around)
            current = btk_notebook_search_page (notebook, NULL,
                                                offset < 0 ? STEP_PREV : STEP_NEXT,
                                                TRUE);
          else
            break;
        }

      offset += offset < 0 ? 1 : -1;
    }

  if (current)
    btk_notebook_switch_page (notebook, current->data);
  else
    btk_widget_error_bell (BTK_WIDGET (notebook));

  return TRUE;
}

static BtkDirectionType
get_effective_direction (BtkNotebook      *notebook,
			 BtkDirectionType  direction)
{
  /* Remap the directions into the effective direction it would be for a
   * BTK_POS_TOP notebook
   */

#define D(rest) BTK_DIR_##rest

  static const BtkDirectionType translate_direction[2][4][6] = {
    /* LEFT */   {{ D(TAB_FORWARD),  D(TAB_BACKWARD), D(LEFT), D(RIGHT), D(UP),   D(DOWN) },
    /* RIGHT */  { D(TAB_BACKWARD), D(TAB_FORWARD),  D(LEFT), D(RIGHT), D(DOWN), D(UP)   },
    /* TOP */    { D(TAB_FORWARD),  D(TAB_BACKWARD), D(UP),   D(DOWN),  D(LEFT), D(RIGHT) },
    /* BOTTOM */ { D(TAB_BACKWARD), D(TAB_FORWARD),  D(DOWN), D(UP),    D(LEFT), D(RIGHT) }},
    /* LEFT */  {{ D(TAB_BACKWARD), D(TAB_FORWARD),  D(LEFT), D(RIGHT), D(DOWN), D(UP)   },
    /* RIGHT */  { D(TAB_FORWARD),  D(TAB_BACKWARD), D(LEFT), D(RIGHT), D(UP),   D(DOWN) },
    /* TOP */    { D(TAB_FORWARD),  D(TAB_BACKWARD), D(UP),   D(DOWN),  D(RIGHT), D(LEFT) },
    /* BOTTOM */ { D(TAB_BACKWARD), D(TAB_FORWARD),  D(DOWN), D(UP),    D(RIGHT), D(LEFT) }},
  };

#undef D

  int text_dir = btk_widget_get_direction (BTK_WIDGET (notebook)) == BTK_TEXT_DIR_RTL ? 1 : 0;

  return translate_direction[text_dir][notebook->tab_pos][direction];
}

static bint
get_effective_tab_pos (BtkNotebook *notebook)
{
  if (btk_widget_get_direction (BTK_WIDGET (notebook)) == BTK_TEXT_DIR_RTL)
    {
      switch (notebook->tab_pos) 
	{
	case BTK_POS_LEFT:
	  return BTK_POS_RIGHT;
	case BTK_POS_RIGHT:
	  return BTK_POS_LEFT;
	default: ;
	}
    }

  return notebook->tab_pos;
}

static bint
get_tab_gap_pos (BtkNotebook *notebook)
{
  bint tab_pos = get_effective_tab_pos (notebook);
  bint gap_side = BTK_POS_BOTTOM;
  
  switch (tab_pos)
    {
    case BTK_POS_TOP:
      gap_side = BTK_POS_BOTTOM;
      break;
    case BTK_POS_BOTTOM:
      gap_side = BTK_POS_TOP;
      break;
    case BTK_POS_LEFT:
      gap_side = BTK_POS_RIGHT;
      break;
    case BTK_POS_RIGHT:
      gap_side = BTK_POS_LEFT;
      break;
    }

  return gap_side;
}

static void
btk_notebook_move_focus_out (BtkNotebook      *notebook,
			     BtkDirectionType  direction_type)
{
  BtkDirectionType effective_direction = get_effective_direction (notebook, direction_type);
  BtkWidget *toplevel;
  
  if (BTK_CONTAINER (notebook)->focus_child && effective_direction == BTK_DIR_UP)
    if (focus_tabs_in (notebook))
      return;
  if (btk_widget_is_focus (BTK_WIDGET (notebook)) && effective_direction == BTK_DIR_DOWN)
    if (focus_child_in (notebook, BTK_DIR_TAB_FORWARD))
      return;

  /* At this point, we know we should be focusing out of the notebook entirely. We
   * do this by setting a flag, then propagating the focus motion to the notebook.
   */
  toplevel = btk_widget_get_toplevel (BTK_WIDGET (notebook));
  if (!btk_widget_is_toplevel (toplevel))
    return;

  g_object_ref (notebook);
  
  notebook->focus_out = TRUE;
  g_signal_emit_by_name (toplevel, "move-focus", direction_type);
  notebook->focus_out = FALSE;
  
  g_object_unref (notebook);
}

static bint
reorder_tab (BtkNotebook *notebook, GList *position, GList *tab)
{
  GList *elem;

  if (position == tab)
    return g_list_position (notebook->children, tab);

  /* check that we aren't inserting the tab in the
   * same relative position, taking packing into account */
  elem = (position) ? position->prev : g_list_last (notebook->children);

  while (elem && elem != tab && BTK_NOTEBOOK_PAGE (elem)->pack != BTK_NOTEBOOK_PAGE (tab)->pack)
    elem = elem->prev;

  if (elem == tab)
    return g_list_position (notebook->children, tab);

  /* now actually reorder the tab */
  if (notebook->first_tab == tab)
    notebook->first_tab = btk_notebook_search_page (notebook, notebook->first_tab,
						    STEP_NEXT, TRUE);

  notebook->children = g_list_remove_link (notebook->children, tab);

  if (!position)
    elem = g_list_last (notebook->children);
  else
    {
      elem = position->prev;
      position->prev = tab;
    }

  if (elem)
    elem->next = tab;
  else
    notebook->children = tab;

  tab->prev = elem;
  tab->next = position;

  return g_list_position (notebook->children, tab);
}

static bboolean
btk_notebook_reorder_tab (BtkNotebook      *notebook,
			  BtkDirectionType  direction_type,
			  bboolean          move_to_last)
{
  BtkDirectionType effective_direction = get_effective_direction (notebook, direction_type);
  BtkNotebookPage *page;
  GList *last, *child;
  bint page_num;

  if (!btk_widget_is_focus (BTK_WIDGET (notebook)) || !notebook->show_tabs)
    return FALSE;

  if (!notebook->cur_page ||
      !notebook->cur_page->reorderable)
    return FALSE;

  if (effective_direction != BTK_DIR_LEFT &&
      effective_direction != BTK_DIR_RIGHT)
    return FALSE;

  if (move_to_last)
    {
      child = notebook->focus_tab;

      do
	{
	  last = child;
	  child = btk_notebook_search_page (notebook, last, 
					    (effective_direction == BTK_DIR_RIGHT) ? STEP_NEXT : STEP_PREV,
					    TRUE);
	}
      while (child && BTK_NOTEBOOK_PAGE (last)->pack == BTK_NOTEBOOK_PAGE (child)->pack);

      child = last;
    }
  else
    child = btk_notebook_search_page (notebook, notebook->focus_tab,
				      (effective_direction == BTK_DIR_RIGHT) ? STEP_NEXT : STEP_PREV,
				      TRUE);

  if (!child || child->data == notebook->cur_page)
    return FALSE;

  page = child->data;

  if (page->pack == notebook->cur_page->pack)
    {
      if (effective_direction == BTK_DIR_RIGHT)
	page_num = reorder_tab (notebook, (page->pack == BTK_PACK_START) ? child->next : child, notebook->focus_tab);
      else
	page_num = reorder_tab (notebook, (page->pack == BTK_PACK_START) ? child : child->next, notebook->focus_tab);

      btk_notebook_pages_allocate (notebook);

      g_signal_emit (notebook,
		     notebook_signals[PAGE_REORDERED],
		     0,
		     ((BtkNotebookPage *) notebook->focus_tab->data)->child,
		     page_num);

      return TRUE;
    }

  return FALSE;
}

/**
 * btk_notebook_new:
 * 
 * Creates a new #BtkNotebook widget with no pages.

 * Return value: the newly created #BtkNotebook
 **/
BtkWidget*
btk_notebook_new (void)
{
  return g_object_new (BTK_TYPE_NOTEBOOK, NULL);
}

/* Private BtkObject Methods :
 * 
 * btk_notebook_destroy
 * btk_notebook_set_arg
 * btk_notebook_get_arg
 */
static void
btk_notebook_destroy (BtkObject *object)
{
  BtkNotebook *notebook = BTK_NOTEBOOK (object);
  BtkNotebookPrivate *priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);

  if (priv->action_widget[BTK_PACK_START])
    {
      btk_widget_unparent (priv->action_widget[BTK_PACK_START]);
      priv->action_widget[BTK_PACK_START] = NULL;
    }

  if (priv->action_widget[BTK_PACK_END])
    {
      btk_widget_unparent (priv->action_widget[BTK_PACK_END]);
      priv->action_widget[BTK_PACK_END] = NULL;
    }

  if (notebook->menu)
    btk_notebook_popup_disable (notebook);

  if (priv->source_targets)
    {
      btk_target_list_unref (priv->source_targets);
      priv->source_targets = NULL;
    }

  if (priv->switch_tab_timer)
    {
      g_source_remove (priv->switch_tab_timer);
      priv->switch_tab_timer = 0;
    }

  BTK_OBJECT_CLASS (btk_notebook_parent_class)->destroy (object);
}

static void
btk_notebook_set_property (BObject         *object,
			   buint            prop_id,
			   const BValue    *value,
			   BParamSpec      *pspec)
{
  BtkNotebook *notebook;

  notebook = BTK_NOTEBOOK (object);

  switch (prop_id)
    {
    case PROP_SHOW_TABS:
      btk_notebook_set_show_tabs (notebook, b_value_get_boolean (value));
      break;
    case PROP_SHOW_BORDER:
      btk_notebook_set_show_border (notebook, b_value_get_boolean (value));
      break;
    case PROP_SCROLLABLE:
      btk_notebook_set_scrollable (notebook, b_value_get_boolean (value));
      break;
    case PROP_ENABLE_POPUP:
      if (b_value_get_boolean (value))
	btk_notebook_popup_enable (notebook);
      else
	btk_notebook_popup_disable (notebook);
      break;
    case PROP_HOMOGENEOUS:
      btk_notebook_set_homogeneous_tabs_internal (notebook, b_value_get_boolean (value));
      break;  
    case PROP_PAGE:
      btk_notebook_set_current_page (notebook, b_value_get_int (value));
      break;
    case PROP_TAB_POS:
      btk_notebook_set_tab_pos (notebook, b_value_get_enum (value));
      break;
    case PROP_TAB_BORDER:
      btk_notebook_set_tab_border_internal (notebook, b_value_get_uint (value));
      break;
    case PROP_TAB_HBORDER:
      btk_notebook_set_tab_hborder_internal (notebook, b_value_get_uint (value));
      break;
    case PROP_TAB_VBORDER:
      btk_notebook_set_tab_vborder_internal (notebook, b_value_get_uint (value));
      break;
    case PROP_GROUP_ID:
      btk_notebook_set_group_id (notebook, b_value_get_int (value));
      break;
    case PROP_GROUP:
      btk_notebook_set_group (notebook, b_value_get_pointer (value));
      break;
    case PROP_GROUP_NAME:
      btk_notebook_set_group_name (notebook, b_value_get_string (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_notebook_get_property (BObject         *object,
			   buint            prop_id,
			   BValue          *value,
			   BParamSpec      *pspec)
{
  BtkNotebook *notebook;
  BtkNotebookPrivate *priv;

  notebook = BTK_NOTEBOOK (object);
  priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);

  switch (prop_id)
    {
    case PROP_SHOW_TABS:
      b_value_set_boolean (value, notebook->show_tabs);
      break;
    case PROP_SHOW_BORDER:
      b_value_set_boolean (value, notebook->show_border);
      break;
    case PROP_SCROLLABLE:
      b_value_set_boolean (value, notebook->scrollable);
      break;
    case PROP_ENABLE_POPUP:
      b_value_set_boolean (value, notebook->menu != NULL);
      break;
    case PROP_HOMOGENEOUS:
      b_value_set_boolean (value, notebook->homogeneous);
      break;
    case PROP_PAGE:
      b_value_set_int (value, btk_notebook_get_current_page (notebook));
      break;
    case PROP_TAB_POS:
      b_value_set_enum (value, notebook->tab_pos);
      break;
    case PROP_TAB_HBORDER:
      b_value_set_uint (value, notebook->tab_hborder);
      break;
    case PROP_TAB_VBORDER:
      b_value_set_uint (value, notebook->tab_vborder);
      break;
    case PROP_GROUP_ID:
      b_value_set_int (value, btk_notebook_get_group_id (notebook));
      break;
    case PROP_GROUP:
      b_value_set_pointer (value, priv->group);
      break;
    case PROP_GROUP_NAME:
      b_value_set_string (value, btk_notebook_get_group_name (notebook));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* Private BtkWidget Methods :
 * 
 * btk_notebook_map
 * btk_notebook_unmap
 * btk_notebook_realize
 * btk_notebook_size_request
 * btk_notebook_size_allocate
 * btk_notebook_expose
 * btk_notebook_scroll
 * btk_notebook_button_press
 * btk_notebook_button_release
 * btk_notebook_popup_menu
 * btk_notebook_leave_notify
 * btk_notebook_motion_notify
 * btk_notebook_focus_in
 * btk_notebook_focus_out
 * btk_notebook_draw_focus
 * btk_notebook_style_set
 * btk_notebook_drag_begin
 * btk_notebook_drag_end
 * btk_notebook_drag_failed
 * btk_notebook_drag_motion
 * btk_notebook_drag_drop
 * btk_notebook_drag_data_get
 * btk_notebook_drag_data_received
 */
static bboolean
btk_notebook_get_event_window_position (BtkNotebook  *notebook,
					BdkRectangle *rectangle)
{
  BtkNotebookPrivate *priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);
  BtkWidget *widget = BTK_WIDGET (notebook);
  bint border_width = BTK_CONTAINER (notebook)->border_width;
  BtkNotebookPage *visible_page = NULL;
  GList *tmp_list;
  bint tab_pos = get_effective_tab_pos (notebook);
  bboolean is_rtl;
  bint i;

  for (tmp_list = notebook->children; tmp_list; tmp_list = tmp_list->next)
    {
      BtkNotebookPage *page = tmp_list->data;
      if (btk_widget_get_visible (page->child))
	{
	  visible_page = page;
	  break;
	}
    }

  if (notebook->show_tabs && visible_page)
    {
      if (rectangle)
	{
	  is_rtl = btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL;
	  rectangle->x = widget->allocation.x + border_width;
	  rectangle->y = widget->allocation.y + border_width;

	  switch (tab_pos)
	    {
	    case BTK_POS_TOP:
	    case BTK_POS_BOTTOM:
	      rectangle->width = widget->allocation.width - 2 * border_width;
	      rectangle->height = visible_page->requisition.height;
	      if (tab_pos == BTK_POS_BOTTOM)
		rectangle->y += widget->allocation.height - 2 * border_width - rectangle->height;

              for (i = 0; i < N_ACTION_WIDGETS; i++)
                {
                  if (priv->action_widget[i] &&
                      btk_widget_get_visible (priv->action_widget[i]))
                    {
                      rectangle->width -= priv->action_widget[i]->allocation.width;
                      if ((!is_rtl && i == ACTION_WIDGET_START) ||
                          (is_rtl && i == ACTION_WIDGET_END))
                        rectangle->x += priv->action_widget[i]->allocation.width;
                    }
                }
	      break;
	    case BTK_POS_LEFT:
	    case BTK_POS_RIGHT:
	      rectangle->width = visible_page->requisition.width;
	      rectangle->height = widget->allocation.height - 2 * border_width;
	      if (tab_pos == BTK_POS_RIGHT)
		rectangle->x += widget->allocation.width - 2 * border_width - rectangle->width;

              for (i = 0; i < N_ACTION_WIDGETS; i++)
                {
                  if (priv->action_widget[i] &&
                      btk_widget_get_visible (priv->action_widget[i]))
                    {
                      rectangle->height -= priv->action_widget[i]->allocation.height;

                      if (i == ACTION_WIDGET_START)
                        rectangle->y += priv->action_widget[i]->allocation.height;
                    }
                }
              break;
	    }
	}

      return TRUE;
    }
  else
    {
      if (rectangle)
	{
	  rectangle->x = rectangle->y = 0;
	  rectangle->width = rectangle->height = 10;
	}
    }

  return FALSE;
}

static void
btk_notebook_map (BtkWidget *widget)
{
  BtkNotebookPrivate *priv;
  BtkNotebook *notebook;
  BtkNotebookPage *page;
  GList *children;
  bint i;

  btk_widget_set_mapped (widget, TRUE);

  notebook = BTK_NOTEBOOK (widget);
  priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);

  if (notebook->cur_page &&
      btk_widget_get_visible (notebook->cur_page->child) &&
      !btk_widget_get_mapped (notebook->cur_page->child))
    btk_widget_map (notebook->cur_page->child);

  for (i = 0; i < N_ACTION_WIDGETS; i++)
    {
      if (priv->action_widget[i] &&
          btk_widget_get_visible (priv->action_widget[i]) &&
          BTK_WIDGET_CHILD_VISIBLE (priv->action_widget[i]) &&
          !btk_widget_get_mapped (priv->action_widget[i]))
        btk_widget_map (priv->action_widget[i]);
    }

  if (notebook->scrollable)
    btk_notebook_pages_allocate (notebook);
  else
    {
      children = notebook->children;

      while (children)
	{
	  page = children->data;
	  children = children->next;

	  if (page->tab_label &&
	      btk_widget_get_visible (page->tab_label) &&
	      !btk_widget_get_mapped (page->tab_label))
	    btk_widget_map (page->tab_label);
	}
    }

  if (btk_notebook_get_event_window_position (notebook, NULL))
    bdk_window_show_unraised (notebook->event_window);
}

static void
btk_notebook_unmap (BtkWidget *widget)
{
  stop_scrolling (BTK_NOTEBOOK (widget));
  
  btk_widget_set_mapped (widget, FALSE);

  bdk_window_hide (BTK_NOTEBOOK (widget)->event_window);

  BTK_WIDGET_CLASS (btk_notebook_parent_class)->unmap (widget);
}

static void
btk_notebook_realize (BtkWidget *widget)
{
  BtkNotebook *notebook;
  BdkWindowAttr attributes;
  bint attributes_mask;
  BdkRectangle event_window_pos;

  notebook = BTK_NOTEBOOK (widget);

  btk_widget_set_realized (widget, TRUE);

  btk_notebook_get_event_window_position (notebook, &event_window_pos);
  
  widget->window = btk_widget_get_parent_window (widget);
  g_object_ref (widget->window);
  
  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.x = event_window_pos.x;
  attributes.y = event_window_pos.y;
  attributes.width = event_window_pos.width;
  attributes.height = event_window_pos.height;
  attributes.wclass = BDK_INPUT_ONLY;
  attributes.event_mask = btk_widget_get_events (widget);
  attributes.event_mask |= (BDK_BUTTON_PRESS_MASK |
			    BDK_BUTTON_RELEASE_MASK | BDK_KEY_PRESS_MASK |
			    BDK_POINTER_MOTION_MASK | BDK_LEAVE_NOTIFY_MASK |
			    BDK_SCROLL_MASK);
  attributes_mask = BDK_WA_X | BDK_WA_Y;

  notebook->event_window = bdk_window_new (btk_widget_get_parent_window (widget), 
					   &attributes, attributes_mask);
  bdk_window_set_user_data (notebook->event_window, notebook);

  widget->style = btk_style_attach (widget->style, widget->window);
}

static void
btk_notebook_unrealize (BtkWidget *widget)
{
  BtkNotebook *notebook;
  BtkNotebookPrivate *priv;

  notebook = BTK_NOTEBOOK (widget);
  priv = BTK_NOTEBOOK_GET_PRIVATE (widget);

  bdk_window_set_user_data (notebook->event_window, NULL);
  bdk_window_destroy (notebook->event_window);
  notebook->event_window = NULL;

  if (priv->drag_window)
    {
      bdk_window_set_user_data (priv->drag_window, NULL);
      bdk_window_destroy (priv->drag_window);
      priv->drag_window = NULL;
    }

  BTK_WIDGET_CLASS (btk_notebook_parent_class)->unrealize (widget);
}

static void
btk_notebook_size_request (BtkWidget      *widget,
			   BtkRequisition *requisition)
{
  BtkNotebookPrivate *priv = BTK_NOTEBOOK_GET_PRIVATE (widget);
  BtkNotebook *notebook = BTK_NOTEBOOK (widget);
  BtkNotebookPage *page;
  GList *children;
  BtkRequisition child_requisition;
  BtkRequisition action_widget_requisition[2] = { { 0 }, { 0 } };
  bboolean switch_page = FALSE;
  bint vis_pages;
  bint focus_width;
  bint tab_overlap;
  bint tab_curvature;
  bint arrow_spacing;
  bint scroll_arrow_hlength;
  bint scroll_arrow_vlength;

  btk_widget_style_get (widget,
                        "focus-line-width", &focus_width,
			"tab-overlap", &tab_overlap,
			"tab-curvature", &tab_curvature,
                        "arrow-spacing", &arrow_spacing,
                        "scroll-arrow-hlength", &scroll_arrow_hlength,
                        "scroll-arrow-vlength", &scroll_arrow_vlength,
			NULL);

  widget->requisition.width = 0;
  widget->requisition.height = 0;

  for (children = notebook->children, vis_pages = 0; children;
       children = children->next)
    {
      page = children->data;

      if (btk_widget_get_visible (page->child))
	{
	  vis_pages++;
	  btk_widget_size_request (page->child, &child_requisition);
	  
	  widget->requisition.width = MAX (widget->requisition.width,
					   child_requisition.width);
	  widget->requisition.height = MAX (widget->requisition.height,
					    child_requisition.height);

	  if (notebook->menu && page->menu_label->parent &&
	      !btk_widget_get_visible (page->menu_label->parent))
	    btk_widget_show (page->menu_label->parent);
	}
      else
	{
	  if (page == notebook->cur_page)
	    switch_page = TRUE;
	  if (notebook->menu && page->menu_label->parent &&
	      btk_widget_get_visible (page->menu_label->parent))
	    btk_widget_hide (page->menu_label->parent);
	}
    }

  if (notebook->show_border || notebook->show_tabs)
    {
      widget->requisition.width += widget->style->xthickness * 2;
      widget->requisition.height += widget->style->ythickness * 2;

      if (notebook->show_tabs)
	{
	  bint tab_width = 0;
	  bint tab_height = 0;
	  bint tab_max = 0;
	  bint padding;
          bint i;
          bint action_width = 0;
          bint action_height = 0;
	  
	  for (children = notebook->children; children;
	       children = children->next)
	    {
	      page = children->data;
	      
	      if (btk_widget_get_visible (page->child))
		{
		  if (!btk_widget_get_visible (page->tab_label))
		    btk_widget_show (page->tab_label);

		  btk_widget_size_request (page->tab_label,
					   &child_requisition);

		  page->requisition.width = 
		    child_requisition.width +
		    2 * widget->style->xthickness;
		  page->requisition.height = 
		    child_requisition.height +
		    2 * widget->style->ythickness;
		  
		  switch (notebook->tab_pos)
		    {
		    case BTK_POS_TOP:
		    case BTK_POS_BOTTOM:
		      page->requisition.height += 2 * (notebook->tab_vborder +
						       focus_width);
		      tab_height = MAX (tab_height, page->requisition.height);
		      tab_max = MAX (tab_max, page->requisition.width);
		      break;
		    case BTK_POS_LEFT:
		    case BTK_POS_RIGHT:
		      page->requisition.width += 2 * (notebook->tab_hborder +
						      focus_width);
		      tab_width = MAX (tab_width, page->requisition.width);
		      tab_max = MAX (tab_max, page->requisition.height);
		      break;
		    }
		}
	      else if (btk_widget_get_visible (page->tab_label))
		btk_widget_hide (page->tab_label);
	    }

	  children = notebook->children;

	  if (vis_pages)
	    {
              for (i = 0; i < N_ACTION_WIDGETS; i++)
                {
                  if (priv->action_widget[i])
                    {
                      btk_widget_size_request (priv->action_widget[i], &action_widget_requisition[i]);
                      action_widget_requisition[i].width += widget->style->xthickness;
                      action_widget_requisition[i].height += widget->style->ythickness;
                    }
                }

	      switch (notebook->tab_pos)
		{
		case BTK_POS_TOP:
		case BTK_POS_BOTTOM:
		  if (tab_height == 0)
		    break;

		  if (notebook->scrollable && vis_pages > 1 && 
		      widget->requisition.width < tab_width)
		    tab_height = MAX (tab_height, scroll_arrow_hlength);

                  tab_height = MAX (tab_height, action_widget_requisition[ACTION_WIDGET_START].height);
                  tab_height = MAX (tab_height, action_widget_requisition[ACTION_WIDGET_END].height);

		  padding = 2 * (tab_curvature + focus_width +
				 notebook->tab_hborder) - tab_overlap;
		  tab_max += padding;
		  while (children)
		    {
		      page = children->data;
		      children = children->next;
		  
		      if (!btk_widget_get_visible (page->child))
			continue;

		      if (notebook->homogeneous)
			page->requisition.width = tab_max;
		      else
			page->requisition.width += padding;

		      tab_width += page->requisition.width;
		      page->requisition.height = tab_height;
		    }

		  if (notebook->scrollable && vis_pages > 1 &&
		      widget->requisition.width < tab_width)
		    tab_width = tab_max + 2 * (scroll_arrow_hlength + arrow_spacing);

		  action_width += action_widget_requisition[ACTION_WIDGET_START].width;
		  action_width += action_widget_requisition[ACTION_WIDGET_END].width;
                  if (notebook->homogeneous && !notebook->scrollable)
                    widget->requisition.width = MAX (widget->requisition.width,
                                                     vis_pages * tab_max +
                                                     tab_overlap + action_width);
                  else
                    widget->requisition.width = MAX (widget->requisition.width,
                                                     tab_width + tab_overlap + action_width);

		  widget->requisition.height += tab_height;
		  break;
		case BTK_POS_LEFT:
		case BTK_POS_RIGHT:
		  if (tab_width == 0)
		    break;

		  if (notebook->scrollable && vis_pages > 1 && 
		      widget->requisition.height < tab_height)
		    tab_width = MAX (tab_width,
                                     arrow_spacing + 2 * scroll_arrow_vlength);

		  tab_width = MAX (tab_width, action_widget_requisition[ACTION_WIDGET_START].width);
		  tab_width = MAX (tab_width, action_widget_requisition[ACTION_WIDGET_END].width);

		  padding = 2 * (tab_curvature + focus_width +
				 notebook->tab_vborder) - tab_overlap;
		  tab_max += padding;

		  while (children)
		    {
		      page = children->data;
		      children = children->next;

		      if (!btk_widget_get_visible (page->child))
			continue;

		      page->requisition.width = tab_width;

		      if (notebook->homogeneous)
			page->requisition.height = tab_max;
		      else
			page->requisition.height += padding;

		      tab_height += page->requisition.height;
		    }

		  if (notebook->scrollable && vis_pages > 1 && 
		      widget->requisition.height < tab_height)
		    tab_height = tab_max + (2 * scroll_arrow_vlength + arrow_spacing);
		  action_height += action_widget_requisition[ACTION_WIDGET_START].height;
		  action_height += action_widget_requisition[ACTION_WIDGET_END].height;

                  if (notebook->homogeneous && !notebook->scrollable)
                    widget->requisition.height =
		      MAX (widget->requisition.height,
			   vis_pages * tab_max + tab_overlap + action_height);
                  else
                    widget->requisition.height =
		      MAX (widget->requisition.height,
			   tab_height + tab_overlap + action_height);

		  if (!notebook->homogeneous || notebook->scrollable)
		    vis_pages = 1;
		  widget->requisition.height = MAX (widget->requisition.height,
						    vis_pages * tab_max +
						    tab_overlap);

		  widget->requisition.width += tab_width;
		  break;
		}
	    }
	}
      else
	{
	  for (children = notebook->children; children;
	       children = children->next)
	    {
	      page = children->data;
	      
	      if (page->tab_label && btk_widget_get_visible (page->tab_label))
		btk_widget_hide (page->tab_label);
	    }
	}
    }

  widget->requisition.width += BTK_CONTAINER (widget)->border_width * 2;
  widget->requisition.height += BTK_CONTAINER (widget)->border_width * 2;

  if (switch_page)
    {
      if (vis_pages)
	{
	  for (children = notebook->children; children;
	       children = children->next)
	    {
	      page = children->data;
	      if (btk_widget_get_visible (page->child))
		{
		  btk_notebook_switch_page (notebook, page);
		  break;
		}
	    }
	}
      else if (btk_widget_get_visible (widget))
	{
	  widget->requisition.width = BTK_CONTAINER (widget)->border_width * 2;
	  widget->requisition.height= BTK_CONTAINER (widget)->border_width * 2;
	}
    }
  if (vis_pages && !notebook->cur_page)
    {
      children = btk_notebook_search_page (notebook, NULL, STEP_NEXT, TRUE);
      if (children)
	{
	  notebook->first_tab = children;
	  btk_notebook_switch_page (notebook, BTK_NOTEBOOK_PAGE (children));
	}
    }
}

static void
btk_notebook_size_allocate (BtkWidget     *widget,
			    BtkAllocation *allocation)
{
  BtkNotebookPrivate *priv = BTK_NOTEBOOK_GET_PRIVATE (widget);
  BtkNotebook *notebook = BTK_NOTEBOOK (widget);
  bint tab_pos = get_effective_tab_pos (notebook);
  bboolean is_rtl;
  bint focus_width;

  btk_widget_style_get (widget, "focus-line-width", &focus_width, NULL);
  
  widget->allocation = *allocation;
  if (btk_widget_get_realized (widget))
    {
      BdkRectangle position;

      if (btk_notebook_get_event_window_position (notebook, &position))
	{
	  bdk_window_move_resize (notebook->event_window,
				  position.x, position.y,
				  position.width, position.height);
	  if (btk_widget_get_mapped (BTK_WIDGET (notebook)))
	    bdk_window_show_unraised (notebook->event_window);
	}
      else
	bdk_window_hide (notebook->event_window);
    }

  if (notebook->children)
    {
      bint border_width = BTK_CONTAINER (widget)->border_width;
      BtkNotebookPage *page;
      BtkAllocation child_allocation;
      GList *children;
      bint i;
      
      child_allocation.x = widget->allocation.x + border_width;
      child_allocation.y = widget->allocation.y + border_width;
      child_allocation.width = MAX (1, allocation->width - border_width * 2);
      child_allocation.height = MAX (1, allocation->height - border_width * 2);

      if (notebook->show_tabs || notebook->show_border)
	{
	  child_allocation.x += widget->style->xthickness;
	  child_allocation.y += widget->style->ythickness;
	  child_allocation.width = MAX (1, child_allocation.width -
					widget->style->xthickness * 2);
	  child_allocation.height = MAX (1, child_allocation.height -
					 widget->style->ythickness * 2);

	  if (notebook->show_tabs && notebook->children && notebook->cur_page)
	    {
	      switch (tab_pos)
		{
		case BTK_POS_TOP:
		  child_allocation.y += notebook->cur_page->requisition.height;
		case BTK_POS_BOTTOM:
		  child_allocation.height =
		    MAX (1, child_allocation.height -
			 notebook->cur_page->requisition.height);
		  break;
		case BTK_POS_LEFT:
		  child_allocation.x += notebook->cur_page->requisition.width;
		case BTK_POS_RIGHT:
		  child_allocation.width =
		    MAX (1, child_allocation.width -
			 notebook->cur_page->requisition.width);
		  break;
		}

              for (i = 0; i < N_ACTION_WIDGETS; i++)
                {
                  BtkAllocation widget_allocation;

                  if (!priv->action_widget[i])
                    continue;

		  widget_allocation.x = widget->allocation.x + border_width;
		  widget_allocation.y = widget->allocation.y + border_width;
		  is_rtl = btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL;

		  switch (tab_pos)
		    {
		    case BTK_POS_BOTTOM:
		      widget_allocation.y +=
			widget->allocation.height - 2 * border_width - notebook->cur_page->requisition.height;
		      /* fall through */
		    case BTK_POS_TOP:
		      widget_allocation.width = priv->action_widget[i]->requisition.width;
		      widget_allocation.height = notebook->cur_page->requisition.height - widget->style->ythickness;

		      if ((i == ACTION_WIDGET_START && is_rtl) ||
                          (i == ACTION_WIDGET_END && !is_rtl))
			widget_allocation.x +=
			  widget->allocation.width - 2 * border_width -
			  priv->action_widget[i]->requisition.width;
                      if (tab_pos == BTK_POS_TOP) /* no fall through */
                          widget_allocation.y += 2 * focus_width;
		      break;
		    case BTK_POS_RIGHT:
		      widget_allocation.x +=
			widget->allocation.width - 2 * border_width - notebook->cur_page->requisition.width;
		      /* fall through */
		    case BTK_POS_LEFT:
		      widget_allocation.height = priv->action_widget[i]->requisition.height;
		      widget_allocation.width = notebook->cur_page->requisition.width - widget->style->xthickness;

                      if (i == ACTION_WIDGET_END)
                        widget_allocation.y +=
                          widget->allocation.height - 2 * border_width -
                          priv->action_widget[i]->requisition.height;
                      if (tab_pos == BTK_POS_LEFT) /* no fall through */  
                        widget_allocation.x += 2 * focus_width;
		      break;
		    }

		  btk_widget_size_allocate (priv->action_widget[i], &widget_allocation);
		}
	    }
	}

      children = notebook->children;
      while (children)
	{
	  page = children->data;
	  children = children->next;
	  
	  if (btk_widget_get_visible (page->child))
	    btk_widget_size_allocate (page->child, &child_allocation);
	}

      btk_notebook_pages_allocate (notebook);
    }
}

static bint
btk_notebook_expose (BtkWidget      *widget,
		     BdkEventExpose *event)
{
  BtkNotebook *notebook;
  BtkNotebookPrivate *priv;
  bint i;

  notebook = BTK_NOTEBOOK (widget);
  priv = BTK_NOTEBOOK_GET_PRIVATE (widget);

  if (event->window == priv->drag_window)
    {
      BdkRectangle area = { 0, };
      bairo_t *cr;

      /* FIXME: This is a workaround to make tabs reordering work better
       * with engines with rounded tabs. If the drag window background
       * isn't set, the rounded corners would be black.
       *
       * Ideally, these corners should be made transparent, Either by using
       * ARGB visuals or shape windows.
       */
      cr = bdk_bairo_create (priv->drag_window);
      bdk_bairo_set_source_color (cr, &widget->style->bg [BTK_STATE_NORMAL]);
      bairo_paint (cr);
      bairo_destroy (cr);

      area.width = bdk_window_get_width (priv->drag_window);
      area.height = bdk_window_get_height (priv->drag_window);
      btk_notebook_draw_tab (notebook,
			     notebook->cur_page,
			     &area);
      btk_notebook_draw_focus (widget, event);
      btk_container_propagate_expose (BTK_CONTAINER (notebook),
				      notebook->cur_page->tab_label, event);
    }
  else if (btk_widget_is_drawable (widget))
    {
      btk_notebook_paint (widget, &event->area);
      if (notebook->show_tabs)
	{
	  BtkNotebookPage *page;
	  GList *pages;

	  btk_notebook_draw_focus (widget, event);
	  pages = notebook->children;

	  while (pages)
	    {
	      page = BTK_NOTEBOOK_PAGE (pages);
	      pages = pages->next;

	      if (page->tab_label->window == event->window &&
		  btk_widget_is_drawable (page->tab_label))
		btk_container_propagate_expose (BTK_CONTAINER (notebook),
						page->tab_label, event);
	    }
	}

      if (notebook->cur_page)
	btk_container_propagate_expose (BTK_CONTAINER (notebook),
					notebook->cur_page->child,
					event);
      if (notebook->show_tabs)
      {
        for (i = 0; i < N_ACTION_WIDGETS; i++)
        {
          if (priv->action_widget[i] &&
              btk_widget_is_drawable (priv->action_widget[i]))
            btk_container_propagate_expose (BTK_CONTAINER (notebook),
                                            priv->action_widget[i], event);
        }
      }
    }

  return FALSE;
}

static bboolean
btk_notebook_show_arrows (BtkNotebook *notebook)
{
  bboolean show_arrow = FALSE;
  GList *children;
  
  if (!notebook->scrollable)
    return FALSE;

  children = notebook->children;
  while (children)
    {
      BtkNotebookPage *page = children->data;

      if (page->tab_label && !btk_widget_get_child_visible (page->tab_label))
	show_arrow = TRUE;

      children = children->next;
    }

  return show_arrow;
}

static void
btk_notebook_get_arrow_rect (BtkNotebook     *notebook,
			     BdkRectangle    *rectangle,
			     BtkNotebookArrow arrow)
{
  BdkRectangle event_window_pos;
  bboolean before = ARROW_IS_BEFORE (arrow);
  bboolean left = ARROW_IS_LEFT (arrow);

  if (btk_notebook_get_event_window_position (notebook, &event_window_pos))
    {
      bint scroll_arrow_hlength;
      bint scroll_arrow_vlength;

      btk_widget_style_get (BTK_WIDGET (notebook),
                            "scroll-arrow-hlength", &scroll_arrow_hlength,
                            "scroll-arrow-vlength", &scroll_arrow_vlength,
                            NULL);

      switch (notebook->tab_pos)
	{
	case BTK_POS_LEFT:
	case BTK_POS_RIGHT:
          rectangle->width = scroll_arrow_vlength;
          rectangle->height = scroll_arrow_vlength;

	  if ((before && (notebook->has_before_previous != notebook->has_before_next)) ||
	      (!before && (notebook->has_after_previous != notebook->has_after_next))) 
	  rectangle->x = event_window_pos.x + (event_window_pos.width - rectangle->width) / 2;
	  else if (left)
	    rectangle->x = event_window_pos.x + event_window_pos.width / 2 - rectangle->width;
	  else 
	    rectangle->x = event_window_pos.x + event_window_pos.width / 2;
	  rectangle->y = event_window_pos.y;
	  if (!before)
	    rectangle->y += event_window_pos.height - rectangle->height;
	  break;

	case BTK_POS_TOP:
	case BTK_POS_BOTTOM:
          rectangle->width = scroll_arrow_hlength;
          rectangle->height = scroll_arrow_hlength;

	  if (before)
	    {
	      if (left || !notebook->has_before_previous)
		rectangle->x = event_window_pos.x;
	      else
		rectangle->x = event_window_pos.x + rectangle->width;
	    }
	  else
	    {
	      if (!left || !notebook->has_after_next)
		rectangle->x = event_window_pos.x + event_window_pos.width - rectangle->width;
	      else
		rectangle->x = event_window_pos.x + event_window_pos.width - 2 * rectangle->width;
	    }
	  rectangle->y = event_window_pos.y + (event_window_pos.height - rectangle->height) / 2;
	  break;
	}
    }
}

static BtkNotebookArrow
btk_notebook_get_arrow (BtkNotebook *notebook,
			bint         x,
			bint         y)
{
  BdkRectangle arrow_rect;
  BdkRectangle event_window_pos;
  bint i;
  bint x0, y0;
  BtkNotebookArrow arrow[4];

  arrow[0] = notebook->has_before_previous ? ARROW_LEFT_BEFORE : ARROW_NONE;
  arrow[1] = notebook->has_before_next ? ARROW_RIGHT_BEFORE : ARROW_NONE;
  arrow[2] = notebook->has_after_previous ? ARROW_LEFT_AFTER : ARROW_NONE;
  arrow[3] = notebook->has_after_next ? ARROW_RIGHT_AFTER : ARROW_NONE;

  if (btk_notebook_show_arrows (notebook))
    {
      btk_notebook_get_event_window_position (notebook, &event_window_pos);
      for (i = 0; i < 4; i++) 
	{ 
	  if (arrow[i] == ARROW_NONE)
	    continue;
      
	  btk_notebook_get_arrow_rect (notebook, &arrow_rect, arrow[i]);
      
	  x0 = x - arrow_rect.x;
	  y0 = y - arrow_rect.y;

	  if (y0 >= 0 && y0 < arrow_rect.height &&
	      x0 >= 0 && x0 < arrow_rect.width)
	    return arrow[i];
	}
    }

  return ARROW_NONE;
}

static void
btk_notebook_do_arrow (BtkNotebook     *notebook,
		       BtkNotebookArrow arrow)
{
  BtkWidget *widget = BTK_WIDGET (notebook);
  bboolean is_rtl, left;

  is_rtl = btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL;
  left = (ARROW_IS_LEFT (arrow) && !is_rtl) || 
         (!ARROW_IS_LEFT (arrow) && is_rtl);

  if (!notebook->focus_tab ||
      btk_notebook_search_page (notebook, notebook->focus_tab,
				left ? STEP_PREV : STEP_NEXT,
				TRUE))
    {
      btk_notebook_change_current_page (notebook, left ? -1 : 1);
      btk_widget_grab_focus (widget);
    }
}

static bboolean
btk_notebook_arrow_button_press (BtkNotebook      *notebook,
				 BtkNotebookArrow  arrow,
				 bint              button)
{
  BtkWidget *widget = BTK_WIDGET (notebook);
  bboolean is_rtl = btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL;
  bboolean left = (ARROW_IS_LEFT (arrow) && !is_rtl) || 
                  (!ARROW_IS_LEFT (arrow) && is_rtl);

  if (!btk_widget_has_focus (widget))
    btk_widget_grab_focus (widget);
  
  notebook->button = button;
  notebook->click_child = arrow;

  if (button == 1)
    {
      btk_notebook_do_arrow (notebook, arrow);
      btk_notebook_set_scroll_timer (notebook);
    }
  else if (button == 2)
    btk_notebook_page_select (notebook, TRUE);
  else if (button == 3)
    btk_notebook_switch_focus_tab (notebook,
				   btk_notebook_search_page (notebook,
							     NULL,
							     left ? STEP_NEXT : STEP_PREV,
							     TRUE));
  btk_notebook_redraw_arrows (notebook);

  return TRUE;
}

static bboolean
get_widget_coordinates (BtkWidget *widget,
			BdkEvent  *event,
			bint      *x,
			bint      *y)
{
  BdkWindow *window = ((BdkEventAny *)event)->window;
  bdouble tx, ty;

  if (!bdk_event_get_coords (event, &tx, &ty))
    return FALSE;

  while (window && window != widget->window)
    {
      bint window_x, window_y;

      bdk_window_get_position (window, &window_x, &window_y);
      tx += window_x;
      ty += window_y;

      window = bdk_window_get_parent (window);
    }

  if (window)
    {
      *x = tx;
      *y = ty;

      return TRUE;
    }
  else
    return FALSE;
}

static bboolean
btk_notebook_scroll (BtkWidget      *widget,
                     BdkEventScroll *event)
{
  BtkNotebookPrivate *priv = BTK_NOTEBOOK_GET_PRIVATE (widget);
  BtkNotebook *notebook = BTK_NOTEBOOK (widget);
  BtkWidget *child, *event_widget;
  bint i;

  if (!notebook->cur_page)
    return FALSE;

  child = notebook->cur_page->child;
  event_widget = btk_get_event_widget ((BdkEvent *)event);

  /* ignore scroll events from the content of the page */
  if (!event_widget || btk_widget_is_ancestor (event_widget, child) || event_widget == child)
    return FALSE;

  /* nor from the action area */
  for (i = 0; i < 2; i++)
    {
      if (event_widget == priv->action_widget[i] ||
          (priv->action_widget[i] &&
           btk_widget_is_ancestor (event_widget, priv->action_widget[i])))
        return FALSE;
    }

  switch (event->direction)
    {
    case BDK_SCROLL_RIGHT:
    case BDK_SCROLL_DOWN:
      btk_notebook_next_page (notebook);
      break;
    case BDK_SCROLL_LEFT:
    case BDK_SCROLL_UP:
      btk_notebook_prev_page (notebook);
      break;
    }

  return TRUE;
}

static GList*
get_tab_at_pos (BtkNotebook *notebook, bint x, bint y)
{
  BtkNotebookPage *page;
  GList *children = notebook->children;

  while (children)
    {
      page = children->data;
      
      if (btk_widget_get_visible (page->child) &&
	  page->tab_label && btk_widget_get_mapped (page->tab_label) &&
	  (x >= page->allocation.x) &&
	  (y >= page->allocation.y) &&
	  (x <= (page->allocation.x + page->allocation.width)) &&
	  (y <= (page->allocation.y + page->allocation.height)))
	return children;

      children = children->next;
    }

  return NULL;
}

static bboolean
btk_notebook_button_press (BtkWidget      *widget,
			   BdkEventButton *event)
{
  BtkNotebook *notebook = BTK_NOTEBOOK (widget);
  BtkNotebookPrivate *priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);
  BtkNotebookPage *page;
  GList *tab;
  BtkNotebookArrow arrow;
  bint x, y;

  if (event->type != BDK_BUTTON_PRESS || !notebook->children ||
      notebook->button)
    return FALSE;

  if (!get_widget_coordinates (widget, (BdkEvent *)event, &x, &y))
    return FALSE;

  arrow = btk_notebook_get_arrow (notebook, x, y);
  if (arrow)
    return btk_notebook_arrow_button_press (notebook, arrow, event->button);

  if (notebook->menu && _btk_button_event_triggers_context_menu (event))
    {
      btk_menu_popup (BTK_MENU (notebook->menu), NULL, NULL, 
		      NULL, NULL, 3, event->time);
      return TRUE;
    }

  if (event->button != 1)
    return FALSE;

  notebook->button = event->button;

  if ((tab = get_tab_at_pos (notebook, x, y)) != NULL)
    {
      bboolean page_changed, was_focus;

      page = tab->data;
      page_changed = page != notebook->cur_page;
      was_focus = btk_widget_is_focus (widget);

      btk_notebook_switch_focus_tab (notebook, tab);
      btk_widget_grab_focus (widget);

      if (page_changed && !was_focus)
	btk_widget_child_focus (page->child, BTK_DIR_TAB_FORWARD);

      /* save press to possibly begin a drag */
      if (page->reorderable || page->detachable)
	{
	  priv->during_detach = FALSE;
	  priv->during_reorder = FALSE;
	  priv->pressed_button = event->button;

	  priv->mouse_x = x;
	  priv->mouse_y = y;

	  priv->drag_begin_x = priv->mouse_x;
	  priv->drag_begin_y = priv->mouse_y;
	  priv->drag_offset_x = priv->drag_begin_x - page->allocation.x;
	  priv->drag_offset_y = priv->drag_begin_y - page->allocation.y;
	}
    }

  return TRUE;
}

static void
popup_position_func (BtkMenu  *menu,
                     bint     *x,
                     bint     *y,
                     bboolean *push_in,
                     bpointer  data)
{
  BtkNotebook *notebook = data;
  BtkWidget *w;
  BtkRequisition requisition;

  if (notebook->focus_tab)
    {
      BtkNotebookPage *page;

      page = notebook->focus_tab->data;
      w = page->tab_label;
    }
  else
   {
     w = BTK_WIDGET (notebook);
   }

  bdk_window_get_origin (w->window, x, y);
  btk_widget_size_request (BTK_WIDGET (menu), &requisition);

  if (btk_widget_get_direction (w) == BTK_TEXT_DIR_RTL)
    *x += w->allocation.x + w->allocation.width - requisition.width;
  else
    *x += w->allocation.x;

  *y += w->allocation.y + w->allocation.height;

  *push_in = FALSE;
}

static bboolean
btk_notebook_popup_menu (BtkWidget *widget)
{
  BtkNotebook *notebook = BTK_NOTEBOOK (widget);

  if (notebook->menu)
    {
      btk_menu_popup (BTK_MENU (notebook->menu), NULL, NULL, 
		      popup_position_func, notebook,
		      0, btk_get_current_event_time ());
      btk_menu_shell_select_first (BTK_MENU_SHELL (notebook->menu), FALSE);
      return TRUE;
    }

  return FALSE;
}

static void 
stop_scrolling (BtkNotebook *notebook)
{
  if (notebook->timer)
    {
      g_source_remove (notebook->timer);
      notebook->timer = 0;
      notebook->need_timer = FALSE;
    }
  notebook->click_child = 0;
  notebook->button = 0;
  btk_notebook_redraw_arrows (notebook);
}

static GList*
get_drop_position (BtkNotebook *notebook,
		   buint        pack)
{
  BtkNotebookPrivate *priv;
  GList *children, *last_child;
  BtkNotebookPage *page;
  bboolean is_rtl;
  bint x, y;

  priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);
  x = priv->mouse_x;
  y = priv->mouse_y;

  is_rtl = btk_widget_get_direction ((BtkWidget *) notebook) == BTK_TEXT_DIR_RTL;
  children = notebook->children;
  last_child = NULL;

  while (children)
    {
      page = children->data;

      if ((priv->operation != DRAG_OPERATION_REORDER || page != notebook->cur_page) &&
	  btk_widget_get_visible (page->child) &&
	  page->tab_label &&
	  btk_widget_get_mapped (page->tab_label) &&
	  page->pack == pack)
	{
	  switch (notebook->tab_pos)
	    {
	    case BTK_POS_TOP:
	    case BTK_POS_BOTTOM:
	      if (!is_rtl)
		{
		  if ((page->pack == BTK_PACK_START && PAGE_MIDDLE_X (page) > x) ||
		      (page->pack == BTK_PACK_END && PAGE_MIDDLE_X (page) < x))
		    return children;
		}
	      else
		{
		  if ((page->pack == BTK_PACK_START && PAGE_MIDDLE_X (page) < x) ||
		      (page->pack == BTK_PACK_END && PAGE_MIDDLE_X (page) > x))
		    return children;
		}

	      break;
	    case BTK_POS_LEFT:
	    case BTK_POS_RIGHT:
	      if ((page->pack == BTK_PACK_START && PAGE_MIDDLE_Y (page) > y) ||
		  (page->pack == BTK_PACK_END && PAGE_MIDDLE_Y (page) < y))
		return children;

	      break;
	    }

	  last_child = children->next;
	}

      children = children->next;
    }

  return last_child;
}

static void
show_drag_window (BtkNotebook        *notebook,
		  BtkNotebookPrivate *priv,
		  BtkNotebookPage    *page)
{
  BtkWidget *widget = BTK_WIDGET (notebook);

  if (!priv->drag_window)
    {
      BdkWindowAttr attributes;
      buint attributes_mask;

      attributes.x = page->allocation.x;
      attributes.y = page->allocation.y;
      attributes.width = page->allocation.width;
      attributes.height = page->allocation.height;
      attributes.window_type = BDK_WINDOW_CHILD;
      attributes.wclass = BDK_INPUT_OUTPUT;
      attributes.visual = btk_widget_get_visual (widget);
      attributes.colormap = btk_widget_get_colormap (widget);
      attributes.event_mask = BDK_VISIBILITY_NOTIFY_MASK | BDK_EXPOSURE_MASK | BDK_POINTER_MOTION_MASK;
      attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;

      priv->drag_window = bdk_window_new (btk_widget_get_parent_window (widget),
					  &attributes,
					  attributes_mask);
      bdk_window_set_user_data (priv->drag_window, widget);
    }

  g_object_ref (page->tab_label);
  btk_widget_unparent (page->tab_label);
  btk_widget_set_parent_window (page->tab_label, priv->drag_window);
  btk_widget_set_parent (page->tab_label, widget);
  g_object_unref (page->tab_label);

  bdk_window_show (priv->drag_window);

  /* the grab will dissapear when the window is hidden */
  bdk_pointer_grab (priv->drag_window,
		    FALSE,
		    BDK_POINTER_MOTION_MASK | BDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, BDK_CURRENT_TIME);
}

/* This function undoes the reparenting that happens both when drag_window
 * is shown for reordering and when the DnD icon is shown for detaching
 */
static void
hide_drag_window (BtkNotebook        *notebook,
		  BtkNotebookPrivate *priv,
		  BtkNotebookPage    *page)
{
  BtkWidget *widget = BTK_WIDGET (notebook);
  BtkWidget *parent = page->tab_label->parent;

  if (page->tab_label->window != widget->window ||
      !NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page))
    {
      g_object_ref (page->tab_label);

      if (BTK_IS_WINDOW (parent))
	{
	  /* parent widget is the drag window */
	  btk_container_remove (BTK_CONTAINER (parent), page->tab_label);
	}
      else
	btk_widget_unparent (page->tab_label);

      btk_widget_set_parent (page->tab_label, widget);
      g_object_unref (page->tab_label);
    }

  if (priv->drag_window &&
      bdk_window_is_visible (priv->drag_window))
    bdk_window_hide (priv->drag_window);
}

static void
btk_notebook_stop_reorder (BtkNotebook *notebook)
{
  BtkNotebookPrivate *priv;
  BtkNotebookPage *page;

  priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);

  if (priv->operation == DRAG_OPERATION_DETACH)
    page = priv->detached_tab;
  else
    page = notebook->cur_page;

  if (!page || !page->tab_label)
    return;

  priv->pressed_button = -1;

  if (page->reorderable || page->detachable)
    {
      if (priv->during_reorder)
	{
	  bint old_page_num, page_num;
	  GList *element;

	  element = get_drop_position (notebook, page->pack);
	  old_page_num = g_list_position (notebook->children, notebook->focus_tab);
	  page_num = reorder_tab (notebook, element, notebook->focus_tab);
          btk_notebook_child_reordered (notebook, page);
          
	  if (priv->has_scrolled || old_page_num != page_num)
	    g_signal_emit (notebook,
			   notebook_signals[PAGE_REORDERED], 0,
			   page->child, page_num);

	  priv->has_scrolled = FALSE;
          priv->during_reorder = FALSE; 
	}

      hide_drag_window (notebook, priv, page);

      priv->operation = DRAG_OPERATION_NONE;
      btk_notebook_pages_allocate (notebook);

      if (priv->dnd_timer)
	{
	  g_source_remove (priv->dnd_timer);
	  priv->dnd_timer = 0;
	}
    }
}

static bint
btk_notebook_button_release (BtkWidget      *widget,
			     BdkEventButton *event)
{
  BtkNotebook *notebook;
  BtkNotebookPrivate *priv;
  BtkNotebookPage *page;

  if (event->type != BDK_BUTTON_RELEASE)
    return FALSE;

  notebook = BTK_NOTEBOOK (widget);
  priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);
  page = notebook->cur_page;

  if (!priv->during_detach &&
      page->reorderable &&
      event->button == priv->pressed_button)
    btk_notebook_stop_reorder (notebook);

  if (event->button == notebook->button)
    {
      stop_scrolling (notebook);
      return TRUE;
    }
  else
    return FALSE;
}

static bint
btk_notebook_leave_notify (BtkWidget        *widget,
			   BdkEventCrossing *event)
{
  BtkNotebook *notebook = BTK_NOTEBOOK (widget);
  bint x, y;

  if (!get_widget_coordinates (widget, (BdkEvent *)event, &x, &y))
    return FALSE;

  if (notebook->in_child)
    {
      notebook->in_child = 0;
      btk_notebook_redraw_arrows (notebook);
    }

  return TRUE;
}

static BtkNotebookPointerPosition
get_pointer_position (BtkNotebook *notebook)
{
  BtkWidget *widget = (BtkWidget *) notebook;
  BtkNotebookPrivate *priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);
  bint wx, wy, width, height;
  bboolean is_rtl;

  if (!notebook->scrollable)
    return POINTER_BETWEEN;

  bdk_window_get_position (notebook->event_window, &wx, &wy);
  width = bdk_window_get_width (notebook->event_window);
  height = bdk_window_get_height (notebook->event_window);

  if (notebook->tab_pos == BTK_POS_TOP ||
      notebook->tab_pos == BTK_POS_BOTTOM)
    {
      bint x;

      is_rtl = btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL;
      x = priv->mouse_x - wx;

      if (x > width - SCROLL_THRESHOLD)
	return (is_rtl) ? POINTER_BEFORE : POINTER_AFTER;
      else if (x < SCROLL_THRESHOLD)
	return (is_rtl) ? POINTER_AFTER : POINTER_BEFORE;
      else
	return POINTER_BETWEEN;
    }
  else
    {
      bint y;

      y = priv->mouse_y - wy;
      if (y > height - SCROLL_THRESHOLD)
	return POINTER_AFTER;
      else if (y < SCROLL_THRESHOLD)
	return POINTER_BEFORE;
      else
	return POINTER_BETWEEN;
    }
}

static bboolean
scroll_notebook_timer (bpointer data)
{
  BtkNotebook *notebook = (BtkNotebook *) data;
  BtkNotebookPrivate *priv;
  BtkNotebookPointerPosition pointer_position;
  GList *element, *first_tab;

  priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);
  pointer_position = get_pointer_position (notebook);

  element = get_drop_position (notebook, notebook->cur_page->pack);
  reorder_tab (notebook, element, notebook->focus_tab);
  first_tab = btk_notebook_search_page (notebook, notebook->first_tab,
					(pointer_position == POINTER_BEFORE) ? STEP_PREV : STEP_NEXT,
					TRUE);
  if (first_tab)
    {
      notebook->first_tab = first_tab;
      btk_notebook_pages_allocate (notebook);

      bdk_window_move_resize (priv->drag_window,
			      priv->drag_window_x,
			      priv->drag_window_y,
			      notebook->cur_page->allocation.width,
			      notebook->cur_page->allocation.height);
      bdk_window_raise (priv->drag_window);
    }

  return TRUE;
}

static bboolean
check_threshold (BtkNotebook *notebook,
		 bint         current_x,
		 bint         current_y)
{
  BtkWidget *widget;
  bint dnd_threshold;
  BdkRectangle rectangle = { 0, }; /* shut up gcc */
  BtkSettings *settings;
  
  widget = BTK_WIDGET (notebook);
  settings = btk_widget_get_settings (BTK_WIDGET (notebook));
  g_object_get (B_OBJECT (settings), "btk-dnd-drag-threshold", &dnd_threshold, NULL);

  /* we want a large threshold */
  dnd_threshold *= DND_THRESHOLD_MULTIPLIER;

  bdk_window_get_position (notebook->event_window, &rectangle.x, &rectangle.y);
  rectangle.width = bdk_window_get_width (notebook->event_window);
  rectangle.height = bdk_window_get_height (notebook->event_window);

  rectangle.x -= dnd_threshold;
  rectangle.width += 2 * dnd_threshold;
  rectangle.y -= dnd_threshold;
  rectangle.height += 2 * dnd_threshold;

  return (current_x < rectangle.x ||
	  current_x > rectangle.x + rectangle.width ||
	  current_y < rectangle.y ||
	  current_y > rectangle.y + rectangle.height);
}

static bint
btk_notebook_motion_notify (BtkWidget      *widget,
			    BdkEventMotion *event)
{
  BtkNotebook *notebook = BTK_NOTEBOOK (widget);
  BtkNotebookPrivate *priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);
  BtkNotebookPage *page;
  BtkNotebookArrow arrow;
  BtkNotebookPointerPosition pointer_position;
  BtkSettings *settings;
  buint timeout;
  bint x_win, y_win;

  page = notebook->cur_page;

  if (!page)
    return FALSE;

  if (!(event->state & BDK_BUTTON1_MASK) &&
      priv->pressed_button != -1)
    {
      btk_notebook_stop_reorder (notebook);
      stop_scrolling (notebook);
    }

  if (event->time < priv->timestamp + MSECS_BETWEEN_UPDATES)
    return FALSE;

  priv->timestamp = event->time;

  /* While animating the move, event->x is relative to the flying tab
   * (priv->drag_window has a pointer grab), but we need coordinates relative to
   * the notebook widget.
   */
  bdk_window_get_origin (widget->window, &x_win, &y_win);
  priv->mouse_x = event->x_root - x_win;
  priv->mouse_y = event->y_root - y_win;

  arrow = btk_notebook_get_arrow (notebook, priv->mouse_x, priv->mouse_y);
  if (arrow != notebook->in_child)
    {
      notebook->in_child = arrow;
      btk_notebook_redraw_arrows (notebook);
    }

  if (priv->pressed_button == -1)
    return FALSE;

  if (page->detachable &&
      check_threshold (notebook, priv->mouse_x, priv->mouse_y))
    {
      priv->detached_tab = notebook->cur_page;
      priv->during_detach = TRUE;

      btk_drag_begin (widget, priv->source_targets, BDK_ACTION_MOVE,
		      priv->pressed_button, (BdkEvent*) event);
      return TRUE;
    }

  if (page->reorderable &&
      (priv->during_reorder ||
       btk_drag_check_threshold (widget, priv->drag_begin_x, priv->drag_begin_y, priv->mouse_x, priv->mouse_y)))
    {
      priv->during_reorder = TRUE;
      pointer_position = get_pointer_position (notebook);

      if (event->window == priv->drag_window &&
	  pointer_position != POINTER_BETWEEN &&
	  btk_notebook_show_arrows (notebook))
	{
	  /* scroll tabs */
	  if (!priv->dnd_timer)
	    {
	      priv->has_scrolled = TRUE;
	      settings = btk_widget_get_settings (BTK_WIDGET (notebook));
	      g_object_get (settings, "btk-timeout-repeat", &timeout, NULL);

	      priv->dnd_timer = bdk_threads_add_timeout (timeout * SCROLL_DELAY_FACTOR,
					       scroll_notebook_timer, 
					       (bpointer) notebook);
	    }
	}
      else
	{
	  if (priv->dnd_timer)
	    {
	      g_source_remove (priv->dnd_timer);
	      priv->dnd_timer = 0;
	    }
	}

      if (event->window == priv->drag_window ||
	  priv->operation != DRAG_OPERATION_REORDER)
	{
	  /* the drag operation is beginning, create the window */
	  if (priv->operation != DRAG_OPERATION_REORDER)
	    {
	      priv->operation = DRAG_OPERATION_REORDER;
	      show_drag_window (notebook, priv, page);
	    }

	  btk_notebook_pages_allocate (notebook);
	  bdk_window_move_resize (priv->drag_window,
				  priv->drag_window_x,
				  priv->drag_window_y,
				  page->allocation.width,
				  page->allocation.height);
	}
    }

  return TRUE;
}

static void
btk_notebook_grab_notify (BtkWidget *widget,
			  bboolean   was_grabbed)
{
  BtkNotebook *notebook = BTK_NOTEBOOK (widget);

  if (!was_grabbed)
    {
      btk_notebook_stop_reorder (notebook);
      stop_scrolling (notebook);
    }
}

static void
btk_notebook_state_changed (BtkWidget    *widget,
			    BtkStateType  previous_state)
{
  if (!btk_widget_is_sensitive (widget))
    stop_scrolling (BTK_NOTEBOOK (widget));
}

static bint
btk_notebook_focus_in (BtkWidget     *widget,
		       BdkEventFocus *event)
{
  btk_notebook_redraw_tabs (BTK_NOTEBOOK (widget));
  
  return FALSE;
}

static bint
btk_notebook_focus_out (BtkWidget     *widget,
			BdkEventFocus *event)
{
  btk_notebook_redraw_tabs (BTK_NOTEBOOK (widget));

  return FALSE;
}

static void
btk_notebook_draw_focus (BtkWidget      *widget,
			 BdkEventExpose *event)
{
  BtkNotebook *notebook = BTK_NOTEBOOK (widget);

  if (btk_widget_has_focus (widget) && btk_widget_is_drawable (widget) &&
      notebook->show_tabs && notebook->cur_page &&
      notebook->cur_page->tab_label->window == event->window)
    {
      BtkNotebookPage *page;

      page = notebook->cur_page;

      if (btk_widget_intersect (page->tab_label, &event->area, NULL))
        {
          BdkRectangle area;
          bint focus_width;

          btk_widget_style_get (widget, "focus-line-width", &focus_width, NULL);

          area.x = page->tab_label->allocation.x - focus_width;
          area.y = page->tab_label->allocation.y - focus_width;
          area.width = page->tab_label->allocation.width + 2 * focus_width;
          area.height = page->tab_label->allocation.height + 2 * focus_width;

	  btk_paint_focus (widget->style, event->window, 
                           btk_widget_get_state (widget), NULL, widget, "tab",
			   area.x, area.y, area.width, area.height);
        }
    }
}

static void
btk_notebook_style_set  (BtkWidget *widget,
			 BtkStyle  *previous)
{
  BtkNotebook *notebook;

  bboolean has_before_previous;
  bboolean has_before_next;
  bboolean has_after_previous;
  bboolean has_after_next;

  notebook = BTK_NOTEBOOK (widget);
  
  btk_widget_style_get (widget,
                        "has-backward-stepper", &has_before_previous,
                        "has-secondary-forward-stepper", &has_before_next,
                        "has-secondary-backward-stepper", &has_after_previous,
                        "has-forward-stepper", &has_after_next,
                        NULL);
  
  notebook->has_before_previous = has_before_previous;
  notebook->has_before_next = has_before_next;
  notebook->has_after_previous = has_after_previous;
  notebook->has_after_next = has_after_next;

  BTK_WIDGET_CLASS (btk_notebook_parent_class)->style_set (widget, previous);
}

static bboolean
on_drag_icon_expose (BtkWidget      *widget,
		     BdkEventExpose *event,
		     bpointer        data)
{
  BtkWidget *notebook, *child = BTK_WIDGET (data);
  BtkRequisition requisition;
  bint gap_pos;

  notebook = BTK_WIDGET (data);
  child = BTK_BIN (widget)->child;
  btk_widget_size_request (widget, &requisition);
  gap_pos = get_tab_gap_pos (BTK_NOTEBOOK (notebook));

  btk_paint_extension (notebook->style, widget->window,
		       BTK_STATE_NORMAL, BTK_SHADOW_OUT,
		       NULL, widget, "tab",
		       0, 0,
		       requisition.width, requisition.height,
		       gap_pos);
  if (child)
    btk_container_propagate_expose (BTK_CONTAINER (widget), child, event);

  return TRUE;
}

static void
btk_notebook_drag_begin (BtkWidget        *widget,
			 BdkDragContext   *context)
{
  BtkNotebookPrivate *priv = BTK_NOTEBOOK_GET_PRIVATE (widget);
  BtkNotebook *notebook = (BtkNotebook*) widget;
  BtkWidget *tab_label;

  if (priv->dnd_timer)
    {
      g_source_remove (priv->dnd_timer);
      priv->dnd_timer = 0;
    }

  priv->operation = DRAG_OPERATION_DETACH;
  btk_notebook_pages_allocate (notebook);

  tab_label = priv->detached_tab->tab_label;

  hide_drag_window (notebook, priv, notebook->cur_page);
  g_object_ref (tab_label);
  btk_widget_unparent (tab_label);

  priv->dnd_window = btk_window_new (BTK_WINDOW_POPUP);
  btk_window_set_screen (BTK_WINDOW (priv->dnd_window),
                         btk_widget_get_screen (widget));
  btk_widget_set_colormap (priv->dnd_window, btk_widget_get_colormap (widget));
  btk_container_add (BTK_CONTAINER (priv->dnd_window), tab_label);
  btk_widget_set_size_request (priv->dnd_window,
			       priv->detached_tab->allocation.width,
			       priv->detached_tab->allocation.height);
  g_object_unref (tab_label);

  g_signal_connect (B_OBJECT (priv->dnd_window), "expose-event",
		    G_CALLBACK (on_drag_icon_expose), notebook);

  btk_drag_set_icon_widget (context, priv->dnd_window, -2, -2);
}

static void
btk_notebook_drag_end (BtkWidget      *widget,
		       BdkDragContext *context)
{
  BtkNotebookPrivate *priv = BTK_NOTEBOOK_GET_PRIVATE (widget);

  btk_notebook_stop_reorder (BTK_NOTEBOOK (widget));

  if (priv->detached_tab)
    btk_notebook_switch_page (BTK_NOTEBOOK (widget), priv->detached_tab);

  BTK_BIN (priv->dnd_window)->child = NULL;
  btk_widget_destroy (priv->dnd_window);
  priv->dnd_window = NULL;

  priv->operation = DRAG_OPERATION_NONE;
}

static BtkNotebook *
btk_notebook_create_window (BtkNotebook *notebook,
                            BtkWidget   *page,
                            bint         x,
                            bint         y)
{
  if (window_creation_hook)
    return (* window_creation_hook) (notebook, page, x, y, window_creation_hook_data);

  return NULL;
}

static bboolean
btk_notebook_drag_failed (BtkWidget      *widget,
			  BdkDragContext *context,
			  BtkDragResult   result,
			  bpointer        data)
{
  if (result == BTK_DRAG_RESULT_NO_TARGET)
    {
      BtkNotebookPrivate *priv;
      BtkNotebook *notebook, *dest_notebook = NULL;
      BdkDisplay *display;
      bint x, y;

      notebook = BTK_NOTEBOOK (widget);
      priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);

      display = btk_widget_get_display (widget);
      bdk_display_get_pointer (display, NULL, &x, &y, NULL);

      g_signal_emit (notebook, notebook_signals[CREATE_WINDOW], 0,
                     priv->detached_tab->child, x, y, &dest_notebook);

      if (dest_notebook)
	do_detach_tab (notebook, dest_notebook, priv->detached_tab->child, 0, 0);

      return TRUE;
    }

  return FALSE;
}

static bboolean
btk_notebook_switch_tab_timeout (bpointer data)
{
  BtkNotebook *notebook;
  BtkNotebookPrivate *priv;
  GList *tab;
  bint x, y;

  notebook = BTK_NOTEBOOK (data);
  priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);

  priv->switch_tab_timer = 0;
  x = priv->mouse_x;
  y = priv->mouse_y;

  if ((tab = get_tab_at_pos (notebook, x, y)) != NULL)
    {
      /* FIXME: hack, we don't want the
       * focus to move fom the source widget
       */
      notebook->child_has_focus = FALSE;
      btk_notebook_switch_focus_tab (notebook, tab);
    }

  return FALSE;
}

static bboolean
btk_notebook_drag_motion (BtkWidget      *widget,
			  BdkDragContext *context,
			  bint            x,
			  bint            y,
			  buint           time)
{
  BtkNotebook *notebook;
  BtkNotebookPrivate *priv;
  BdkRectangle position;
  BtkSettings *settings;
  BtkNotebookArrow arrow;
  buint timeout;
  BdkAtom target, tab_target;

  notebook = BTK_NOTEBOOK (widget);
  arrow = btk_notebook_get_arrow (notebook,
				  x + widget->allocation.x,
				  y + widget->allocation.y);
  if (arrow)
    {
      notebook->click_child = arrow;
      btk_notebook_set_scroll_timer (notebook);
      bdk_drag_status (context, 0, time);
      return TRUE;
    }

  stop_scrolling (notebook);
  target = btk_drag_dest_find_target (widget, context, NULL);
  tab_target = bdk_atom_intern_static_string ("BTK_NOTEBOOK_TAB");

  if (target == tab_target)
    {
      bpointer widget_group, source_widget_group;
      BtkWidget *source_widget;

      source_widget = btk_drag_get_source_widget (context);
      g_assert (source_widget);

      widget_group = btk_notebook_get_group (notebook);
      source_widget_group = btk_notebook_get_group (BTK_NOTEBOOK (source_widget));

      if (widget_group && source_widget_group &&
	  widget_group == source_widget_group &&
	  !(widget == BTK_NOTEBOOK (source_widget)->cur_page->child || 
	    btk_widget_is_ancestor (widget, BTK_NOTEBOOK (source_widget)->cur_page->child)))
	{
	  bdk_drag_status (context, BDK_ACTION_MOVE, time);
	  return TRUE;
	}
      else
	{
	  /* it's a tab, but doesn't share
	   * ID with this notebook */
	  bdk_drag_status (context, 0, time);
	}
    }

  priv = BTK_NOTEBOOK_GET_PRIVATE (widget);
  x += widget->allocation.x;
  y += widget->allocation.y;

  if (btk_notebook_get_event_window_position (notebook, &position) &&
      x >= position.x && x <= position.x + position.width &&
      y >= position.y && y <= position.y + position.height)
    {
      priv->mouse_x = x;
      priv->mouse_y = y;

      if (!priv->switch_tab_timer)
	{
          settings = btk_widget_get_settings (widget);

          g_object_get (settings, "btk-timeout-expand", &timeout, NULL);
	  priv->switch_tab_timer = bdk_threads_add_timeout (timeout,
						  btk_notebook_switch_tab_timeout,
						  widget);
	}
    }
  else
    {
      if (priv->switch_tab_timer)
	{
	  g_source_remove (priv->switch_tab_timer);
	  priv->switch_tab_timer = 0;
	}
    }

  return (target == tab_target) ? TRUE : FALSE;
}

static void
btk_notebook_drag_leave (BtkWidget      *widget,
			 BdkDragContext *context,
			 buint           time)
{
  BtkNotebookPrivate *priv;

  priv = BTK_NOTEBOOK_GET_PRIVATE (widget);

  if (priv->switch_tab_timer)
    {
      g_source_remove (priv->switch_tab_timer);
      priv->switch_tab_timer = 0;
    }

  stop_scrolling (BTK_NOTEBOOK (widget));
}

static bboolean
btk_notebook_drag_drop (BtkWidget        *widget,
			BdkDragContext   *context,
			bint              x,
			bint              y,
			buint             time)
{
  BdkAtom target, tab_target;

  target = btk_drag_dest_find_target (widget, context, NULL);
  tab_target = bdk_atom_intern_static_string ("BTK_NOTEBOOK_TAB");

  if (target == tab_target)
    {
      btk_drag_get_data (widget, context, target, time);
      return TRUE;
    }

  return FALSE;
}

static void
do_detach_tab (BtkNotebook     *from,
	       BtkNotebook     *to,
	       BtkWidget       *child,
	       bint             x,
	       bint             y)
{
  BtkNotebookPrivate *priv;
  BtkWidget *tab_label, *menu_label;
  bboolean tab_expand, tab_fill, reorderable, detachable;
  GList *element;
  buint tab_pack;
  bint page_num;

  menu_label = btk_notebook_get_menu_label (from, child);

  if (menu_label)
    g_object_ref (menu_label);

  tab_label = btk_notebook_get_tab_label (from, child);
  
  if (tab_label)
    g_object_ref (tab_label);

  g_object_ref (child);

  btk_container_child_get (BTK_CONTAINER (from),
			   child,
			   "tab-expand", &tab_expand,
			   "tab-fill", &tab_fill,
			   "tab-pack", &tab_pack,
			   "reorderable", &reorderable,
			   "detachable", &detachable,
			   NULL);

  btk_container_remove (BTK_CONTAINER (from), child);

  priv = BTK_NOTEBOOK_GET_PRIVATE (to);
  priv->mouse_x = x + BTK_WIDGET (to)->allocation.x;
  priv->mouse_y = y + BTK_WIDGET (to)->allocation.y;

  element = get_drop_position (to, tab_pack);
  page_num = g_list_position (to->children, element);
  btk_notebook_insert_page_menu (to, child, tab_label, menu_label, page_num);

  btk_container_child_set (BTK_CONTAINER (to), child,
			   "tab-pack", tab_pack,
			   "tab-expand", tab_expand,
			   "tab-fill", tab_fill,
			   "reorderable", reorderable,
			   "detachable", detachable,
			   NULL);
  if (child)
    g_object_unref (child);

  if (tab_label)
    g_object_unref (tab_label);

  if (menu_label)
    g_object_unref (menu_label);

  btk_notebook_set_current_page (to, page_num);
}

static void
btk_notebook_drag_data_get (BtkWidget        *widget,
			    BdkDragContext   *context,
			    BtkSelectionData *data,
			    buint             info,
			    buint             time)
{
  BtkNotebookPrivate *priv;

  if (data->target == bdk_atom_intern_static_string ("BTK_NOTEBOOK_TAB"))
    {
      priv = BTK_NOTEBOOK_GET_PRIVATE (widget);

      btk_selection_data_set (data,
			      data->target,
			      8,
			      (void*) &priv->detached_tab->child,
			      sizeof (bpointer));
    }
}

static void
btk_notebook_drag_data_received (BtkWidget        *widget,
				 BdkDragContext   *context,
				 bint              x,
				 bint              y,
				 BtkSelectionData *data,
				 buint             info,
				 buint             time)
{
  BtkNotebook *notebook;
  BtkWidget *source_widget;
  BtkWidget **child;

  notebook = BTK_NOTEBOOK (widget);
  source_widget = btk_drag_get_source_widget (context);

  if (source_widget &&
      data->target == bdk_atom_intern_static_string ("BTK_NOTEBOOK_TAB"))
    {
      child = (void*) data->data;

      do_detach_tab (BTK_NOTEBOOK (source_widget), notebook, *child, x, y);
      btk_drag_finish (context, TRUE, FALSE, time);
    }
  else
    btk_drag_finish (context, FALSE, FALSE, time);
}

/* Private BtkContainer Methods :
 * 
 * btk_notebook_set_child_arg
 * btk_notebook_get_child_arg
 * btk_notebook_add
 * btk_notebook_remove
 * btk_notebook_focus
 * btk_notebook_set_focus_child
 * btk_notebook_child_type
 * btk_notebook_forall
 */
static void
btk_notebook_set_child_property (BtkContainer    *container,
				 BtkWidget       *child,
				 buint            property_id,
				 const BValue    *value,
				 BParamSpec      *pspec)
{
  bboolean expand;
  bboolean fill;
  BtkPackType pack_type;

  /* not finding child's page is valid for menus or labels */
  if (!btk_notebook_find_child (BTK_NOTEBOOK (container), child, NULL))
    return;

  switch (property_id)
    {
    case CHILD_PROP_TAB_LABEL:
      /* a NULL pointer indicates a default_tab setting, otherwise
       * we need to set the associated label
       */
      btk_notebook_set_tab_label_text (BTK_NOTEBOOK (container), child,
				       b_value_get_string (value));
      break;
    case CHILD_PROP_MENU_LABEL:
      btk_notebook_set_menu_label_text (BTK_NOTEBOOK (container), child,
					b_value_get_string (value));
      break;
    case CHILD_PROP_POSITION:
      btk_notebook_reorder_child (BTK_NOTEBOOK (container), child,
				  b_value_get_int (value));
      break;
    case CHILD_PROP_TAB_EXPAND:
      btk_notebook_query_tab_label_packing (BTK_NOTEBOOK (container), child,
					    &expand, &fill, &pack_type);
      btk_notebook_set_tab_label_packing (BTK_NOTEBOOK (container), child,
					  b_value_get_boolean (value),
					  fill, pack_type);
      break;
    case CHILD_PROP_TAB_FILL:
      btk_notebook_query_tab_label_packing (BTK_NOTEBOOK (container), child,
					    &expand, &fill, &pack_type);
      btk_notebook_set_tab_label_packing (BTK_NOTEBOOK (container), child,
					  expand,
					  b_value_get_boolean (value),
					  pack_type);
      break;
    case CHILD_PROP_TAB_PACK:
      btk_notebook_query_tab_label_packing (BTK_NOTEBOOK (container), child,
					    &expand, &fill, &pack_type);
      btk_notebook_set_tab_label_packing (BTK_NOTEBOOK (container), child,
					  expand, fill,
					  b_value_get_enum (value));
      break;
    case CHILD_PROP_REORDERABLE:
      btk_notebook_set_tab_reorderable (BTK_NOTEBOOK (container), child,
					b_value_get_boolean (value));
      break;
    case CHILD_PROP_DETACHABLE:
      btk_notebook_set_tab_detachable (BTK_NOTEBOOK (container), child,
				       b_value_get_boolean (value));
      break;
    default:
      BTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
btk_notebook_get_child_property (BtkContainer    *container,
				 BtkWidget       *child,
				 buint            property_id,
				 BValue          *value,
				 BParamSpec      *pspec)
{
  GList *list;
  BtkNotebook *notebook;
  BtkWidget *label;
  bboolean expand;
  bboolean fill;
  BtkPackType pack_type;

  notebook = BTK_NOTEBOOK (container);

  /* not finding child's page is valid for menus or labels */
  list = btk_notebook_find_child (notebook, child, NULL);
  if (!list)
    {
      /* nothing to set on labels or menus */
      g_param_value_set_default (pspec, value);
      return;
    }

  switch (property_id)
    {
    case CHILD_PROP_TAB_LABEL:
      label = btk_notebook_get_tab_label (notebook, child);

      if (BTK_IS_LABEL (label))
	b_value_set_string (value, BTK_LABEL (label)->label);
      else
	b_value_set_string (value, NULL);
      break;
    case CHILD_PROP_MENU_LABEL:
      label = btk_notebook_get_menu_label (notebook, child);

      if (BTK_IS_LABEL (label))
	b_value_set_string (value, BTK_LABEL (label)->label);
      else
	b_value_set_string (value, NULL);
      break;
    case CHILD_PROP_POSITION:
      b_value_set_int (value, g_list_position (notebook->children, list));
      break;
    case CHILD_PROP_TAB_EXPAND:
	btk_notebook_query_tab_label_packing (BTK_NOTEBOOK (container), child,
					      &expand, NULL, NULL);
	b_value_set_boolean (value, expand);
      break;
    case CHILD_PROP_TAB_FILL:
	btk_notebook_query_tab_label_packing (BTK_NOTEBOOK (container), child,
					      NULL, &fill, NULL);
	b_value_set_boolean (value, fill);
      break;
    case CHILD_PROP_TAB_PACK:
	btk_notebook_query_tab_label_packing (BTK_NOTEBOOK (container), child,
					      NULL, NULL, &pack_type);
	b_value_set_enum (value, pack_type);
      break;
    case CHILD_PROP_REORDERABLE:
      b_value_set_boolean (value,
			   btk_notebook_get_tab_reorderable (BTK_NOTEBOOK (container), child));
      break;
    case CHILD_PROP_DETACHABLE:
      b_value_set_boolean (value,
			   btk_notebook_get_tab_detachable (BTK_NOTEBOOK (container), child));
      break;
    default:
      BTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
btk_notebook_add (BtkContainer *container,
		  BtkWidget    *widget)
{
  btk_notebook_insert_page_menu (BTK_NOTEBOOK (container), widget, 
				 NULL, NULL, -1);
}

static void
btk_notebook_remove (BtkContainer *container,
		     BtkWidget    *widget)
{
  BtkNotebook *notebook;
  BtkNotebookPage *page;
  GList *children;
  bint page_num = 0;

  notebook = BTK_NOTEBOOK (container);

  children = notebook->children;
  while (children)
    {
      page = children->data;

      if (page->child == widget)
	break;

      page_num++;
      children = children->next;
    }
 
  if (children == NULL)
    return;
  
  g_object_ref (widget);

  btk_notebook_real_remove (notebook, children);

  g_signal_emit (notebook,
		 notebook_signals[PAGE_REMOVED],
		 0,
		 widget,
		 page_num);
  
  g_object_unref (widget);
}

static bboolean
focus_tabs_in (BtkNotebook *notebook)
{
  if (notebook->show_tabs && notebook->cur_page)
    {
      btk_widget_grab_focus (BTK_WIDGET (notebook));

      btk_notebook_switch_focus_tab (notebook,
				     g_list_find (notebook->children,
						  notebook->cur_page));

      return TRUE;
    }
  else
    return FALSE;
}

static bboolean
focus_tabs_move (BtkNotebook     *notebook,
		 BtkDirectionType direction,
		 bint             search_direction)
{
  GList *new_page;

  new_page = btk_notebook_search_page (notebook, notebook->focus_tab,
				       search_direction, TRUE);
  if (!new_page)
    {
      bboolean wrap_around;

      g_object_get (btk_widget_get_settings (BTK_WIDGET (notebook)),
                    "btk-keynav-wrap-around", &wrap_around,
                    NULL);

      if (wrap_around)
        new_page = btk_notebook_search_page (notebook, NULL,
                                             search_direction, TRUE);
    }

  if (new_page)
    btk_notebook_switch_focus_tab (notebook, new_page);
  else
    btk_widget_error_bell (BTK_WIDGET (notebook));

  return TRUE;
}

static bboolean
focus_child_in (BtkNotebook      *notebook,
		BtkDirectionType  direction)
{
  if (notebook->cur_page)
    return btk_widget_child_focus (notebook->cur_page->child, direction);
  else
    return FALSE;
}

static bboolean
focus_action_in (BtkNotebook      *notebook,
                 bint              action,
                 BtkDirectionType  direction)
{
  BtkNotebookPrivate *priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);

  if (priv->action_widget[action] &&
      btk_widget_get_visible (priv->action_widget[action]))
    return btk_widget_child_focus (priv->action_widget[action], direction);
  else
    return FALSE;
}

/* Focus in the notebook can either be on the pages, or on
 * the tabs or on the action_widgets.
 */
static bint
btk_notebook_focus (BtkWidget        *widget,
		    BtkDirectionType  direction)
{
  BtkNotebookPrivate *priv;
  BtkWidget *old_focus_child;
  BtkNotebook *notebook;
  BtkDirectionType effective_direction;
  bint first_action;
  bint last_action;

  bboolean widget_is_focus;
  BtkContainer *container;

  container = BTK_CONTAINER (widget);
  notebook = BTK_NOTEBOOK (container);
  priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);

  if (notebook->tab_pos == BTK_POS_TOP ||
      notebook->tab_pos == BTK_POS_LEFT)
    {
      first_action = ACTION_WIDGET_START;
      last_action = ACTION_WIDGET_END;
    }
  else
    {
      first_action = ACTION_WIDGET_END;
      last_action = ACTION_WIDGET_START;
    }

  if (notebook->focus_out)
    {
      notebook->focus_out = FALSE; /* Clear this to catch the wrap-around case */
      return FALSE;
    }

  widget_is_focus = btk_widget_is_focus (widget);
  old_focus_child = container->focus_child;

  effective_direction = get_effective_direction (notebook, direction);

  if (old_focus_child)		/* Focus on page child or action widget */
    {
      if (btk_widget_child_focus (old_focus_child, direction))
        return TRUE;

      if (old_focus_child == priv->action_widget[ACTION_WIDGET_START])
	{
	  switch (effective_direction)
	    {
	    case BTK_DIR_DOWN:
              return focus_child_in (notebook, BTK_DIR_TAB_FORWARD);
	    case BTK_DIR_RIGHT:
	      return focus_tabs_in (notebook);
	    case BTK_DIR_LEFT:
              return FALSE;
	    case BTK_DIR_UP:
	      return FALSE;
            default:
              switch (direction)
                {
	        case BTK_DIR_TAB_FORWARD:
                  if ((notebook->tab_pos == BTK_POS_RIGHT || notebook->tab_pos == BTK_POS_BOTTOM) &&
                      focus_child_in (notebook, direction))
                    return TRUE;
	          return focus_tabs_in (notebook);
                case BTK_DIR_TAB_BACKWARD:
                  return FALSE;
                default:
                  g_assert_not_reached ();
                }
	    }
	}
      else if (old_focus_child == priv->action_widget[ACTION_WIDGET_END])
	{
	  switch (effective_direction)
	    {
	    case BTK_DIR_DOWN:
              return focus_child_in (notebook, BTK_DIR_TAB_FORWARD);
	    case BTK_DIR_RIGHT:
              return FALSE;
	    case BTK_DIR_LEFT:
	      return focus_tabs_in (notebook);
	    case BTK_DIR_UP:
	      return FALSE;
            default:
              switch (direction)
                {
	        case BTK_DIR_TAB_FORWARD:
                  return FALSE;
                case BTK_DIR_TAB_BACKWARD:
                  if ((notebook->tab_pos == BTK_POS_TOP || notebook->tab_pos == BTK_POS_LEFT) &&
                      focus_child_in (notebook, direction))
                    return TRUE;
	          return focus_tabs_in (notebook);
                default:
                  g_assert_not_reached ();
                }
	    }
	}
      else
	{
	  switch (effective_direction)
	    {
	    case BTK_DIR_TAB_BACKWARD:
	    case BTK_DIR_UP:
	      /* Focus onto the tabs */
	      return focus_tabs_in (notebook);
	    case BTK_DIR_DOWN:
	    case BTK_DIR_LEFT:
	    case BTK_DIR_RIGHT:
	      return FALSE;
	    case BTK_DIR_TAB_FORWARD:
              return focus_action_in (notebook, last_action, direction);
	    }
	}
    }
  else if (widget_is_focus)	/* Focus was on tabs */
    {
      switch (effective_direction)
	{
        case BTK_DIR_TAB_BACKWARD:
              return focus_action_in (notebook, first_action, direction);
        case BTK_DIR_UP:
	  return FALSE;
        case BTK_DIR_TAB_FORWARD:
          if (focus_child_in (notebook, BTK_DIR_TAB_FORWARD))
            return TRUE;
          return focus_action_in (notebook, last_action, direction);
	case BTK_DIR_DOWN:
	  /* We use TAB_FORWARD rather than direction so that we focus a more
	   * predictable widget for the user; users may be using arrow focusing
	   * in this situation even if they don't usually use arrow focusing.
	   */
          return focus_child_in (notebook, BTK_DIR_TAB_FORWARD);
	case BTK_DIR_LEFT:
	  return focus_tabs_move (notebook, direction, STEP_PREV);
	case BTK_DIR_RIGHT:
	  return focus_tabs_move (notebook, direction, STEP_NEXT);
	}
    }
  else /* Focus was not on widget */
    {
      switch (effective_direction)
	{
	case BTK_DIR_TAB_FORWARD:
	case BTK_DIR_DOWN:
          if (focus_action_in (notebook, first_action, direction))
            return TRUE;
	  if (focus_tabs_in (notebook))
	    return TRUE;
          if (focus_action_in (notebook, last_action, direction))
            return TRUE;
	  if (focus_child_in (notebook, direction))
	    return TRUE;
	  return FALSE;
	case BTK_DIR_TAB_BACKWARD:
          if (focus_action_in (notebook, last_action, direction))
            return TRUE;
	  if (focus_child_in (notebook, direction))
	    return TRUE;
	  if (focus_tabs_in (notebook))
	    return TRUE;
          if (focus_action_in (notebook, first_action, direction))
            return TRUE;
	case BTK_DIR_UP:
	case BTK_DIR_LEFT:
	case BTK_DIR_RIGHT:
	  return focus_child_in (notebook, direction);
	}
    }

  g_assert_not_reached ();
  return FALSE;
}

static void
btk_notebook_set_focus_child (BtkContainer *container,
			      BtkWidget    *child)
{
  BtkNotebook *notebook = BTK_NOTEBOOK (container);
  BtkWidget *page_child;
  BtkWidget *toplevel;

  /* If the old focus widget was within a page of the notebook,
   * (child may either be NULL or not in this case), record it
   * for future use if we switch to the page with a mnemonic.
   */

  toplevel = btk_widget_get_toplevel (BTK_WIDGET (container));
  if (toplevel && btk_widget_is_toplevel (toplevel))
    {
      page_child = BTK_WINDOW (toplevel)->focus_widget; 
      while (page_child)
	{
	  if (page_child->parent == BTK_WIDGET (container))
	    {
	      GList *list = btk_notebook_find_child (notebook, page_child, NULL);
	      if (list != NULL) 
		{
		  BtkNotebookPage *page = list->data;
	      
		  if (page->last_focus_child)
		    g_object_remove_weak_pointer (B_OBJECT (page->last_focus_child), (bpointer *)&page->last_focus_child);
		  
		  page->last_focus_child = BTK_WINDOW (toplevel)->focus_widget;
		  g_object_add_weak_pointer (B_OBJECT (page->last_focus_child), (bpointer *)&page->last_focus_child);
	      
		  break;
		}
	    }

	  page_child = page_child->parent;
	}
    }
  
  if (child)
    {
      g_return_if_fail (BTK_IS_WIDGET (child));

      notebook->child_has_focus = TRUE;
      if (!notebook->focus_tab)
	{
	  GList *children;
	  BtkNotebookPage *page;

	  children = notebook->children;
	  while (children)
	    {
	      page = children->data;
	      if (page->child == child || page->tab_label == child)
		btk_notebook_switch_focus_tab (notebook, children);
	      children = children->next;
	    }
	}
    }
  else
    notebook->child_has_focus = FALSE;

  BTK_CONTAINER_CLASS (btk_notebook_parent_class)->set_focus_child (container, child);
}

static void
btk_notebook_forall (BtkContainer *container,
		     bboolean      include_internals,
		     BtkCallback   callback,
		     bpointer      callback_data)
{
  BtkNotebookPrivate *priv;
  BtkNotebook *notebook;
  GList *children;
  bint i;

  notebook = BTK_NOTEBOOK (container);
  priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);

  children = notebook->children;
  while (children)
    {
      BtkNotebookPage *page;
      
      page = children->data;
      children = children->next;
      (* callback) (page->child, callback_data);

      if (include_internals)
	{
	  if (page->tab_label)
	    (* callback) (page->tab_label, callback_data);
	}
    }

  if (include_internals) {
    for (i = 0; i < N_ACTION_WIDGETS; i++)
      {
        if (priv->action_widget[i])
          (* callback) (priv->action_widget[i], callback_data);
      }
  }
}

static GType
btk_notebook_child_type (BtkContainer     *container)
{
  return BTK_TYPE_WIDGET;
}

/* Private BtkNotebook Methods:
 *
 * btk_notebook_real_insert_page
 */
static void
page_visible_cb (BtkWidget  *page,
                 BParamSpec *arg,
                 bpointer    data)
{
  BtkNotebook *notebook = (BtkNotebook *) data;
  GList *list;
  GList *next = NULL;

  if (notebook->cur_page &&
      notebook->cur_page->child == page &&
      !btk_widget_get_visible (page))
    {
      list = g_list_find (notebook->children, notebook->cur_page);
      if (list)
        {
          next = btk_notebook_search_page (notebook, list, STEP_NEXT, TRUE);
          if (!next)
            next = btk_notebook_search_page (notebook, list, STEP_PREV, TRUE);
        }

      if (next)
        btk_notebook_switch_page (notebook, BTK_NOTEBOOK_PAGE (next));
    }
}

static bint
btk_notebook_real_insert_page (BtkNotebook *notebook,
			       BtkWidget   *child,
			       BtkWidget   *tab_label,
			       BtkWidget   *menu_label,
			       bint         position)
{
  BtkNotebookPage *page;
  bint nchildren;

  btk_widget_freeze_child_notify (child);

  page = g_slice_new0 (BtkNotebookPage);
  page->child = child;

  nchildren = g_list_length (notebook->children);
  if ((position < 0) || (position > nchildren))
    position = nchildren;

  notebook->children = g_list_insert (notebook->children, page, position);

  if (!tab_label)
    {
      page->default_tab = TRUE;
      if (notebook->show_tabs)
	tab_label = btk_label_new (NULL);
    }
  page->tab_label = tab_label;
  page->menu_label = menu_label;
  page->expand = FALSE;
  page->fill = TRUE;
  page->pack = BTK_PACK_START; 

  if (!menu_label)
    page->default_menu = TRUE;
  else  
    g_object_ref_sink (page->menu_label);

  if (notebook->menu)
    btk_notebook_menu_item_create (notebook,
				   g_list_find (notebook->children, page));

  btk_widget_set_parent (child, BTK_WIDGET (notebook));
  if (tab_label)
    btk_widget_set_parent (tab_label, BTK_WIDGET (notebook));

  btk_notebook_update_labels (notebook);

  if (!notebook->first_tab)
    notebook->first_tab = notebook->children;

  /* child visible will be turned on by switch_page below */
  if (notebook->cur_page != page)
    btk_widget_set_child_visible (child, FALSE);

  if (tab_label)
    {
      if (notebook->show_tabs && btk_widget_get_visible (child))
	btk_widget_show (tab_label);
      else
	btk_widget_hide (tab_label);

    page->mnemonic_activate_signal =
      g_signal_connect (tab_label,
			"mnemonic-activate",
			G_CALLBACK (btk_notebook_mnemonic_activate_switch_page),
			notebook);
    }

  page->notify_visible_handler = g_signal_connect (child, "notify::visible",
						   G_CALLBACK (page_visible_cb), notebook);

  g_signal_emit (notebook,
		 notebook_signals[PAGE_ADDED],
		 0,
		 child,
		 position);

  if (!notebook->cur_page)
    {
      btk_notebook_switch_page (notebook, page);
      /* focus_tab is set in the switch_page method */
      btk_notebook_switch_focus_tab (notebook, notebook->focus_tab);
    }

  btk_notebook_update_tab_states (notebook);

  if (notebook->scrollable)
    btk_notebook_redraw_arrows (notebook);

  btk_widget_child_notify (child, "tab-expand");
  btk_widget_child_notify (child, "tab-fill");
  btk_widget_child_notify (child, "tab-pack");
  btk_widget_child_notify (child, "tab-label");
  btk_widget_child_notify (child, "menu-label");
  btk_widget_child_notify (child, "position");
  btk_widget_thaw_child_notify (child);

  /* The page-added handler might have reordered the pages, re-get the position */
  return btk_notebook_page_num (notebook, child);
}

/* Private BtkNotebook Functions:
 *
 * btk_notebook_redraw_tabs
 * btk_notebook_real_remove
 * btk_notebook_update_labels
 * btk_notebook_timer
 * btk_notebook_set_scroll_timer
 * btk_notebook_page_compare
 * btk_notebook_real_page_position
 * btk_notebook_search_page
 */
static void
btk_notebook_redraw_tabs (BtkNotebook *notebook)
{
  BtkWidget *widget;
  BtkNotebookPage *page;
  BdkRectangle redraw_rect;
  bint border;
  bint tab_pos = get_effective_tab_pos (notebook);

  widget = BTK_WIDGET (notebook);
  border = BTK_CONTAINER (notebook)->border_width;

  if (!btk_widget_get_mapped (widget) || !notebook->first_tab)
    return;

  page = notebook->first_tab->data;

  redraw_rect.x = border;
  redraw_rect.y = border;

  switch (tab_pos)
    {
    case BTK_POS_BOTTOM:
      redraw_rect.y = widget->allocation.height - border -
	page->allocation.height - widget->style->ythickness;

      if (page != notebook->cur_page)
	redraw_rect.y -= widget->style->ythickness;
      /* fall through */
    case BTK_POS_TOP:
      redraw_rect.width = widget->allocation.width - 2 * border;
      redraw_rect.height = page->allocation.height + widget->style->ythickness;

      if (page != notebook->cur_page)
	redraw_rect.height += widget->style->ythickness;
      break;
    case BTK_POS_RIGHT:
      redraw_rect.x = widget->allocation.width - border -
	page->allocation.width - widget->style->xthickness;

      if (page != notebook->cur_page)
	redraw_rect.x -= widget->style->xthickness;
      /* fall through */
    case BTK_POS_LEFT:
      redraw_rect.width = page->allocation.width + widget->style->xthickness;
      redraw_rect.height = widget->allocation.height - 2 * border;

      if (page != notebook->cur_page)
	redraw_rect.width += widget->style->xthickness;
      break;
    }

  redraw_rect.x += widget->allocation.x;
  redraw_rect.y += widget->allocation.y;

  bdk_window_invalidate_rect (widget->window, &redraw_rect, TRUE);
}

static void
btk_notebook_redraw_arrows (BtkNotebook *notebook)
{
  if (btk_widget_get_mapped (BTK_WIDGET (notebook)) &&
      btk_notebook_show_arrows (notebook))
    {
      BdkRectangle rect;
      bint i;
      BtkNotebookArrow arrow[4];

      arrow[0] = notebook->has_before_previous ? ARROW_LEFT_BEFORE : ARROW_NONE;
      arrow[1] = notebook->has_before_next ? ARROW_RIGHT_BEFORE : ARROW_NONE;
      arrow[2] = notebook->has_after_previous ? ARROW_LEFT_AFTER : ARROW_NONE;
      arrow[3] = notebook->has_after_next ? ARROW_RIGHT_AFTER : ARROW_NONE;

      for (i = 0; i < 4; i++) 
	{
	  if (arrow[i] == ARROW_NONE)
	    continue;

	  btk_notebook_get_arrow_rect (notebook, &rect, arrow[i]);
	  bdk_window_invalidate_rect (BTK_WIDGET (notebook)->window, 
				      &rect, FALSE);
	}
    }
}

static bboolean
btk_notebook_timer (BtkNotebook *notebook)
{
  bboolean retval = FALSE;

  if (notebook->timer)
    {
      btk_notebook_do_arrow (notebook, notebook->click_child);

      if (notebook->need_timer)
	{
          BtkSettings *settings;
          buint        timeout;

          settings = btk_widget_get_settings (BTK_WIDGET (notebook));
          g_object_get (settings, "btk-timeout-repeat", &timeout, NULL);

	  notebook->need_timer = FALSE;
	  notebook->timer = bdk_threads_add_timeout (timeout * SCROLL_DELAY_FACTOR,
					   (GSourceFunc) btk_notebook_timer,
					   (bpointer) notebook);
	}
      else
	retval = TRUE;
    }

  return retval;
}

static void
btk_notebook_set_scroll_timer (BtkNotebook *notebook)
{
  BtkWidget *widget = BTK_WIDGET (notebook);

  if (!notebook->timer)
    {
      BtkSettings *settings = btk_widget_get_settings (widget);
      buint timeout;

      g_object_get (settings, "btk-timeout-initial", &timeout, NULL);

      notebook->timer = bdk_threads_add_timeout (timeout,
				       (GSourceFunc) btk_notebook_timer,
				       (bpointer) notebook);
      notebook->need_timer = TRUE;
    }
}

static bint
btk_notebook_page_compare (gconstpointer a,
			   gconstpointer b)
{
  return (((BtkNotebookPage *) a)->child != b);
}

static GList*
btk_notebook_find_child (BtkNotebook *notebook,
			 BtkWidget   *child,
			 const bchar *function)
{
  GList *list = g_list_find_custom (notebook->children, child,
				    btk_notebook_page_compare);

#ifndef G_DISABLE_CHECKS
  if (!list && function)
    g_warning ("%s: unable to find child %p in notebook %p",
	       function, child, notebook);
#endif

  return list;
}

static void
btk_notebook_remove_tab_label (BtkNotebook     *notebook,
			       BtkNotebookPage *page)
{
  if (page->tab_label)
    {
      if (page->mnemonic_activate_signal)
	g_signal_handler_disconnect (page->tab_label,
				     page->mnemonic_activate_signal);
      page->mnemonic_activate_signal = 0;

      btk_widget_set_state (page->tab_label, BTK_STATE_NORMAL);
      btk_widget_unparent (page->tab_label);
      page->tab_label = NULL;
    }
}

static void
btk_notebook_real_remove (BtkNotebook *notebook,
			  GList       *list)
{
  BtkNotebookPrivate *priv;
  BtkNotebookPage *page;
  GList * next_list;
  bint need_resize = FALSE;
  BtkWidget *tab_label;

  bboolean destroying;

  priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);
  destroying = BTK_OBJECT_FLAGS (notebook) & BTK_IN_DESTRUCTION;
  
  next_list = btk_notebook_search_page (notebook, list, STEP_NEXT, TRUE);
  if (!next_list)
    next_list = btk_notebook_search_page (notebook, list, STEP_PREV, TRUE);

  notebook->children = g_list_remove_link (notebook->children, list);

  if (notebook->cur_page == list->data)
    { 
      notebook->cur_page = NULL;
      if (next_list && !destroying)
	btk_notebook_switch_page (notebook, BTK_NOTEBOOK_PAGE (next_list));
    }

  if (priv->detached_tab == list->data)
    priv->detached_tab = NULL;

  if (list == notebook->first_tab)
    notebook->first_tab = next_list;
  if (list == notebook->focus_tab && !destroying)
    btk_notebook_switch_focus_tab (notebook, next_list);

  page = list->data;
  
  g_signal_handler_disconnect (page->child, page->notify_visible_handler); 

  if (btk_widget_get_visible (page->child) &&
      btk_widget_get_visible (BTK_WIDGET (notebook)))
    need_resize = TRUE;

  btk_widget_unparent (page->child);

  tab_label = page->tab_label;
  if (tab_label)
    {
      g_object_ref (tab_label);
      btk_notebook_remove_tab_label (notebook, page);
      if (destroying)
        btk_widget_destroy (tab_label);
      g_object_unref (tab_label);
    }

  if (notebook->menu)
    {
      BtkWidget *parent = page->menu_label->parent;

      btk_notebook_menu_label_unparent (parent, NULL);
      btk_container_remove (BTK_CONTAINER (notebook->menu), parent);

      btk_widget_queue_resize (notebook->menu);
    }
  if (!page->default_menu)
    g_object_unref (page->menu_label);

  g_list_free (list);

  if (page->last_focus_child)
    {
      g_object_remove_weak_pointer (B_OBJECT (page->last_focus_child), (bpointer *)&page->last_focus_child);
      page->last_focus_child = NULL;
    }
  
  g_slice_free (BtkNotebookPage, page);

  btk_notebook_update_labels (notebook);
  if (need_resize)
    btk_widget_queue_resize (BTK_WIDGET (notebook));
}

static void
btk_notebook_update_labels (BtkNotebook *notebook)
{
  BtkNotebookPage *page;
  GList *list;
  bchar string[32];
  bint page_num = 1;

  if (!notebook->show_tabs && !notebook->menu)
    return;

  for (list = btk_notebook_search_page (notebook, NULL, STEP_NEXT, FALSE);
       list;
       list = btk_notebook_search_page (notebook, list, STEP_NEXT, FALSE))
    {
      page = list->data;
      g_snprintf (string, sizeof(string), _("Page %u"), page_num++);
      if (notebook->show_tabs)
	{
	  if (page->default_tab)
	    {
	      if (!page->tab_label)
		{
		  page->tab_label = btk_label_new (string);
		  btk_widget_set_parent (page->tab_label,
					 BTK_WIDGET (notebook));
		}
	      else
		btk_label_set_text (BTK_LABEL (page->tab_label), string);
	    }

	  if (btk_widget_get_visible (page->child) &&
	      !btk_widget_get_visible (page->tab_label))
	    btk_widget_show (page->tab_label);
	  else if (!btk_widget_get_visible (page->child) &&
		   btk_widget_get_visible (page->tab_label))
	    btk_widget_hide (page->tab_label);
	}
      if (notebook->menu && page->default_menu)
	{
	  if (BTK_IS_LABEL (page->tab_label))
	    btk_label_set_text (BTK_LABEL (page->menu_label),
                                BTK_LABEL (page->tab_label)->label);
	  else
	    btk_label_set_text (BTK_LABEL (page->menu_label), string);
	}
    }  
}

static bint
btk_notebook_real_page_position (BtkNotebook *notebook,
				 GList       *list)
{
  GList *work;
  bint count_start;

  for (work = notebook->children, count_start = 0;
       work && work != list; work = work->next)
    if (BTK_NOTEBOOK_PAGE (work)->pack == BTK_PACK_START)
      count_start++;

  if (!work)
    return -1;

  if (BTK_NOTEBOOK_PAGE (list)->pack == BTK_PACK_START)
    return count_start;

  return (count_start + g_list_length (list) - 1);
}

static GList *
btk_notebook_search_page (BtkNotebook *notebook,
			  GList       *list,
			  bint         direction,
			  bboolean     find_visible)
{
  BtkNotebookPage *page = NULL;
  GList *old_list = NULL;
  bint flag = 0;

  switch (direction)
    {
    case STEP_PREV:
      flag = BTK_PACK_END;
      break;

    case STEP_NEXT:
      flag = BTK_PACK_START;
      break;
    }

  if (list)
    page = list->data;

  if (!page || page->pack == flag)
    {
      if (list)
	{
	  old_list = list;
	  list = list->next;
	}
      else
	list = notebook->children;

      while (list)
	{
	  page = list->data;
	  if (page->pack == flag &&
	      (!find_visible ||
	       (btk_widget_get_visible (page->child) &&
		(!page->tab_label || NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page)))))
	    return list;
	  old_list = list;
	  list = list->next;
	}
      list = old_list;
    }
  else
    {
      old_list = list;
      list = list->prev;
    }
  while (list)
    {
      page = list->data;
      if (page->pack != flag &&
	  (!find_visible ||
	   (btk_widget_get_visible (page->child) &&
	    (!page->tab_label || NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page)))))
	return list;
      old_list = list;
      list = list->prev;
    }
  return NULL;
}

/* Private BtkNotebook Drawing Functions:
 *
 * btk_notebook_paint
 * btk_notebook_draw_tab
 * btk_notebook_draw_arrow
 */
static void
btk_notebook_paint (BtkWidget    *widget,
		    BdkRectangle *area)
{
  BtkNotebook *notebook;
  BtkNotebookPrivate *priv;
  BtkNotebookPage *page;
  GList *children;
  bboolean showarrow;
  bint width, height;
  bint x, y;
  bint border_width = BTK_CONTAINER (widget)->border_width;
  bint gap_x = 0, gap_width = 0, step = STEP_PREV;
  bboolean is_rtl;
  bint tab_pos;
   
  if (!btk_widget_is_drawable (widget))
    return;

  notebook = BTK_NOTEBOOK (widget);
  priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);
  is_rtl = btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL;
  tab_pos = get_effective_tab_pos (notebook);

  if ((!notebook->show_tabs && !notebook->show_border) ||
      !notebook->cur_page || !btk_widget_get_visible (notebook->cur_page->child))
    return;

  x = widget->allocation.x + border_width;
  y = widget->allocation.y + border_width;
  width = widget->allocation.width - border_width * 2;
  height = widget->allocation.height - border_width * 2;

  if (notebook->show_border && (!notebook->show_tabs || !notebook->children))
    {
      btk_paint_box (widget->style, widget->window,
		     BTK_STATE_NORMAL, BTK_SHADOW_OUT,
		     area, widget, "notebook",
		     x, y, width, height);
      return;
    }

  if (!notebook->first_tab)
    notebook->first_tab = notebook->children;

  if (!btk_widget_get_mapped (notebook->cur_page->tab_label))
    page = BTK_NOTEBOOK_PAGE (notebook->first_tab);
  else
    page = notebook->cur_page;

  switch (tab_pos)
    {
    case BTK_POS_TOP:
      y += page->allocation.height;
      /* fall thru */
    case BTK_POS_BOTTOM:
      height -= page->allocation.height;
      break;
    case BTK_POS_LEFT:
      x += page->allocation.width;
      /* fall thru */
    case BTK_POS_RIGHT:
      width -= page->allocation.width;
      break;
    }

  if (!NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, notebook->cur_page) ||
      !btk_widget_get_mapped (notebook->cur_page->tab_label))
    {
      gap_x = 0;
      gap_width = 0;
    }
  else
    {
      switch (tab_pos)
	{
	case BTK_POS_TOP:
	case BTK_POS_BOTTOM:
	  if (priv->operation == DRAG_OPERATION_REORDER)
	    gap_x = priv->drag_window_x - widget->allocation.x - border_width;
	  else
	    gap_x = notebook->cur_page->allocation.x - widget->allocation.x - border_width;

	  gap_width = notebook->cur_page->allocation.width;
	  step = is_rtl ? STEP_NEXT : STEP_PREV;
	  break;
	case BTK_POS_LEFT:
	case BTK_POS_RIGHT:
	  if (priv->operation == DRAG_OPERATION_REORDER)
	    gap_x = priv->drag_window_y - border_width - widget->allocation.y;
	  else
	    gap_x = notebook->cur_page->allocation.y - widget->allocation.y - border_width;

	  gap_width = notebook->cur_page->allocation.height;
	  step = STEP_PREV;
	  break;
	}
    }
  btk_paint_box_gap (widget->style, widget->window,
		     BTK_STATE_NORMAL, BTK_SHADOW_OUT,
		     area, widget, "notebook",
		     x, y, width, height,
		     tab_pos, gap_x, gap_width);

  showarrow = FALSE;
  children = btk_notebook_search_page (notebook, NULL, step, TRUE);
  while (children)
    {
      page = children->data;
      children = btk_notebook_search_page (notebook, children,
					   step, TRUE);
      if (!btk_widget_get_visible (page->child))
	continue;
      if (!btk_widget_get_mapped (page->tab_label))
	showarrow = TRUE;
      else if (page != notebook->cur_page)
	btk_notebook_draw_tab (notebook, page, area);
    }

  if (showarrow && notebook->scrollable) 
    {
      if (notebook->has_before_previous)
	btk_notebook_draw_arrow (notebook, ARROW_LEFT_BEFORE);
      if (notebook->has_before_next)
	btk_notebook_draw_arrow (notebook, ARROW_RIGHT_BEFORE);
      if (notebook->has_after_previous)
	btk_notebook_draw_arrow (notebook, ARROW_LEFT_AFTER);
      if (notebook->has_after_next)
	btk_notebook_draw_arrow (notebook, ARROW_RIGHT_AFTER);
    }
  btk_notebook_draw_tab (notebook, notebook->cur_page, area);
}

static void
btk_notebook_draw_tab (BtkNotebook     *notebook,
		       BtkNotebookPage *page,
		       BdkRectangle    *area)
{
  BtkNotebookPrivate *priv;
  BdkRectangle child_area;
  BdkRectangle page_area;
  BtkStateType state_type;
  BtkPositionType gap_side;
  BdkWindow *window;
  BtkWidget *widget;
  
  if (!NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page) ||
      !btk_widget_get_mapped (page->tab_label) ||
      (page->allocation.width == 0) || (page->allocation.height == 0))
    return;

  widget = BTK_WIDGET (notebook);
  priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);

  if (priv->operation == DRAG_OPERATION_REORDER && page == notebook->cur_page)
    window = priv->drag_window;
  else
    window = widget->window;

  page_area.x = page->allocation.x;
  page_area.y = page->allocation.y;
  page_area.width = page->allocation.width;
  page_area.height = page->allocation.height;

  if (bdk_rectangle_intersect (&page_area, area, &child_area))
    {
      gap_side = get_tab_gap_pos (notebook);

      if (notebook->cur_page == page)
	state_type = BTK_STATE_NORMAL;
      else 
	state_type = BTK_STATE_ACTIVE;

      btk_paint_extension (widget->style, window,
			   state_type, BTK_SHADOW_OUT,
			   area, widget, "tab",
			   page_area.x, page_area.y,
			   page_area.width, page_area.height,
			   gap_side);
    }
}

static void
btk_notebook_draw_arrow (BtkNotebook      *notebook,
			 BtkNotebookArrow  nbarrow)
{
  BtkStateType state_type;
  BtkShadowType shadow_type;
  BtkWidget *widget;
  BdkRectangle arrow_rect;
  BtkArrowType arrow;
  bboolean is_rtl, left;

  widget = BTK_WIDGET (notebook);

  if (btk_widget_is_drawable (widget))
    {
      bint scroll_arrow_hlength;
      bint scroll_arrow_vlength;
      bint arrow_size;

      btk_notebook_get_arrow_rect (notebook, &arrow_rect, nbarrow);

      is_rtl = btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL;
      left = (ARROW_IS_LEFT (nbarrow) && !is_rtl) ||
             (!ARROW_IS_LEFT (nbarrow) && is_rtl); 

      btk_widget_style_get (widget,
                            "scroll-arrow-hlength", &scroll_arrow_hlength,
                            "scroll-arrow-vlength", &scroll_arrow_vlength,
                            NULL);

      if (notebook->in_child == nbarrow)
        {
          if (notebook->click_child == nbarrow)
            state_type = BTK_STATE_ACTIVE;
          else
            state_type = BTK_STATE_PRELIGHT;
        }
      else
        state_type = btk_widget_get_state (widget);

      if (notebook->click_child == nbarrow)
        shadow_type = BTK_SHADOW_IN;
      else
        shadow_type = BTK_SHADOW_OUT;

      if (notebook->focus_tab &&
	  !btk_notebook_search_page (notebook, notebook->focus_tab,
				     left ? STEP_PREV : STEP_NEXT, TRUE))
	{
	  shadow_type = BTK_SHADOW_ETCHED_IN;
	  state_type = BTK_STATE_INSENSITIVE;
	}
      
      if (notebook->tab_pos == BTK_POS_LEFT ||
	  notebook->tab_pos == BTK_POS_RIGHT)
        {
          arrow = (ARROW_IS_LEFT (nbarrow) ? BTK_ARROW_UP : BTK_ARROW_DOWN);
          arrow_size = scroll_arrow_vlength;
        }
      else
        {
          arrow = (ARROW_IS_LEFT (nbarrow) ? BTK_ARROW_LEFT : BTK_ARROW_RIGHT);
          arrow_size = scroll_arrow_hlength;
        }
     
      btk_paint_arrow (widget->style, widget->window, state_type, 
		       shadow_type, NULL, widget, "notebook",
		       arrow, TRUE, arrow_rect.x, arrow_rect.y, 
		       arrow_size, arrow_size);
    }
}

/* Private BtkNotebook Size Allocate Functions:
 *
 * btk_notebook_tab_space
 * btk_notebook_calculate_shown_tabs
 * btk_notebook_calculate_tabs_allocation
 * btk_notebook_pages_allocate
 * btk_notebook_page_allocate
 * btk_notebook_calc_tabs
 */
static void
btk_notebook_tab_space (BtkNotebook *notebook,
			bboolean    *show_arrows,
			bint        *min,
			bint        *max,
			bint        *tab_space)
{
  BtkNotebookPrivate *priv;
  BtkWidget *widget;
  GList *children;
  bint tab_pos = get_effective_tab_pos (notebook);
  bint tab_overlap;
  bint arrow_spacing;
  bint scroll_arrow_hlength;
  bint scroll_arrow_vlength;
  bboolean is_rtl;
  bint i;

  widget = BTK_WIDGET (notebook);
  priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);
  children = notebook->children;
  is_rtl = btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL;

  btk_widget_style_get (BTK_WIDGET (notebook),
                        "arrow-spacing", &arrow_spacing,
                        "scroll-arrow-hlength", &scroll_arrow_hlength,
                        "scroll-arrow-vlength", &scroll_arrow_vlength,
                        NULL);

  switch (tab_pos)
    {
    case BTK_POS_TOP:
    case BTK_POS_BOTTOM:
      *min = widget->allocation.x + BTK_CONTAINER (notebook)->border_width;
      *max = widget->allocation.x + widget->allocation.width - BTK_CONTAINER (notebook)->border_width;

      for (i = 0; i < N_ACTION_WIDGETS; i++)
        {
          if (priv->action_widget[i])
            {
              if ((i == ACTION_WIDGET_START && !is_rtl) ||
                  (i == ACTION_WIDGET_END && is_rtl))
                *min += priv->action_widget[i]->allocation.width + widget->style->xthickness;
              else
                *max -= priv->action_widget[i]->allocation.width + widget->style->xthickness;
            }
        }

      while (children)
	{
          BtkNotebookPage *page;

	  page = children->data;
	  children = children->next;

	  if (NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page) &&
	      btk_widget_get_visible (page->child))
	    *tab_space += page->requisition.width;
	}
      break;
    case BTK_POS_RIGHT:
    case BTK_POS_LEFT:
      *min = widget->allocation.y + BTK_CONTAINER (notebook)->border_width;
      *max = widget->allocation.y + widget->allocation.height - BTK_CONTAINER (notebook)->border_width;

      for (i = 0; i < N_ACTION_WIDGETS; i++)
        {
          if (priv->action_widget[i])
            {
              if (i == ACTION_WIDGET_START)
                *min += priv->action_widget[i]->allocation.height + widget->style->ythickness;
              else
                *max -= priv->action_widget[i]->allocation.height + widget->style->ythickness;
            }
        }

      while (children)
	{
          BtkNotebookPage *page;

	  page = children->data;
	  children = children->next;

	  if (NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page) &&
	      btk_widget_get_visible (page->child))
	    *tab_space += page->requisition.height;
	}
      break;
    }

  if (!notebook->scrollable)
    *show_arrows = FALSE;
  else
    {
      btk_widget_style_get (widget, "tab-overlap", &tab_overlap, NULL);

      switch (tab_pos)
	{
	case BTK_POS_TOP:
	case BTK_POS_BOTTOM:
	  if (*tab_space > *max - *min - tab_overlap)
	    {
	      *show_arrows = TRUE;

	      /* take arrows into account */
	      *tab_space = *max - *min - tab_overlap;

	      if (notebook->has_after_previous)
		{
		  *tab_space -= arrow_spacing + scroll_arrow_hlength;
		  *max -= arrow_spacing + scroll_arrow_hlength;
		}

	      if (notebook->has_after_next)
		{
		  *tab_space -= arrow_spacing + scroll_arrow_hlength;
		  *max -= arrow_spacing + scroll_arrow_hlength;
		}

	      if (notebook->has_before_previous)
		{
		  *tab_space -= arrow_spacing + scroll_arrow_hlength;
		  *min += arrow_spacing + scroll_arrow_hlength;
		}

	      if (notebook->has_before_next)
		{
		  *tab_space -= arrow_spacing + scroll_arrow_hlength;
		  *min += arrow_spacing + scroll_arrow_hlength;
		}
	    }
	  break;
	case BTK_POS_LEFT:
	case BTK_POS_RIGHT:
	  if (*tab_space > *max - *min - tab_overlap)
	    {
	      *show_arrows = TRUE;

	      /* take arrows into account */
	      *tab_space = *max - *min - tab_overlap;

	      if (notebook->has_after_previous || notebook->has_after_next)
		{
		  *tab_space -= arrow_spacing + scroll_arrow_vlength;
		  *max -= arrow_spacing + scroll_arrow_vlength;
		}

	      if (notebook->has_before_previous || notebook->has_before_next)
		{
		  *tab_space -= arrow_spacing + scroll_arrow_vlength;
		  *min += arrow_spacing + scroll_arrow_vlength;
		}
	    }
	  break;
	}
    }
}

static void
btk_notebook_calculate_shown_tabs (BtkNotebook *notebook,
				   bboolean     show_arrows,
				   bint         min,
				   bint         max,
				   bint         tab_space,
				   GList      **last_child,
				   bint        *n,
				   bint        *remaining_space)
{
  BtkWidget *widget;
  BtkContainer *container;
  GList *children;
  BtkNotebookPage *page;
  bint tab_pos, tab_overlap;
  
  widget = BTK_WIDGET (notebook);
  container = BTK_CONTAINER (notebook);
  btk_widget_style_get (widget, "tab-overlap", &tab_overlap, NULL);
  tab_pos = get_effective_tab_pos (notebook);

  if (show_arrows) /* first_tab <- focus_tab */
    {
      *remaining_space = tab_space;

      if (NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, notebook->cur_page) &&
	  btk_widget_get_visible (notebook->cur_page->child))
	{
	  btk_notebook_calc_tabs (notebook,
				  notebook->focus_tab,
				  &(notebook->focus_tab),
				  remaining_space, STEP_NEXT);
	}

      if (tab_space <= 0 || *remaining_space <= 0)
	{
	  /* show 1 tab */
	  notebook->first_tab = notebook->focus_tab;
	  *last_child = btk_notebook_search_page (notebook, notebook->focus_tab,
						  STEP_NEXT, TRUE);
          page = notebook->first_tab->data;
          *remaining_space = tab_space - page->requisition.width;
          *n = 1;
	}
      else
	{
	  children = NULL;

	  if (notebook->first_tab && notebook->first_tab != notebook->focus_tab)
	    {
	      /* Is first_tab really predecessor of focus_tab? */
	      page = notebook->first_tab->data;
	      if (NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page) &&
		  btk_widget_get_visible (page->child))
		for (children = notebook->focus_tab;
		     children && children != notebook->first_tab;
		     children = btk_notebook_search_page (notebook,
							  children,
							  STEP_PREV,
							  TRUE));
	    }

	  if (!children)
	    {
	      if (NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, notebook->cur_page))
		notebook->first_tab = notebook->focus_tab;
	      else
		notebook->first_tab = btk_notebook_search_page (notebook, notebook->focus_tab,
								STEP_NEXT, TRUE);
	    }
	  else
	    /* calculate shown tabs counting backwards from the focus tab */
	    btk_notebook_calc_tabs (notebook,
				    btk_notebook_search_page (notebook,
							      notebook->focus_tab,
							      STEP_PREV,
							      TRUE),
				    &(notebook->first_tab), remaining_space,
				    STEP_PREV);

	  if (*remaining_space < 0)
	    {
	      notebook->first_tab =
		btk_notebook_search_page (notebook, notebook->first_tab,
					  STEP_NEXT, TRUE);
	      if (!notebook->first_tab)
		notebook->first_tab = notebook->focus_tab;

	      *last_child = btk_notebook_search_page (notebook, notebook->focus_tab,
						      STEP_NEXT, TRUE); 
	    }
	  else /* focus_tab -> end */   
	    {
	      if (!notebook->first_tab)
		notebook->first_tab = btk_notebook_search_page (notebook,
								NULL,
								STEP_NEXT,
								TRUE);
	      children = NULL;
	      btk_notebook_calc_tabs (notebook,
				      btk_notebook_search_page (notebook,
								notebook->focus_tab,
								STEP_NEXT,
								TRUE),
				      &children, remaining_space, STEP_NEXT);

	      if (*remaining_space <= 0) 
		*last_child = children;
	      else /* start <- first_tab */
		{
		  *last_child = NULL;
		  children = NULL;

		  btk_notebook_calc_tabs (notebook,
					  btk_notebook_search_page (notebook,
								    notebook->first_tab,
								    STEP_PREV,
								    TRUE),
					  &children, remaining_space, STEP_PREV);

		  if (*remaining_space == 0)
		    notebook->first_tab = children;
		  else
		    notebook->first_tab = btk_notebook_search_page(notebook,
								   children,
								   STEP_NEXT,
								   TRUE);
		}
	    }

          if (*remaining_space < 0)
            {
              /* calculate number of tabs */
              *remaining_space = - (*remaining_space);
              *n = 0;

              for (children = notebook->first_tab;
                   children && children != *last_child;
                   children = btk_notebook_search_page (notebook, children,
                                                        STEP_NEXT, TRUE))
                (*n)++;
	    }
          else
	    *remaining_space = 0;
        }

      /* unmap all non-visible tabs */
      for (children = btk_notebook_search_page (notebook, NULL,
						STEP_NEXT, TRUE);
	   children && children != notebook->first_tab;
	   children = btk_notebook_search_page (notebook, children,
						STEP_NEXT, TRUE))
	{
	  page = children->data;

	  if (page->tab_label &&
	      NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page))
	    btk_widget_set_child_visible (page->tab_label, FALSE);
	}

      for (children = *last_child; children;
	   children = btk_notebook_search_page (notebook, children,
						STEP_NEXT, TRUE))
	{
	  page = children->data;

	  if (page->tab_label &&
	      NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page))
	    btk_widget_set_child_visible (page->tab_label, FALSE);
	}
    }
  else /* !show_arrows */
    {
      bint c = 0;
      *n = 0;

      *remaining_space = max - min - tab_overlap - tab_space;
      children = notebook->children;
      notebook->first_tab = btk_notebook_search_page (notebook, NULL,
						      STEP_NEXT, TRUE);
      while (children)
	{
	  page = children->data;
	  children = children->next;

	  if (!NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page) ||
	      !btk_widget_get_visible (page->child))
	    continue;

	  c++;

	  if (page->expand)
	    (*n)++;
	}

      /* if notebook is homogeneous, all tabs are expanded */
      if (notebook->homogeneous && *n)
	*n = c;
    }
}

static bboolean
get_allocate_at_bottom (BtkWidget *widget,
			bint       search_direction)
{
  bboolean is_rtl = (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL);
  bboolean tab_pos = get_effective_tab_pos (BTK_NOTEBOOK (widget));

  switch (tab_pos)
    {
    case BTK_POS_TOP:
    case BTK_POS_BOTTOM:
      if (!is_rtl)
	return (search_direction == STEP_PREV);
      else
	return (search_direction == STEP_NEXT);

      break;
    case BTK_POS_RIGHT:
    case BTK_POS_LEFT:
      return (search_direction == STEP_PREV);
      break;
    }

  return FALSE;
}

static void
btk_notebook_calculate_tabs_allocation (BtkNotebook  *notebook,
					GList       **children,
					GList        *last_child,
					bboolean      showarrow,
					bint          direction,
					bint         *remaining_space,
					bint         *expanded_tabs,
					bint          min,
					bint          max)
{
  BtkWidget *widget;
  BtkContainer *container;
  BtkNotebookPrivate *priv;
  BtkNotebookPage *page;
  bboolean allocate_at_bottom;
  bint tab_overlap, tab_pos, tab_extra_space;
  bint left_x, right_x, top_y, bottom_y, anchor;
  bint xthickness, ythickness;
  bboolean gap_left, packing_changed;
  BtkAllocation child_allocation = { 0, };

  widget = BTK_WIDGET (notebook);
  container = BTK_CONTAINER (notebook);
  priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);
  btk_widget_style_get (widget, "tab-overlap", &tab_overlap, NULL);
  tab_pos = get_effective_tab_pos (notebook);
  allocate_at_bottom = get_allocate_at_bottom (widget, direction);
  anchor = 0;

  child_allocation.x = widget->allocation.x + container->border_width;
  child_allocation.y = widget->allocation.y + container->border_width;

  xthickness = widget->style->xthickness;
  ythickness = widget->style->ythickness;

  switch (tab_pos)
    {
    case BTK_POS_BOTTOM:
      child_allocation.y = widget->allocation.y + widget->allocation.height -
	notebook->cur_page->requisition.height - container->border_width;
      /* fall through */
    case BTK_POS_TOP:
      child_allocation.x = (allocate_at_bottom) ? max : min;
      child_allocation.height = notebook->cur_page->requisition.height;
      anchor = child_allocation.x;
      break;
      
    case BTK_POS_RIGHT:
      child_allocation.x = widget->allocation.x + widget->allocation.width -
	notebook->cur_page->requisition.width - container->border_width;
      /* fall through */
    case BTK_POS_LEFT:
      child_allocation.y = (allocate_at_bottom) ? max : min;
      child_allocation.width = notebook->cur_page->requisition.width;
      anchor = child_allocation.y;
      break;
    }

  left_x   = CLAMP (priv->mouse_x - priv->drag_offset_x,
		    min, max - notebook->cur_page->allocation.width);
  top_y    = CLAMP (priv->mouse_y - priv->drag_offset_y,
		    min, max - notebook->cur_page->allocation.height);
  right_x  = left_x + notebook->cur_page->allocation.width;
  bottom_y = top_y + notebook->cur_page->allocation.height;
  gap_left = packing_changed = FALSE;

  while (*children && *children != last_child)
    {
      page = (*children)->data;

      if (direction == STEP_NEXT && page->pack != BTK_PACK_START)
	{
	  if (!showarrow)
	    break;
	  else if (priv->operation == DRAG_OPERATION_REORDER)
	    packing_changed = TRUE;
	}

      if (direction == STEP_NEXT)
	*children = btk_notebook_search_page (notebook, *children, direction, TRUE);
      else
	{
	  *children = (*children)->next;

          if (page->pack != BTK_PACK_END || !btk_widget_get_visible (page->child))
	    continue;
	}

      if (!NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page))
	continue;

      tab_extra_space = 0;
      if (*expanded_tabs && (showarrow || page->expand || notebook->homogeneous))
	{
	  tab_extra_space = *remaining_space / *expanded_tabs;
	  *remaining_space -= tab_extra_space;
	  (*expanded_tabs)--;
	}

      switch (tab_pos)
	{
	case BTK_POS_TOP:
	case BTK_POS_BOTTOM:
	  child_allocation.width = page->requisition.width + tab_overlap + tab_extra_space;

	  /* make sure that the reordered tab doesn't go past the last position */
	  if (priv->operation == DRAG_OPERATION_REORDER &&
	      !gap_left && packing_changed)
	    {
	      if (!allocate_at_bottom)
		{
		  if ((notebook->cur_page->pack == BTK_PACK_START && left_x >= anchor) ||
		      (notebook->cur_page->pack == BTK_PACK_END && left_x < anchor))
		    {
		      left_x = priv->drag_window_x = anchor;
		      anchor += notebook->cur_page->allocation.width - tab_overlap;
		    }
		}
	      else
		{
		  if ((notebook->cur_page->pack == BTK_PACK_START && right_x <= anchor) ||
		      (notebook->cur_page->pack == BTK_PACK_END && right_x > anchor))
		    {
		      anchor -= notebook->cur_page->allocation.width;
		      left_x = priv->drag_window_x = anchor;
		      anchor += tab_overlap;
		    }
		}

	      gap_left = TRUE;
	    }

	  if (priv->operation == DRAG_OPERATION_REORDER && page == notebook->cur_page)
	    {
	      priv->drag_window_x = left_x;
	      priv->drag_window_y = child_allocation.y;
	    }
 	  else
 	    {
 	      if (allocate_at_bottom)
		anchor -= child_allocation.width;
 
 	      if (priv->operation == DRAG_OPERATION_REORDER && page->pack == notebook->cur_page->pack)
 		{
 		  if (!allocate_at_bottom &&
 		      left_x >= anchor &&
 		      left_x <= anchor + child_allocation.width / 2)
 		    anchor += notebook->cur_page->allocation.width - tab_overlap;
 		  else if (allocate_at_bottom &&
 			   right_x >= anchor + child_allocation.width / 2 &&
 			   right_x <= anchor + child_allocation.width)
 		    anchor -= notebook->cur_page->allocation.width - tab_overlap;
 		}

	      child_allocation.x = anchor;
 	    }

	  break;
	case BTK_POS_LEFT:
	case BTK_POS_RIGHT:
	  child_allocation.height = page->requisition.height + tab_overlap + tab_extra_space;

	  /* make sure that the reordered tab doesn't go past the last position */
	  if (priv->operation == DRAG_OPERATION_REORDER &&
	      !gap_left && packing_changed)
	    {
	      if (!allocate_at_bottom &&
		  ((notebook->cur_page->pack == BTK_PACK_START && top_y >= anchor) ||
		   (notebook->cur_page->pack == BTK_PACK_END && top_y < anchor)))
		{
		  top_y = priv->drag_window_y = anchor;
		  anchor += notebook->cur_page->allocation.height - tab_overlap;
		}
 
	      gap_left = TRUE;
	    }

	  if (priv->operation == DRAG_OPERATION_REORDER && page == notebook->cur_page)
	    {
	      priv->drag_window_x = child_allocation.x;
	      priv->drag_window_y = top_y;
	    }
 	  else
 	    {
	      if (allocate_at_bottom)
		anchor -= child_allocation.height;

 	      if (priv->operation == DRAG_OPERATION_REORDER && page->pack == notebook->cur_page->pack)
		{
		  if (!allocate_at_bottom &&
		      top_y >= anchor &&
		      top_y <= anchor + child_allocation.height / 2)
		    anchor += notebook->cur_page->allocation.height - tab_overlap;
		  else if (allocate_at_bottom &&
			   bottom_y >= anchor + child_allocation.height / 2 &&
			   bottom_y <= anchor + child_allocation.height)
		    anchor -= notebook->cur_page->allocation.height - tab_overlap;
		}

	      child_allocation.y = anchor;
 	    }

	  break;
	}

      page->allocation = child_allocation;

      if ((page == priv->detached_tab && priv->operation == DRAG_OPERATION_DETACH) ||
	  (page == notebook->cur_page && priv->operation == DRAG_OPERATION_REORDER))
	{
	  /* needs to be allocated at 0,0
	   * to be shown in the drag window */
	  page->allocation.x = 0;
	  page->allocation.y = 0;
	}
      
      if (page != notebook->cur_page)
	{
	  switch (tab_pos)
	    {
	    case BTK_POS_TOP:
	      page->allocation.y += ythickness;
	      /* fall through */
	    case BTK_POS_BOTTOM:
	      page->allocation.height = MAX (1, page->allocation.height - ythickness);
	      break;
	    case BTK_POS_LEFT:
	      page->allocation.x += xthickness;
	      /* fall through */
	    case BTK_POS_RIGHT:
	      page->allocation.width = MAX (1, page->allocation.width - xthickness);
	      break;
	    }
	}

      /* calculate whether to leave a gap based on reorder operation or not */
      switch (tab_pos)
	{
	case BTK_POS_TOP:
	case BTK_POS_BOTTOM:
 	  if (priv->operation != DRAG_OPERATION_REORDER ||
	      (priv->operation == DRAG_OPERATION_REORDER && page != notebook->cur_page))
 	    {
 	      if (priv->operation == DRAG_OPERATION_REORDER)
 		{
 		  if (page->pack == notebook->cur_page->pack &&
 		      !allocate_at_bottom &&
 		      left_x >  anchor + child_allocation.width / 2 &&
 		      left_x <= anchor + child_allocation.width)
 		    anchor += notebook->cur_page->allocation.width - tab_overlap;
 		  else if (page->pack == notebook->cur_page->pack &&
 			   allocate_at_bottom &&
 			   right_x >= anchor &&
 			   right_x <= anchor + child_allocation.width / 2)
 		    anchor -= notebook->cur_page->allocation.width - tab_overlap;
 		}
 
 	      if (!allocate_at_bottom)
 		anchor += child_allocation.width - tab_overlap;
 	      else
 		anchor += tab_overlap;
 	    }

	  break;
	case BTK_POS_LEFT:
	case BTK_POS_RIGHT:
 	  if (priv->operation != DRAG_OPERATION_REORDER  ||
	      (priv->operation == DRAG_OPERATION_REORDER && page != notebook->cur_page))
 	    {
 	      if (priv->operation == DRAG_OPERATION_REORDER)
		{
		  if (page->pack == notebook->cur_page->pack &&
		      !allocate_at_bottom &&
		      top_y >= anchor + child_allocation.height / 2 &&
		      top_y <= anchor + child_allocation.height)
		    anchor += notebook->cur_page->allocation.height - tab_overlap;
		  else if (page->pack == notebook->cur_page->pack &&
			   allocate_at_bottom &&
			   bottom_y >= anchor &&
			   bottom_y <= anchor + child_allocation.height / 2)
		    anchor -= notebook->cur_page->allocation.height - tab_overlap;
		}

	      if (!allocate_at_bottom)
		anchor += child_allocation.height - tab_overlap;
	      else
		anchor += tab_overlap;
 	    }

	  break;
	}

      /* set child visible */
      if (page->tab_label)
	btk_widget_set_child_visible (page->tab_label, TRUE);
    }

  /* Don't move the current tab past the last position during tabs reordering */
  if (children &&
      priv->operation == DRAG_OPERATION_REORDER &&
      ((direction == STEP_NEXT && notebook->cur_page->pack == BTK_PACK_START) ||
       ((direction == STEP_PREV || packing_changed) && notebook->cur_page->pack == BTK_PACK_END)))
    {
      switch (tab_pos)
	{
	case BTK_POS_TOP:
	case BTK_POS_BOTTOM:
	  if (allocate_at_bottom)
	    anchor -= notebook->cur_page->allocation.width;

	  if ((!allocate_at_bottom && priv->drag_window_x > anchor) ||
	      (allocate_at_bottom && priv->drag_window_x < anchor))
	    priv->drag_window_x = anchor;
	  break;
	case BTK_POS_LEFT:
	case BTK_POS_RIGHT:
	  if (allocate_at_bottom)
	    anchor -= notebook->cur_page->allocation.height;

	  if ((!allocate_at_bottom && priv->drag_window_y > anchor) ||
	      (allocate_at_bottom && priv->drag_window_y < anchor))
	    priv->drag_window_y = anchor;
	  break;
	}
    }
}

static void
btk_notebook_pages_allocate (BtkNotebook *notebook)
{
  GList *children = NULL;
  GList *last_child = NULL;
  bboolean showarrow = FALSE;
  bint tab_space, min, max, remaining_space;
  bint expanded_tabs, operation;
  bboolean tab_allocations_changed = FALSE;

  if (!notebook->show_tabs || !notebook->children || !notebook->cur_page)
    return;

  min = max = tab_space = remaining_space = 0;
  expanded_tabs = 1;

  btk_notebook_tab_space (notebook, &showarrow,
			  &min, &max, &tab_space);

  btk_notebook_calculate_shown_tabs (notebook, showarrow,
				     min, max, tab_space, &last_child,
				     &expanded_tabs, &remaining_space);

  children = notebook->first_tab;
  btk_notebook_calculate_tabs_allocation (notebook, &children, last_child,
					  showarrow, STEP_NEXT,
					  &remaining_space, &expanded_tabs, min, max);
  if (children && children != last_child)
    {
      children = notebook->children;
      btk_notebook_calculate_tabs_allocation (notebook, &children, last_child,
					      showarrow, STEP_PREV,
					      &remaining_space, &expanded_tabs, min, max);
    }

  children = notebook->children;

  while (children)
    {
      if (btk_notebook_page_allocate (notebook, BTK_NOTEBOOK_PAGE (children)))
	tab_allocations_changed = TRUE;
      children = children->next;
    }

  operation = BTK_NOTEBOOK_GET_PRIVATE (notebook)->operation;

  if (!notebook->first_tab)
    notebook->first_tab = notebook->children;

  if (tab_allocations_changed)
    btk_notebook_redraw_tabs (notebook);
}

static bboolean
btk_notebook_page_allocate (BtkNotebook     *notebook,
			    BtkNotebookPage *page)
{
  BtkWidget *widget = BTK_WIDGET (notebook);
  BtkAllocation child_allocation;
  BtkRequisition tab_requisition;
  bint xthickness;
  bint ythickness;
  bint padding;
  bint focus_width;
  bint tab_curvature;
  bint tab_pos = get_effective_tab_pos (notebook);
  bboolean tab_allocation_changed;
  bboolean was_visible = page->tab_allocated_visible;

  if (!page->tab_label ||
      !btk_widget_get_visible (page->tab_label) ||
      !btk_widget_get_child_visible (page->tab_label))
    {
      page->tab_allocated_visible = FALSE;
      return was_visible;
    }

  xthickness = widget->style->xthickness;
  ythickness = widget->style->ythickness;

  btk_widget_get_child_requisition (page->tab_label, &tab_requisition);
  btk_widget_style_get (widget,
			"focus-line-width", &focus_width,
			"tab-curvature", &tab_curvature,
			NULL);
  switch (tab_pos)
    {
    case BTK_POS_TOP:
    case BTK_POS_BOTTOM:
      padding = tab_curvature + focus_width + notebook->tab_hborder;
      if (page->fill)
	{
	  child_allocation.x = xthickness + focus_width + notebook->tab_hborder;
	  child_allocation.width = MAX (1, page->allocation.width - 2 * child_allocation.x);
	  child_allocation.x += page->allocation.x;
	}
      else
	{
	  child_allocation.x = page->allocation.x +
	    (page->allocation.width - tab_requisition.width) / 2;

	  child_allocation.width = tab_requisition.width;
	}

      child_allocation.y = notebook->tab_vborder + focus_width + page->allocation.y;

      if (tab_pos == BTK_POS_TOP)
	child_allocation.y += ythickness;

      child_allocation.height = MAX (1, (page->allocation.height - ythickness -
					 2 * (notebook->tab_vborder + focus_width)));
      break;
    case BTK_POS_LEFT:
    case BTK_POS_RIGHT:
      padding = tab_curvature + focus_width + notebook->tab_vborder;
      if (page->fill)
	{
	  child_allocation.y = ythickness + padding;
	  child_allocation.height = MAX (1, (page->allocation.height -
					     2 * child_allocation.y));
	  child_allocation.y += page->allocation.y;
	}
      else
	{
	  child_allocation.y = page->allocation.y +
	    (page->allocation.height - tab_requisition.height) / 2;

	  child_allocation.height = tab_requisition.height;
	}

      child_allocation.x = notebook->tab_hborder + focus_width + page->allocation.x;

      if (tab_pos == BTK_POS_LEFT)
	child_allocation.x += xthickness;

      child_allocation.width = MAX (1, (page->allocation.width - xthickness -
					2 * (notebook->tab_hborder + focus_width)));
      break;
    }

  tab_allocation_changed = (child_allocation.x != page->tab_label->allocation.x ||
			    child_allocation.y != page->tab_label->allocation.y ||
			    child_allocation.width != page->tab_label->allocation.width ||
			    child_allocation.height != page->tab_label->allocation.height);

  btk_widget_size_allocate (page->tab_label, &child_allocation);

  if (!was_visible)
    {
      page->tab_allocated_visible = TRUE;
      tab_allocation_changed = TRUE;
    }

  return tab_allocation_changed;
}

static void 
btk_notebook_calc_tabs (BtkNotebook  *notebook,
			GList        *start,
                        GList       **end,
			bint         *tab_space,
                        buint         direction)
{
  BtkNotebookPage *page = NULL;
  GList *children;
  GList *last_list = NULL;
  GList *last_calculated_child = NULL;
  bboolean pack;
  bint tab_pos = get_effective_tab_pos (notebook);
  buint real_direction;

  if (!start)
    return;

  children = start;
  pack = BTK_NOTEBOOK_PAGE (start)->pack;
  if (pack == BTK_PACK_END)
    real_direction = (direction == STEP_PREV) ? STEP_NEXT : STEP_PREV;
  else
    real_direction = direction;

  while (1)
    {
      switch (tab_pos)
	{
	case BTK_POS_TOP:
	case BTK_POS_BOTTOM:
	  while (children)
	    {
	      page = children->data;
	      if (NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page) &&
		  btk_widget_get_visible (page->child))
		{
		  if (page->pack == pack)
		    {
		      *tab_space -= page->requisition.width;
		      if (*tab_space < 0 || children == *end)
			{
			  if (*tab_space < 0) 
			    {
			      *tab_space = - (*tab_space +
					      page->requisition.width);

			      if (*tab_space == 0 && direction == STEP_PREV)
				children = last_calculated_child;

			      *end = children;
			    }
			  return;
			}

		      last_calculated_child = children;
		    }
		  last_list = children;
		}
	      if (real_direction == STEP_NEXT)
		children = children->next;
	      else
		children = children->prev;
	    }
	  break;
	case BTK_POS_LEFT:
	case BTK_POS_RIGHT:
	  while (children)
	    {
	      page = children->data;
	      if (NOTEBOOK_IS_TAB_LABEL_PARENT (notebook, page) &&
		  btk_widget_get_visible (page->child))
		{
		  if (page->pack == pack)
		    {
		      *tab_space -= page->requisition.height;
		      if (*tab_space < 0 || children == *end)
			{
			  if (*tab_space < 0)
			    {
			      *tab_space = - (*tab_space +
					      page->requisition.height);

			      if (*tab_space == 0 && direction == STEP_PREV)
				children = last_calculated_child;

			      *end = children;
			    }
			  return;
			}

		      last_calculated_child = children;
		    }
		  last_list = children;
		}
	      if (real_direction == STEP_NEXT)
		children = children->next;
	      else
		children = children->prev;
	    }
	  break;
	}
      if (real_direction == STEP_PREV)
	return;
      pack = (pack == BTK_PACK_END) ? BTK_PACK_START : BTK_PACK_END;
      real_direction = STEP_PREV;
      children = last_list;
    }
}

static void
btk_notebook_update_tab_states (BtkNotebook *notebook)
{
  GList *list;

  for (list = notebook->children; list != NULL; list = list->next)
    {
      BtkNotebookPage *page = list->data;
      
      if (page->tab_label)
	{
	  if (page == notebook->cur_page)
	    btk_widget_set_state (page->tab_label, BTK_STATE_NORMAL);
	  else
	    btk_widget_set_state (page->tab_label, BTK_STATE_ACTIVE);
	}
    }
}

/* Private BtkNotebook Page Switch Methods:
 *
 * btk_notebook_real_switch_page
 */
static void
btk_notebook_real_switch_page (BtkNotebook     *notebook,
			       BtkNotebookPage* child,
			       buint            page_num)
{
  GList *list = btk_notebook_find_child (notebook, BTK_WIDGET (child), NULL);
  BtkNotebookPage *page = BTK_NOTEBOOK_PAGE (list);
  bboolean child_has_focus;

  if (notebook->cur_page == page || !btk_widget_get_visible (BTK_WIDGET (child)))
    return;

  /* save the value here, changing visibility changes focus */
  child_has_focus = notebook->child_has_focus;

  if (notebook->cur_page)
    btk_widget_set_child_visible (notebook->cur_page->child, FALSE);

  notebook->cur_page = page;

  if (!notebook->focus_tab ||
      notebook->focus_tab->data != (bpointer) notebook->cur_page)
    notebook->focus_tab = 
      g_list_find (notebook->children, notebook->cur_page);

  btk_widget_set_child_visible (notebook->cur_page->child, TRUE);

  /* If the focus was on the previous page, move it to the first
   * element on the new page, if possible, or if not, to the
   * notebook itself.
   */
  if (child_has_focus)
    {
      if (notebook->cur_page->last_focus_child &&
	  btk_widget_is_ancestor (notebook->cur_page->last_focus_child, notebook->cur_page->child))
	btk_widget_grab_focus (notebook->cur_page->last_focus_child);
      else
	if (!btk_widget_child_focus (notebook->cur_page->child, BTK_DIR_TAB_FORWARD))
	  btk_widget_grab_focus (BTK_WIDGET (notebook));
    }
  
  btk_notebook_update_tab_states (notebook);
  btk_widget_queue_resize (BTK_WIDGET (notebook));
  g_object_notify (B_OBJECT (notebook), "page");
}

/* Private BtkNotebook Page Switch Functions:
 *
 * btk_notebook_switch_page
 * btk_notebook_page_select
 * btk_notebook_switch_focus_tab
 * btk_notebook_menu_switch_page
 */
static void
btk_notebook_switch_page (BtkNotebook     *notebook,
			  BtkNotebookPage *page)
{ 
  buint page_num;

  if (notebook->cur_page == page)
    return;

  page_num = g_list_index (notebook->children, page);

  g_signal_emit (notebook,
		 notebook_signals[SWITCH_PAGE],
		 0,
		 page->child,
		 page_num);
}

static bint
btk_notebook_page_select (BtkNotebook *notebook,
			  bboolean     move_focus)
{
  BtkNotebookPage *page;
  BtkDirectionType dir = BTK_DIR_DOWN; /* Quiet GCC */
  bint tab_pos = get_effective_tab_pos (notebook);

  if (!notebook->focus_tab)
    return FALSE;

  page = notebook->focus_tab->data;
  btk_notebook_switch_page (notebook, page);

  if (move_focus)
    {
      switch (tab_pos)
	{
	case BTK_POS_TOP:
	  dir = BTK_DIR_DOWN;
	  break;
	case BTK_POS_BOTTOM:
	  dir = BTK_DIR_UP;
	  break;
	case BTK_POS_LEFT:
	  dir = BTK_DIR_RIGHT;
	  break;
	case BTK_POS_RIGHT:
	  dir = BTK_DIR_LEFT;
	  break;
	}

      if (btk_widget_child_focus (page->child, dir))
        return TRUE;
    }
  return FALSE;
}

static void
btk_notebook_switch_focus_tab (BtkNotebook *notebook, 
			       GList       *new_child)
{
  GList *old_child;
  BtkNotebookPage *page;

  if (notebook->focus_tab == new_child)
    return;

  old_child = notebook->focus_tab;
  notebook->focus_tab = new_child;

  if (notebook->scrollable)
    btk_notebook_redraw_arrows (notebook);

  if (!notebook->show_tabs || !notebook->focus_tab)
    return;

  page = notebook->focus_tab->data;
  if (btk_widget_get_mapped (page->tab_label))
    btk_notebook_redraw_tabs (notebook);
  else
    btk_notebook_pages_allocate (notebook);

  btk_notebook_switch_page (notebook, page);
}

static void
btk_notebook_menu_switch_page (BtkWidget       *widget,
			       BtkNotebookPage *page)
{
  BtkNotebook *notebook;
  GList *children;
  buint page_num;

  notebook = BTK_NOTEBOOK (btk_menu_get_attach_widget
			   (BTK_MENU (widget->parent)));

  if (notebook->cur_page == page)
    return;

  page_num = 0;
  children = notebook->children;
  while (children && children->data != page)
    {
      children = children->next;
      page_num++;
    }

  g_signal_emit (notebook,
		 notebook_signals[SWITCH_PAGE],
		 0,
		 page->child,
		 page_num);
}

/* Private BtkNotebook Menu Functions:
 *
 * btk_notebook_menu_item_create
 * btk_notebook_menu_label_unparent
 * btk_notebook_menu_detacher
 */
static void
btk_notebook_menu_item_create (BtkNotebook *notebook, 
			       GList       *list)
{	
  BtkNotebookPage *page;
  BtkWidget *menu_item;

  page = list->data;
  if (page->default_menu)
    {
      if (BTK_IS_LABEL (page->tab_label))
	page->menu_label = btk_label_new (BTK_LABEL (page->tab_label)->label);
      else
	page->menu_label = btk_label_new ("");
      btk_misc_set_alignment (BTK_MISC (page->menu_label), 0.0, 0.5);
    }

  btk_widget_show (page->menu_label);
  menu_item = btk_menu_item_new ();
  btk_container_add (BTK_CONTAINER (menu_item), page->menu_label);
  btk_menu_shell_insert (BTK_MENU_SHELL (notebook->menu), menu_item,
			 btk_notebook_real_page_position (notebook, list));
  g_signal_connect (menu_item, "activate",
		    G_CALLBACK (btk_notebook_menu_switch_page), page);
  if (btk_widget_get_visible (page->child))
    btk_widget_show (menu_item);
}

static void
btk_notebook_menu_label_unparent (BtkWidget *widget, 
				  bpointer  data)
{
  btk_widget_unparent (BTK_BIN (widget)->child);
  BTK_BIN (widget)->child = NULL;
}

static void
btk_notebook_menu_detacher (BtkWidget *widget,
			    BtkMenu   *menu)
{
  BtkNotebook *notebook;

  notebook = BTK_NOTEBOOK (widget);
  g_return_if_fail (notebook->menu == (BtkWidget*) menu);

  notebook->menu = NULL;
}

/* Private BtkNotebook Setter Functions:
 *
 * btk_notebook_set_homogeneous_tabs_internal
 * btk_notebook_set_tab_border_internal
 * btk_notebook_set_tab_hborder_internal
 * btk_notebook_set_tab_vborder_internal
 */
static void
btk_notebook_set_homogeneous_tabs_internal (BtkNotebook *notebook,
				            bboolean     homogeneous)
{
  if (homogeneous == notebook->homogeneous)
    return;

  notebook->homogeneous = homogeneous;
  btk_widget_queue_resize (BTK_WIDGET (notebook));

  g_object_notify (B_OBJECT (notebook), "homogeneous");
}

static void
btk_notebook_set_tab_border_internal (BtkNotebook *notebook,
				      buint        border_width)
{
  notebook->tab_hborder = border_width;
  notebook->tab_vborder = border_width;

  if (notebook->show_tabs &&
      btk_widget_get_visible (BTK_WIDGET (notebook)))
    btk_widget_queue_resize (BTK_WIDGET (notebook));

  g_object_freeze_notify (B_OBJECT (notebook));
  g_object_notify (B_OBJECT (notebook), "tab-hborder");
  g_object_notify (B_OBJECT (notebook), "tab-vborder");
  g_object_thaw_notify (B_OBJECT (notebook));
}

static void
btk_notebook_set_tab_hborder_internal (BtkNotebook *notebook,
				       buint        tab_hborder)
{
  if (notebook->tab_hborder == tab_hborder)
    return;

  notebook->tab_hborder = tab_hborder;

  if (notebook->show_tabs &&
      btk_widget_get_visible (BTK_WIDGET (notebook)))
    btk_widget_queue_resize (BTK_WIDGET (notebook));

  g_object_notify (B_OBJECT (notebook), "tab-hborder");
}

static void
btk_notebook_set_tab_vborder_internal (BtkNotebook *notebook,
				       buint        tab_vborder)
{
  if (notebook->tab_vborder == tab_vborder)
    return;

  notebook->tab_vborder = tab_vborder;

  if (notebook->show_tabs &&
      btk_widget_get_visible (BTK_WIDGET (notebook)))
    btk_widget_queue_resize (BTK_WIDGET (notebook));

  g_object_notify (B_OBJECT (notebook), "tab-vborder");
}

/* Public BtkNotebook Page Insert/Remove Methods :
 *
 * btk_notebook_append_page
 * btk_notebook_append_page_menu
 * btk_notebook_prepend_page
 * btk_notebook_prepend_page_menu
 * btk_notebook_insert_page
 * btk_notebook_insert_page_menu
 * btk_notebook_remove_page
 */
/**
 * btk_notebook_append_page:
 * @notebook: a #BtkNotebook
 * @child: the #BtkWidget to use as the contents of the page.
 * @tab_label: (allow-none): the #BtkWidget to be used as the label for the page,
 *             or %NULL to use the default label, 'page N'.
 *
 * Appends a page to @notebook.
 *
 * Return value: the index (starting from 0) of the appended
 * page in the notebook, or -1 if function fails
 **/
bint
btk_notebook_append_page (BtkNotebook *notebook,
			  BtkWidget   *child,
			  BtkWidget   *tab_label)
{
  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), -1);
  g_return_val_if_fail (BTK_IS_WIDGET (child), -1);
  g_return_val_if_fail (tab_label == NULL || BTK_IS_WIDGET (tab_label), -1);
  
  return btk_notebook_insert_page_menu (notebook, child, tab_label, NULL, -1);
}

/**
 * btk_notebook_append_page_menu:
 * @notebook: a #BtkNotebook
 * @child: the #BtkWidget to use as the contents of the page.
 * @tab_label: (allow-none): the #BtkWidget to be used as the label for the page,
 *             or %NULL to use the default label, 'page N'.
 * @menu_label: (allow-none): the widget to use as a label for the page-switch
 *              menu, if that is enabled. If %NULL, and @tab_label
 *              is a #BtkLabel or %NULL, then the menu label will be
 *              a newly created label with the same text as @tab_label;
 *              If @tab_label is not a #BtkLabel, @menu_label must be
 *              specified if the page-switch menu is to be used.
 * 
 * Appends a page to @notebook, specifying the widget to use as the
 * label in the popup menu.
 *
 * Return value: the index (starting from 0) of the appended
 * page in the notebook, or -1 if function fails
 **/
bint
btk_notebook_append_page_menu (BtkNotebook *notebook,
			       BtkWidget   *child,
			       BtkWidget   *tab_label,
			       BtkWidget   *menu_label)
{
  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), -1);
  g_return_val_if_fail (BTK_IS_WIDGET (child), -1);
  g_return_val_if_fail (tab_label == NULL || BTK_IS_WIDGET (tab_label), -1);
  g_return_val_if_fail (menu_label == NULL || BTK_IS_WIDGET (menu_label), -1);
  
  return btk_notebook_insert_page_menu (notebook, child, tab_label, menu_label, -1);
}

/**
 * btk_notebook_prepend_page:
 * @notebook: a #BtkNotebook
 * @child: the #BtkWidget to use as the contents of the page.
 * @tab_label: (allow-none): the #BtkWidget to be used as the label for the page,
 *             or %NULL to use the default label, 'page N'.
 *
 * Prepends a page to @notebook.
 *
 * Return value: the index (starting from 0) of the prepended
 * page in the notebook, or -1 if function fails
 **/
bint
btk_notebook_prepend_page (BtkNotebook *notebook,
			   BtkWidget   *child,
			   BtkWidget   *tab_label)
{
  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), -1);
  g_return_val_if_fail (BTK_IS_WIDGET (child), -1);
  g_return_val_if_fail (tab_label == NULL || BTK_IS_WIDGET (tab_label), -1);
  
  return btk_notebook_insert_page_menu (notebook, child, tab_label, NULL, 0);
}

/**
 * btk_notebook_prepend_page_menu:
 * @notebook: a #BtkNotebook
 * @child: the #BtkWidget to use as the contents of the page.
 * @tab_label: (allow-none): the #BtkWidget to be used as the label for the page,
 *             or %NULL to use the default label, 'page N'.
 * @menu_label: (allow-none): the widget to use as a label for the page-switch
 *              menu, if that is enabled. If %NULL, and @tab_label
 *              is a #BtkLabel or %NULL, then the menu label will be
 *              a newly created label with the same text as @tab_label;
 *              If @tab_label is not a #BtkLabel, @menu_label must be
 *              specified if the page-switch menu is to be used.
 * 
 * Prepends a page to @notebook, specifying the widget to use as the
 * label in the popup menu.
 *
 * Return value: the index (starting from 0) of the prepended
 * page in the notebook, or -1 if function fails
 **/
bint
btk_notebook_prepend_page_menu (BtkNotebook *notebook,
				BtkWidget   *child,
				BtkWidget   *tab_label,
				BtkWidget   *menu_label)
{
  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), -1);
  g_return_val_if_fail (BTK_IS_WIDGET (child), -1);
  g_return_val_if_fail (tab_label == NULL || BTK_IS_WIDGET (tab_label), -1);
  g_return_val_if_fail (menu_label == NULL || BTK_IS_WIDGET (menu_label), -1);
  
  return btk_notebook_insert_page_menu (notebook, child, tab_label, menu_label, 0);
}

/**
 * btk_notebook_insert_page:
 * @notebook: a #BtkNotebook
 * @child: the #BtkWidget to use as the contents of the page.
 * @tab_label: (allow-none): the #BtkWidget to be used as the label for the page,
 *             or %NULL to use the default label, 'page N'.
 * @position: the index (starting at 0) at which to insert the page,
 *            or -1 to append the page after all other pages.
 *
 * Insert a page into @notebook at the given position.
 *
 * Return value: the index (starting from 0) of the inserted
 * page in the notebook, or -1 if function fails
 **/
bint
btk_notebook_insert_page (BtkNotebook *notebook,
			  BtkWidget   *child,
			  BtkWidget   *tab_label,
			  bint         position)
{
  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), -1);
  g_return_val_if_fail (BTK_IS_WIDGET (child), -1);
  g_return_val_if_fail (tab_label == NULL || BTK_IS_WIDGET (tab_label), -1);
  
  return btk_notebook_insert_page_menu (notebook, child, tab_label, NULL, position);
}


static bint
btk_notebook_page_compare_tab (gconstpointer a,
			       gconstpointer b)
{
  return (((BtkNotebookPage *) a)->tab_label != b);
}

static bboolean
btk_notebook_mnemonic_activate_switch_page (BtkWidget *child,
					    bboolean overload,
					    bpointer data)
{
  BtkNotebook *notebook = BTK_NOTEBOOK (data);
  GList *list;
  
  list = g_list_find_custom (notebook->children, child,
			     btk_notebook_page_compare_tab);
  if (list)
    {
      BtkNotebookPage *page = list->data;

      btk_widget_grab_focus (BTK_WIDGET (notebook));	/* Do this first to avoid focusing new page */
      btk_notebook_switch_page (notebook, page);
      focus_tabs_in (notebook);
    }

  return TRUE;
}

/**
 * btk_notebook_insert_page_menu:
 * @notebook: a #BtkNotebook
 * @child: the #BtkWidget to use as the contents of the page.
 * @tab_label: (allow-none): the #BtkWidget to be used as the label for the page,
 *             or %NULL to use the default label, 'page N'.
 * @menu_label: (allow-none): the widget to use as a label for the page-switch
 *              menu, if that is enabled. If %NULL, and @tab_label
 *              is a #BtkLabel or %NULL, then the menu label will be
 *              a newly created label with the same text as @tab_label;
 *              If @tab_label is not a #BtkLabel, @menu_label must be
 *              specified if the page-switch menu is to be used.
 * @position: the index (starting at 0) at which to insert the page,
 *            or -1 to append the page after all other pages.
 * 
 * Insert a page into @notebook at the given position, specifying
 * the widget to use as the label in the popup menu.
 *
 * Return value: the index (starting from 0) of the inserted
 * page in the notebook
 **/
bint
btk_notebook_insert_page_menu (BtkNotebook *notebook,
			       BtkWidget   *child,
			       BtkWidget   *tab_label,
			       BtkWidget   *menu_label,
			       bint         position)
{
  BtkNotebookClass *class;

  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), -1);
  g_return_val_if_fail (BTK_IS_WIDGET (child), -1);
  g_return_val_if_fail (tab_label == NULL || BTK_IS_WIDGET (tab_label), -1);
  g_return_val_if_fail (menu_label == NULL || BTK_IS_WIDGET (menu_label), -1);

  class = BTK_NOTEBOOK_GET_CLASS (notebook);

  return (class->insert_page) (notebook, child, tab_label, menu_label, position);
}

/**
 * btk_notebook_remove_page:
 * @notebook: a #BtkNotebook.
 * @page_num: the index of a notebook page, starting
 *            from 0. If -1, the last page will
 *            be removed.
 * 
 * Removes a page from the notebook given its index
 * in the notebook.
 **/
void
btk_notebook_remove_page (BtkNotebook *notebook,
			  bint         page_num)
{
  GList *list = NULL;

  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));

  if (page_num >= 0)
    list = g_list_nth (notebook->children, page_num);
  else
    list = g_list_last (notebook->children);

  if (list)
    btk_container_remove (BTK_CONTAINER (notebook),
			  ((BtkNotebookPage *) list->data)->child);
}

/* Public BtkNotebook Page Switch Methods :
 * btk_notebook_get_current_page
 * btk_notebook_page_num
 * btk_notebook_set_current_page
 * btk_notebook_next_page
 * btk_notebook_prev_page
 */
/**
 * btk_notebook_get_current_page:
 * @notebook: a #BtkNotebook
 * 
 * Returns the page number of the current page.
 * 
 * Return value: the index (starting from 0) of the current
 * page in the notebook. If the notebook has no pages, then
 * -1 will be returned.
 **/
bint
btk_notebook_get_current_page (BtkNotebook *notebook)
{
  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), -1);

  if (!notebook->cur_page)
    return -1;

  return g_list_index (notebook->children, notebook->cur_page);
}

/**
 * btk_notebook_get_nth_page:
 * @notebook: a #BtkNotebook
 * @page_num: the index of a page in the notebook, or -1
 *            to get the last page.
 * 
 * Returns the child widget contained in page number @page_num.
 *
 * Return value: (transfer none): the child widget, or %NULL if @page_num is
 * out of bounds.
 **/
BtkWidget*
btk_notebook_get_nth_page (BtkNotebook *notebook,
			   bint         page_num)
{
  BtkNotebookPage *page;
  GList *list;

  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), NULL);

  if (page_num >= 0)
    list = g_list_nth (notebook->children, page_num);
  else
    list = g_list_last (notebook->children);

  if (list)
    {
      page = list->data;
      return page->child;
    }

  return NULL;
}

/**
 * btk_notebook_get_n_pages:
 * @notebook: a #BtkNotebook
 * 
 * Gets the number of pages in a notebook.
 * 
 * Return value: the number of pages in the notebook.
 *
 * Since: 2.2
 **/
bint
btk_notebook_get_n_pages (BtkNotebook *notebook)
{
  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), 0);

  return g_list_length (notebook->children);
}

/**
 * btk_notebook_page_num:
 * @notebook: a #BtkNotebook
 * @child: a #BtkWidget
 * 
 * Finds the index of the page which contains the given child
 * widget.
 * 
 * Return value: the index of the page containing @child, or
 *   -1 if @child is not in the notebook.
 **/
bint
btk_notebook_page_num (BtkNotebook      *notebook,
		       BtkWidget        *child)
{
  GList *children;
  bint num;

  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), -1);

  num = 0;
  children = notebook->children;
  while (children)
    {
      BtkNotebookPage *page =  children->data;
      
      if (page->child == child)
	return num;

      children = children->next;
      num++;
    }

  return -1;
}

/**
 * btk_notebook_set_current_page:
 * @notebook: a #BtkNotebook
 * @page_num: index of the page to switch to, starting from 0.
 *            If negative, the last page will be used. If greater
 *            than the number of pages in the notebook, nothing
 *            will be done.
 *                
 * Switches to the page number @page_num. 
 *
 * Note that due to historical reasons, BtkNotebook refuses
 * to switch to a page unless the child widget is visible. 
 * Therefore, it is recommended to show child widgets before
 * adding them to a notebook. 
 */
void
btk_notebook_set_current_page (BtkNotebook *notebook,
			       bint         page_num)
{
  GList *list;

  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));

  if (page_num < 0)
    page_num = g_list_length (notebook->children) - 1;

  list = g_list_nth (notebook->children, page_num);
  if (list)
    btk_notebook_switch_page (notebook, BTK_NOTEBOOK_PAGE (list));
}

/**
 * btk_notebook_next_page:
 * @notebook: a #BtkNotebook
 * 
 * Switches to the next page. Nothing happens if the current page is
 * the last page.
 **/
void
btk_notebook_next_page (BtkNotebook *notebook)
{
  GList *list;

  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));

  list = g_list_find (notebook->children, notebook->cur_page);
  if (!list)
    return;

  list = btk_notebook_search_page (notebook, list, STEP_NEXT, TRUE);
  if (!list)
    return;

  btk_notebook_switch_page (notebook, BTK_NOTEBOOK_PAGE (list));
}

/**
 * btk_notebook_prev_page:
 * @notebook: a #BtkNotebook
 * 
 * Switches to the previous page. Nothing happens if the current page
 * is the first page.
 **/
void
btk_notebook_prev_page (BtkNotebook *notebook)
{
  GList *list;

  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));

  list = g_list_find (notebook->children, notebook->cur_page);
  if (!list)
    return;

  list = btk_notebook_search_page (notebook, list, STEP_PREV, TRUE);
  if (!list)
    return;

  btk_notebook_switch_page (notebook, BTK_NOTEBOOK_PAGE (list));
}

/* Public BtkNotebook/Tab Style Functions
 *
 * btk_notebook_set_show_border
 * btk_notebook_get_show_border
 * btk_notebook_set_show_tabs
 * btk_notebook_get_show_tabs
 * btk_notebook_set_tab_pos
 * btk_notebook_get_tab_pos
 * btk_notebook_set_scrollable
 * btk_notebook_get_scrollable
 * btk_notebook_get_tab_hborder
 * btk_notebook_get_tab_vborder
 */
/**
 * btk_notebook_set_show_border:
 * @notebook: a #BtkNotebook
 * @show_border: %TRUE if a bevel should be drawn around the notebook.
 * 
 * Sets whether a bevel will be drawn around the notebook pages.
 * This only has a visual effect when the tabs are not shown.
 * See btk_notebook_set_show_tabs().
 **/
void
btk_notebook_set_show_border (BtkNotebook *notebook,
			      bboolean     show_border)
{
  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));

  if (notebook->show_border != show_border)
    {
      notebook->show_border = show_border;

      if (btk_widget_get_visible (BTK_WIDGET (notebook)))
	btk_widget_queue_resize (BTK_WIDGET (notebook));
      
      g_object_notify (B_OBJECT (notebook), "show-border");
    }
}

/**
 * btk_notebook_get_show_border:
 * @notebook: a #BtkNotebook
 *
 * Returns whether a bevel will be drawn around the notebook pages. See
 * btk_notebook_set_show_border().
 *
 * Return value: %TRUE if the bevel is drawn
 **/
bboolean
btk_notebook_get_show_border (BtkNotebook *notebook)
{
  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), FALSE);

  return notebook->show_border;
}

/**
 * btk_notebook_set_show_tabs:
 * @notebook: a #BtkNotebook
 * @show_tabs: %TRUE if the tabs should be shown.
 * 
 * Sets whether to show the tabs for the notebook or not.
 **/
void
btk_notebook_set_show_tabs (BtkNotebook *notebook,
			    bboolean     show_tabs)
{
  BtkNotebookPrivate *priv;
  BtkNotebookPage *page;
  GList *children;
  bint i;

  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));

  priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);

  show_tabs = show_tabs != FALSE;

  if (notebook->show_tabs == show_tabs)
    return;

  notebook->show_tabs = show_tabs;
  children = notebook->children;

  if (!show_tabs)
    {
      btk_widget_set_can_focus (BTK_WIDGET (notebook), FALSE);

      while (children)
	{
	  page = children->data;
	  children = children->next;
	  if (page->default_tab)
	    {
	      btk_widget_destroy (page->tab_label);
	      page->tab_label = NULL;
	    }
	  else
	    btk_widget_hide (page->tab_label);
	}
    }
  else
    {
      btk_widget_set_can_focus (BTK_WIDGET (notebook), TRUE);
      btk_notebook_update_labels (notebook);
    }

  for (i = 0; i < N_ACTION_WIDGETS; i++)
    {
      if (priv->action_widget[i])
        btk_widget_set_child_visible (priv->action_widget[i], show_tabs);
    }

  btk_widget_queue_resize (BTK_WIDGET (notebook));

  g_object_notify (B_OBJECT (notebook), "show-tabs");
}

/**
 * btk_notebook_get_show_tabs:
 * @notebook: a #BtkNotebook
 *
 * Returns whether the tabs of the notebook are shown. See
 * btk_notebook_set_show_tabs().
 *
 * Return value: %TRUE if the tabs are shown
 **/
bboolean
btk_notebook_get_show_tabs (BtkNotebook *notebook)
{
  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), FALSE);

  return notebook->show_tabs;
}

/**
 * btk_notebook_set_tab_pos:
 * @notebook: a #BtkNotebook.
 * @pos: the edge to draw the tabs at.
 * 
 * Sets the edge at which the tabs for switching pages in the
 * notebook are drawn.
 **/
void
btk_notebook_set_tab_pos (BtkNotebook     *notebook,
			  BtkPositionType  pos)
{
  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));

  if (notebook->tab_pos != pos)
    {
      notebook->tab_pos = pos;
      if (btk_widget_get_visible (BTK_WIDGET (notebook)))
	btk_widget_queue_resize (BTK_WIDGET (notebook));
    }

  g_object_notify (B_OBJECT (notebook), "tab-pos");
}

/**
 * btk_notebook_get_tab_pos:
 * @notebook: a #BtkNotebook
 *
 * Gets the edge at which the tabs for switching pages in the
 * notebook are drawn.
 *
 * Return value: the edge at which the tabs are drawn
 **/
BtkPositionType
btk_notebook_get_tab_pos (BtkNotebook *notebook)
{
  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), BTK_POS_TOP);

  return notebook->tab_pos;
}

/**
 * btk_notebook_set_homogeneous_tabs:
 * @notebook: a #BtkNotebook
 * @homogeneous: %TRUE if all tabs should be the same size.
 * 
 * Sets whether the tabs must have all the same size or not.
 **/
void
btk_notebook_set_homogeneous_tabs (BtkNotebook *notebook,
				   bboolean     homogeneous)
{
  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));

  btk_notebook_set_homogeneous_tabs_internal (notebook, homogeneous);
}

/**
 * btk_notebook_set_tab_border:
 * @notebook: a #BtkNotebook
 * @border_width: width of the border around the tab labels.
 * 
 * Sets the width the border around the tab labels
 * in a notebook. This is equivalent to calling
 * btk_notebook_set_tab_hborder (@notebook, @border_width) followed
 * by btk_notebook_set_tab_vborder (@notebook, @border_width).
 **/
void
btk_notebook_set_tab_border (BtkNotebook *notebook,
			     buint        border_width)
{
  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));

  btk_notebook_set_tab_border_internal (notebook, border_width);
}

/**
 * btk_notebook_set_tab_hborder:
 * @notebook: a #BtkNotebook
 * @tab_hborder: width of the horizontal border of tab labels.
 * 
 * Sets the width of the horizontal border of tab labels.
 **/
void
btk_notebook_set_tab_hborder (BtkNotebook *notebook,
			      buint        tab_hborder)
{
  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));

  btk_notebook_set_tab_hborder_internal (notebook, tab_hborder);
}

/**
 * btk_notebook_set_tab_vborder:
 * @notebook: a #BtkNotebook
 * @tab_vborder: width of the vertical border of tab labels.
 * 
 * Sets the width of the vertical border of tab labels.
 **/
void
btk_notebook_set_tab_vborder (BtkNotebook *notebook,
			      buint        tab_vborder)
{
  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));

  btk_notebook_set_tab_vborder_internal (notebook, tab_vborder);
}

/**
 * btk_notebook_set_scrollable:
 * @notebook: a #BtkNotebook
 * @scrollable: %TRUE if scroll arrows should be added
 * 
 * Sets whether the tab label area will have arrows for scrolling if
 * there are too many tabs to fit in the area.
 **/
void
btk_notebook_set_scrollable (BtkNotebook *notebook,
			     bboolean     scrollable)
{
  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));

  scrollable = (scrollable != FALSE);

  if (scrollable != notebook->scrollable)
    {
      notebook->scrollable = scrollable;

      if (btk_widget_get_visible (BTK_WIDGET (notebook)))
	btk_widget_queue_resize (BTK_WIDGET (notebook));

      g_object_notify (B_OBJECT (notebook), "scrollable");
    }
}

/**
 * btk_notebook_get_scrollable:
 * @notebook: a #BtkNotebook
 *
 * Returns whether the tab label area has arrows for scrolling. See
 * btk_notebook_set_scrollable().
 *
 * Return value: %TRUE if arrows for scrolling are present
 **/
bboolean
btk_notebook_get_scrollable (BtkNotebook *notebook)
{
  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), FALSE);

  return notebook->scrollable;
}

/**
 * btk_notebook_get_tab_hborder:
 * @notebook: a #BtkNotebook
 *
 * Returns the horizontal width of a tab border.
 *
 * Return value: horizontal width of a tab border
 *
 * Since: 2.22
 */
buint16
btk_notebook_get_tab_hborder (BtkNotebook *notebook)
{
  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), FALSE);

  return notebook->tab_hborder;
}

/**
 * btk_notebook_get_tab_vborder:
 * @notebook: a #BtkNotebook
 *
 * Returns the vertical width of a tab border.
 *
 * Return value: vertical width of a tab border
 *
 * Since: 2.22
 */
buint16
btk_notebook_get_tab_vborder (BtkNotebook *notebook)
{
  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), FALSE);

  return notebook->tab_vborder;
}


/* Public BtkNotebook Popup Menu Methods:
 *
 * btk_notebook_popup_enable
 * btk_notebook_popup_disable
 */


/**
 * btk_notebook_popup_enable:
 * @notebook: a #BtkNotebook
 * 
 * Enables the popup menu: if the user clicks with the right mouse button on
 * the tab labels, a menu with all the pages will be popped up.
 **/
void
btk_notebook_popup_enable (BtkNotebook *notebook)
{
  GList *list;

  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));

  if (notebook->menu)
    return;

  notebook->menu = btk_menu_new ();
  for (list = btk_notebook_search_page (notebook, NULL, STEP_NEXT, FALSE);
       list;
       list = btk_notebook_search_page (notebook, list, STEP_NEXT, FALSE))
    btk_notebook_menu_item_create (notebook, list);

  btk_notebook_update_labels (notebook);
  btk_menu_attach_to_widget (BTK_MENU (notebook->menu),
			     BTK_WIDGET (notebook),
			     btk_notebook_menu_detacher);

  g_object_notify (B_OBJECT (notebook), "enable-popup");
}

/**
 * btk_notebook_popup_disable:
 * @notebook: a #BtkNotebook
 * 
 * Disables the popup menu.
 **/
void       
btk_notebook_popup_disable  (BtkNotebook *notebook)
{
  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));

  if (!notebook->menu)
    return;

  btk_container_foreach (BTK_CONTAINER (notebook->menu),
			 (BtkCallback) btk_notebook_menu_label_unparent, NULL);
  btk_widget_destroy (notebook->menu);

  g_object_notify (B_OBJECT (notebook), "enable-popup");
}

/* Public BtkNotebook Page Properties Functions:
 *
 * btk_notebook_get_tab_label
 * btk_notebook_set_tab_label
 * btk_notebook_set_tab_label_text
 * btk_notebook_get_menu_label
 * btk_notebook_set_menu_label
 * btk_notebook_set_menu_label_text
 * btk_notebook_set_tab_label_packing
 * btk_notebook_query_tab_label_packing
 * btk_notebook_get_tab_reorderable
 * btk_notebook_set_tab_reorderable
 * btk_notebook_get_tab_detachable
 * btk_notebook_set_tab_detachable
 */

/**
 * btk_notebook_get_tab_label:
 * @notebook: a #BtkNotebook
 * @child: the page
 * 
 * Returns the tab label widget for the page @child. %NULL is returned
 * if @child is not in @notebook or if no tab label has specifically
 * been set for @child.
 *
 * Return value: (transfer none): the tab label
 **/
BtkWidget *
btk_notebook_get_tab_label (BtkNotebook *notebook,
			    BtkWidget   *child)
{
  GList *list;

  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), NULL);
  g_return_val_if_fail (BTK_IS_WIDGET (child), NULL);

  list = CHECK_FIND_CHILD (notebook, child);
  if (!list)  
    return NULL;

  if (BTK_NOTEBOOK_PAGE (list)->default_tab)
    return NULL;

  return BTK_NOTEBOOK_PAGE (list)->tab_label;
}  

/**
 * btk_notebook_set_tab_label:
 * @notebook: a #BtkNotebook
 * @child: the page
 * @tab_label: (allow-none): the tab label widget to use, or %NULL for default tab
 *             label.
 *
 * Changes the tab label for @child. If %NULL is specified
 * for @tab_label, then the page will have the label 'page N'.
 **/
void
btk_notebook_set_tab_label (BtkNotebook *notebook,
			    BtkWidget   *child,
			    BtkWidget   *tab_label)
{
  BtkNotebookPage *page;
  GList *list;

  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));
  g_return_if_fail (BTK_IS_WIDGET (child));

  list = CHECK_FIND_CHILD (notebook, child);
  if (!list)  
    return;

  /* a NULL pointer indicates a default_tab setting, otherwise
   * we need to set the associated label
   */
  page = list->data;
  
  if (page->tab_label == tab_label)
    return;
  

  btk_notebook_remove_tab_label (notebook, page);
  
  if (tab_label)
    {
      page->default_tab = FALSE;
      page->tab_label = tab_label;
      btk_widget_set_parent (page->tab_label, BTK_WIDGET (notebook));
    }
  else
    {
      page->default_tab = TRUE;
      page->tab_label = NULL;

      if (notebook->show_tabs)
	{
	  bchar string[32];

	  g_snprintf (string, sizeof(string), _("Page %u"), 
		      btk_notebook_real_page_position (notebook, list));
	  page->tab_label = btk_label_new (string);
	  btk_widget_set_parent (page->tab_label, BTK_WIDGET (notebook));
	}
    }

  if (page->tab_label)
    page->mnemonic_activate_signal =
      g_signal_connect (page->tab_label,
			"mnemonic-activate",
			G_CALLBACK (btk_notebook_mnemonic_activate_switch_page),
			notebook);

  if (notebook->show_tabs && btk_widget_get_visible (child))
    {
      btk_widget_show (page->tab_label);
      btk_widget_queue_resize (BTK_WIDGET (notebook));
    }

  btk_notebook_update_tab_states (notebook);
  btk_widget_child_notify (child, "tab-label");
}

/**
 * btk_notebook_set_tab_label_text:
 * @notebook: a #BtkNotebook
 * @child: the page
 * @tab_text: the label text
 * 
 * Creates a new label and sets it as the tab label for the page
 * containing @child.
 **/
void
btk_notebook_set_tab_label_text (BtkNotebook *notebook,
				 BtkWidget   *child,
				 const bchar *tab_text)
{
  BtkWidget *tab_label = NULL;

  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));

  if (tab_text)
    tab_label = btk_label_new (tab_text);
  btk_notebook_set_tab_label (notebook, child, tab_label);
  btk_widget_child_notify (child, "tab-label");
}

/**
 * btk_notebook_get_tab_label_text:
 * @notebook: a #BtkNotebook
 * @child: a widget contained in a page of @notebook
 *
 * Retrieves the text of the tab label for the page containing
 *    @child.
 *
 * Return value: the text of the tab label, or %NULL if the
 *               tab label widget is not a #BtkLabel. The
 *               string is owned by the widget and must not
 *               be freed.
 **/
const bchar *
btk_notebook_get_tab_label_text (BtkNotebook *notebook,
				 BtkWidget   *child)
{
  BtkWidget *tab_label;

  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), NULL);
  g_return_val_if_fail (BTK_IS_WIDGET (child), NULL);

  tab_label = btk_notebook_get_tab_label (notebook, child);

  if (BTK_IS_LABEL (tab_label))
    return btk_label_get_text (BTK_LABEL (tab_label));
  else
    return NULL;
}

/**
 * btk_notebook_get_menu_label:
 * @notebook: a #BtkNotebook
 * @child: a widget contained in a page of @notebook
 *
 * Retrieves the menu label widget of the page containing @child.
 *
 * Return value: (transfer none): the menu label, or %NULL if the
 *     notebook page does not have a menu label other than the
 *     default (the tab label).
 **/
BtkWidget*
btk_notebook_get_menu_label (BtkNotebook *notebook,
			     BtkWidget   *child)
{
  GList *list;

  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), NULL);
  g_return_val_if_fail (BTK_IS_WIDGET (child), NULL);

  list = CHECK_FIND_CHILD (notebook, child);
  if (!list)
    return NULL;

  if (BTK_NOTEBOOK_PAGE (list)->default_menu)
    return NULL;

  return BTK_NOTEBOOK_PAGE (list)->menu_label;
}

/**
 * btk_notebook_set_menu_label:
 * @notebook: a #BtkNotebook
 * @child: the child widget
 * @menu_label: (allow-none): the menu label, or NULL for default
 *
 * Changes the menu label for the page containing @child.
 **/
void
btk_notebook_set_menu_label (BtkNotebook *notebook,
			     BtkWidget   *child,
			     BtkWidget   *menu_label)
{
  BtkNotebookPage *page;
  GList *list;

  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));
  g_return_if_fail (BTK_IS_WIDGET (child));

  list = CHECK_FIND_CHILD (notebook, child);
  if (!list)  
    return;

  page = list->data;
  if (page->menu_label)
    {
      if (notebook->menu)
	btk_container_remove (BTK_CONTAINER (notebook->menu), 
			      page->menu_label->parent);

      if (!page->default_menu)
	g_object_unref (page->menu_label);
    }

  if (menu_label)
    {
      page->menu_label = menu_label;
      g_object_ref_sink (page->menu_label);
      page->default_menu = FALSE;
    }
  else
    page->default_menu = TRUE;

  if (notebook->menu)
    btk_notebook_menu_item_create (notebook, list);
  btk_widget_child_notify (child, "menu-label");
}

/**
 * btk_notebook_set_menu_label_text:
 * @notebook: a #BtkNotebook
 * @child: the child widget
 * @menu_text: the label text
 * 
 * Creates a new label and sets it as the menu label of @child.
 **/
void
btk_notebook_set_menu_label_text (BtkNotebook *notebook,
				  BtkWidget   *child,
				  const bchar *menu_text)
{
  BtkWidget *menu_label = NULL;

  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));

  if (menu_text)
    {
      menu_label = btk_label_new (menu_text);
      btk_misc_set_alignment (BTK_MISC (menu_label), 0.0, 0.5);
    }
  btk_notebook_set_menu_label (notebook, child, menu_label);
  btk_widget_child_notify (child, "menu-label");
}

/**
 * btk_notebook_get_menu_label_text:
 * @notebook: a #BtkNotebook
 * @child: the child widget of a page of the notebook.
 *
 * Retrieves the text of the menu label for the page containing
 *    @child.
 *
 * Return value: the text of the tab label, or %NULL if the
 *               widget does not have a menu label other than
 *               the default menu label, or the menu label widget
 *               is not a #BtkLabel. The string is owned by
 *               the widget and must not be freed.
 **/
const bchar *
btk_notebook_get_menu_label_text (BtkNotebook *notebook,
				  BtkWidget *child)
{
  BtkWidget *menu_label;

  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), NULL);
  g_return_val_if_fail (BTK_IS_WIDGET (child), NULL);
 
  menu_label = btk_notebook_get_menu_label (notebook, child);

  if (BTK_IS_LABEL (menu_label))
    return btk_label_get_text (BTK_LABEL (menu_label));
  else
    return NULL;
}
  
/* Helper function called when pages are reordered
 */
static void
btk_notebook_child_reordered (BtkNotebook     *notebook,
			      BtkNotebookPage *page)
{
  if (notebook->menu)
    {
      BtkWidget *menu_item;
      
      menu_item = page->menu_label->parent;
      btk_container_remove (BTK_CONTAINER (menu_item), page->menu_label);
      btk_container_remove (BTK_CONTAINER (notebook->menu), menu_item);
      btk_notebook_menu_item_create (notebook, g_list_find (notebook->children, page));
    }

  btk_notebook_update_tab_states (notebook);
  btk_notebook_update_labels (notebook);
}

/**
 * btk_notebook_set_tab_label_packing:
 * @notebook: a #BtkNotebook
 * @child: the child widget
 * @expand: whether to expand the tab label or not
 * @fill: whether the tab label should fill the allocated area or not
 * @pack_type: the position of the tab label
 *
 * Sets the packing parameters for the tab label of the page
 * containing @child. See btk_box_pack_start() for the exact meaning
 * of the parameters.
 *
 * Deprecated: 2.20: Modify the #BtkNotebook:tab-expand and
 *   #BtkNotebook:tab-fill child properties instead.
 *   Modifying the packing of the tab label is a deprecated feature and
 *   shouldn't be done anymore.
 **/
void
btk_notebook_set_tab_label_packing (BtkNotebook *notebook,
				    BtkWidget   *child,
				    bboolean     expand,
				    bboolean     fill,
				    BtkPackType  pack_type)
{
  BtkNotebookPage *page;
  GList *list;

  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));
  g_return_if_fail (BTK_IS_WIDGET (child));

  list = CHECK_FIND_CHILD (notebook, child);
  if (!list)  
    return;

  page = list->data;
  expand = expand != FALSE;
  fill = fill != FALSE;
  if (page->pack == pack_type && page->expand == expand && page->fill == fill)
    return;

  btk_widget_freeze_child_notify (child);
  page->expand = expand;
  btk_widget_child_notify (child, "tab-expand");
  page->fill = fill;
  btk_widget_child_notify (child, "tab-fill");
  if (page->pack != pack_type)
    {
      page->pack = pack_type;
      btk_notebook_child_reordered (notebook, page);
    }
  btk_widget_child_notify (child, "tab-pack");
  btk_widget_child_notify (child, "position");
  if (notebook->show_tabs)
    btk_notebook_pages_allocate (notebook);
  btk_widget_thaw_child_notify (child);
}  

/**
 * btk_notebook_query_tab_label_packing:
 * @notebook: a #BtkNotebook
 * @child: the page
 * @expand: location to store the expand value (or NULL)
 * @fill: location to store the fill value (or NULL)
 * @pack_type: location to store the pack_type (or NULL)
 * 
 * Query the packing attributes for the tab label of the page
 * containing @child.
 *
 * Deprecated: 2.20: Modify the #BtkNotebook:tab-expand and
 *   #BtkNotebook:tab-fill child properties instead.
 **/
void
btk_notebook_query_tab_label_packing (BtkNotebook *notebook,
				      BtkWidget   *child,
				      bboolean    *expand,
				      bboolean    *fill,
				      BtkPackType *pack_type)
{
  GList *list;

  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));
  g_return_if_fail (BTK_IS_WIDGET (child));

  list = CHECK_FIND_CHILD (notebook, child);
  if (!list)
    return;

  if (expand)
    *expand = BTK_NOTEBOOK_PAGE (list)->expand;
  if (fill)
    *fill = BTK_NOTEBOOK_PAGE (list)->fill;
  if (pack_type)
    *pack_type = BTK_NOTEBOOK_PAGE (list)->pack;
}

/**
 * btk_notebook_reorder_child:
 * @notebook: a #BtkNotebook
 * @child: the child to move
 * @position: the new position, or -1 to move to the end
 * 
 * Reorders the page containing @child, so that it appears in position
 * @position. If @position is greater than or equal to the number of
 * children in the list or negative, @child will be moved to the end
 * of the list.
 **/
void
btk_notebook_reorder_child (BtkNotebook *notebook,
			    BtkWidget   *child,
			    bint         position)
{
  GList *list, *new_list;
  BtkNotebookPage *page;
  bint old_pos;
  bint max_pos;

  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));
  g_return_if_fail (BTK_IS_WIDGET (child));

  list = CHECK_FIND_CHILD (notebook, child);
  if (!list)
    return;

  max_pos = g_list_length (notebook->children) - 1;
  if (position < 0 || position > max_pos)
    position = max_pos;

  old_pos = g_list_position (notebook->children, list);

  if (old_pos == position)
    return;

  page = list->data;
  notebook->children = g_list_delete_link (notebook->children, list);

  notebook->children = g_list_insert (notebook->children, page, position);
  new_list = g_list_nth (notebook->children, position);

  /* Fix up GList references in BtkNotebook structure */
  if (notebook->first_tab == list)
    notebook->first_tab = new_list;
  if (notebook->focus_tab == list)
    notebook->focus_tab = new_list;

  btk_widget_freeze_child_notify (child);

  /* Move around the menu items if necessary */
  btk_notebook_child_reordered (notebook, page);
  btk_widget_child_notify (child, "tab-pack");
  btk_widget_child_notify (child, "position");

  if (notebook->show_tabs)
    btk_notebook_pages_allocate (notebook);

  btk_widget_thaw_child_notify (child);

  g_signal_emit (notebook,
		 notebook_signals[PAGE_REORDERED],
		 0,
		 child,
		 position);
}

/**
 * btk_notebook_set_window_creation_hook:
 * @func: (allow-none): the #BtkNotebookWindowCreationFunc, or %NULL
 * @data: user data for @func
 * @destroy: (allow-none): Destroy notifier for @data, or %NULL
 *
 * Installs a global function used to create a window
 * when a detached tab is dropped in an empty area.
 *
 * Since: 2.10
 *
 * Deprecated: 2.24: Use the #BtkNotebook::create-window signal instead
**/
void
btk_notebook_set_window_creation_hook (BtkNotebookWindowCreationFunc  func,
				       bpointer                       data,
                                       GDestroyNotify                 destroy)
{
  if (window_creation_hook_destroy)
    window_creation_hook_destroy (window_creation_hook_data);

  window_creation_hook = func;
  window_creation_hook_data = data;
  window_creation_hook_destroy = destroy;
}

/**
 * btk_notebook_set_group_id:
 * @notebook: a #BtkNotebook
 * @group_id: a group identificator, or -1 to unset it
 *
 * Sets an group identificator for @notebook, notebooks sharing
 * the same group identificator will be able to exchange tabs
 * via drag and drop. A notebook with group identificator -1 will
 * not be able to exchange tabs with any other notebook.
 * 
 * Since: 2.10
 * Deprecated: 2.12: use btk_notebook_set_group_name() instead.
 */
void
btk_notebook_set_group_id (BtkNotebook *notebook,
			   bint         group_id)
{
  bpointer group;

  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));

  /* add 1 to get rid of the -1/NULL difference */
  group = BINT_TO_POINTER (group_id + 1);
  btk_notebook_set_group (notebook, group);
}

/**
 * btk_notebook_set_group:
 * @notebook: a #BtkNotebook
 * @group: (allow-none): a pointer to identify the notebook group, or %NULL to unset it
 *
 * Sets a group identificator pointer for @notebook, notebooks sharing
 * the same group identificator pointer will be able to exchange tabs
 * via drag and drop. A notebook with a %NULL group identificator will
 * not be able to exchange tabs with any other notebook.
 *
 * Since: 2.12
 *
 * Deprecated: 2.24: Use btk_notebook_set_group_name() instead
 */
void
btk_notebook_set_group (BtkNotebook *notebook,
			bpointer     group)
{
  BtkNotebookPrivate *priv;

  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));

  priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);

  if (priv->group != group)
    {
      priv->group = group;
      g_object_notify (B_OBJECT (notebook), "group");
    }
}

/**
 * btk_notebook_set_group_name:
 * @notebook: a #BtkNotebook
 * @name: (allow-none): the name of the notebook group, or %NULL to unset it
 *
 * Sets a group name for @notebook.
 *
 * Notebooks with the same name will be able to exchange tabs
 * via drag and drop. A notebook with a %NULL group name will
 * not be able to exchange tabs with any other notebook.
 *
 * Since: 2.24
 */
void
btk_notebook_set_group_name (BtkNotebook *notebook,
                             const bchar *group_name)
{
  bpointer group;

  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));

  group = (bpointer)g_intern_string (group_name);
  btk_notebook_set_group (notebook, group);
  g_object_notify (B_OBJECT (notebook), "group-name");
}

/**
 * btk_notebook_get_group_id:
 * @notebook: a #BtkNotebook
 * 
 * Gets the current group identificator for @notebook.
 * 
 * Return Value: the group identificator, or -1 if none is set.
 *
 * Since: 2.10
 * Deprecated: 2.12: use btk_notebook_get_group_name() instead.
 */
bint
btk_notebook_get_group_id (BtkNotebook *notebook)
{
  BtkNotebookPrivate *priv;

  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), -1);

  priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);

  /* substract 1 to get rid of the -1/NULL difference */
  return BPOINTER_TO_INT (priv->group) - 1;
}


/**
 * btk_notebook_get_group:
 * @notebook: a #BtkNotebook
 *
 * Gets the current group identificator pointer for @notebook.
 *
 * Return Value: (transfer none): the group identificator,
 *     or %NULL if none is set.
 *
 * Since: 2.12
 *
 * Deprecated: 2.24: Use btk_notebook_get_group_name() instead
 **/
bpointer
btk_notebook_get_group (BtkNotebook *notebook)
{
  BtkNotebookPrivate *priv;

  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), NULL);

  priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);
  return priv->group;
}

/**
 * btk_notebook_get_group_name:
 * @notebook: a #BtkNotebook
 *
 * Gets the current group name for @notebook.
 *
 * Note that this funtion can emphasis not be used
 * together with btk_notebook_set_group() or
 * btk_notebook_set_group_id().
 *
 Return Value: (transfer none): the group name,
 *     or %NULL if none is set.
 *
 * Since: 2.24
 */
const bchar *
btk_notebook_get_group_name (BtkNotebook *notebook)
{
  BtkNotebookPrivate *priv;
  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), NULL);

  priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);
  return (const bchar *)priv->group;
}

/**
 * btk_notebook_get_tab_reorderable:
 * @notebook: a #BtkNotebook
 * @child: a child #BtkWidget
 * 
 * Gets whether the tab can be reordered via drag and drop or not.
 * 
 * Return Value: %TRUE if the tab is reorderable.
 * 
 * Since: 2.10
 **/
bboolean
btk_notebook_get_tab_reorderable (BtkNotebook *notebook,
				  BtkWidget   *child)
{
  GList *list;

  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), FALSE);
  g_return_val_if_fail (BTK_IS_WIDGET (child), FALSE);

  list = CHECK_FIND_CHILD (notebook, child);
  if (!list)  
    return FALSE;

  return BTK_NOTEBOOK_PAGE (list)->reorderable;
}

/**
 * btk_notebook_set_tab_reorderable:
 * @notebook: a #BtkNotebook
 * @child: a child #BtkWidget
 * @reorderable: whether the tab is reorderable or not.
 *
 * Sets whether the notebook tab can be reordered
 * via drag and drop or not.
 * 
 * Since: 2.10
 **/
void
btk_notebook_set_tab_reorderable (BtkNotebook *notebook,
				  BtkWidget   *child,
				  bboolean     reorderable)
{
  GList *list;

  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));
  g_return_if_fail (BTK_IS_WIDGET (child));

  list = CHECK_FIND_CHILD (notebook, child);
  if (!list)  
    return;

  if (BTK_NOTEBOOK_PAGE (list)->reorderable != reorderable)
    {
      BTK_NOTEBOOK_PAGE (list)->reorderable = (reorderable == TRUE);
      btk_widget_child_notify (child, "reorderable");
    }
}

/**
 * btk_notebook_get_tab_detachable:
 * @notebook: a #BtkNotebook
 * @child: a child #BtkWidget
 * 
 * Returns whether the tab contents can be detached from @notebook.
 * 
 * Return Value: TRUE if the tab is detachable.
 *
 * Since: 2.10
 **/
bboolean
btk_notebook_get_tab_detachable (BtkNotebook *notebook,
				 BtkWidget   *child)
{
  GList *list;

  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), FALSE);
  g_return_val_if_fail (BTK_IS_WIDGET (child), FALSE);

  list = CHECK_FIND_CHILD (notebook, child);
  if (!list)  
    return FALSE;

  return BTK_NOTEBOOK_PAGE (list)->detachable;
}

/**
 * btk_notebook_set_tab_detachable:
 * @notebook: a #BtkNotebook
 * @child: a child #BtkWidget
 * @detachable: whether the tab is detachable or not
 *
 * Sets whether the tab can be detached from @notebook to another
 * notebook or widget.
 *
 * Note that 2 notebooks must share a common group identificator
 * (see btk_notebook_set_group_id ()) to allow automatic tabs
 * interchange between them.
 *
 * If you want a widget to interact with a notebook through DnD
 * (i.e.: accept dragged tabs from it) it must be set as a drop
 * destination and accept the target "BTK_NOTEBOOK_TAB". The notebook
 * will fill the selection with a BtkWidget** pointing to the child
 * widget that corresponds to the dropped tab.
 * |[
 *  static void
 *  on_drop_zone_drag_data_received (BtkWidget        *widget,
 *                                   BdkDragContext   *context,
 *                                   bint              x,
 *                                   bint              y,
 *                                   BtkSelectionData *selection_data,
 *                                   buint             info,
 *                                   buint             time,
 *                                   bpointer          user_data)
 *  {
 *    BtkWidget *notebook;
 *    BtkWidget **child;
 *    
 *    notebook = btk_drag_get_source_widget (context);
 *    child = (void*) selection_data->data;
 *    
 *    process_widget (*child);
 *    btk_container_remove (BTK_CONTAINER (notebook), *child);
 *  }
 * ]|
 *
 * If you want a notebook to accept drags from other widgets,
 * you will have to set your own DnD code to do it.
 *
 * Since: 2.10
 **/
void
btk_notebook_set_tab_detachable (BtkNotebook *notebook,
				 BtkWidget  *child,
				 bboolean    detachable)
{
  GList *list;

  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));
  g_return_if_fail (BTK_IS_WIDGET (child));

  list = CHECK_FIND_CHILD (notebook, child);
  if (!list)  
    return;

  if (BTK_NOTEBOOK_PAGE (list)->detachable != detachable)
    {
      BTK_NOTEBOOK_PAGE (list)->detachable = (detachable == TRUE);
      btk_widget_child_notify (child, "detachable");
    }
}

/**
 * btk_notebook_get_action_widget:
 * @notebook: a #BtkNotebook
 * @pack_type: pack type of the action widget to receive
 *
 * Gets one of the action widgets. See btk_notebook_set_action_widget().
 *
 * Returns: (transfer none): The action widget with the given @pack_type
 *     or %NULL when this action widget has not been set
 *
 * Since: 2.20
 */
BtkWidget*
btk_notebook_get_action_widget (BtkNotebook *notebook,
                                BtkPackType  pack_type)
{
  BtkNotebookPrivate *priv;

  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), NULL);

  priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);
  return priv->action_widget[pack_type];
}

/**
 * btk_notebook_set_action_widget:
 * @notebook: a #BtkNotebook
 * @widget: a #BtkWidget
 * @pack_type: pack type of the action widget
 *
 * Sets @widget as one of the action widgets. Depending on the pack type
 * the widget will be placed before or after the tabs. You can use
 * a #BtkBox if you need to pack more than one widget on the same side.
 *
 * Note that action widgets are "internal" children of the notebook and thus
 * not included in the list returned from btk_container_foreach().
 *
 * Since: 2.20
 */
void
btk_notebook_set_action_widget (BtkNotebook *notebook,
				BtkWidget   *widget,
                                BtkPackType  pack_type)
{
  BtkNotebookPrivate *priv;

  g_return_if_fail (BTK_IS_NOTEBOOK (notebook));
  g_return_if_fail (!widget || BTK_IS_WIDGET (widget));
  g_return_if_fail (!widget || widget->parent == NULL);

  priv = BTK_NOTEBOOK_GET_PRIVATE (notebook);

  if (priv->action_widget[pack_type])
    btk_widget_unparent (priv->action_widget[pack_type]);

  priv->action_widget[pack_type] = widget;

  if (widget)
    {
      btk_widget_set_child_visible (widget, notebook->show_tabs);
      btk_widget_set_parent (widget, BTK_WIDGET (notebook));
    }

  btk_widget_queue_resize (BTK_WIDGET (notebook));
}

#define __BTK_NOTEBOOK_C__
#include "btkaliasdef.c"
