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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#define BTK_MENU_INTERNALS
#include "config.h"
#include <string.h>
#include "bdk/bdkkeysyms.h"
#include "btkaccellabel.h"
#include "btkaccelmap.h"
#include "btkbindings.h"
#include "btkcheckmenuitem.h"
#include  <bobject/gvaluecollector.h>
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkmenu.h"
#include "btktearoffmenuitem.h"
#include "btkwindow.h"
#include "btkhbox.h"
#include "btkvscrollbar.h"
#include "btksettings.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"


#define NAVIGATION_REBUNNYION_OVERSHOOT 50  /* How much the navigation rebunnyion
					 * extends below the submenu
					 */

#define MENU_SCROLL_STEP1      8
#define MENU_SCROLL_STEP2     15
#define MENU_SCROLL_FAST_ZONE  8
#define MENU_SCROLL_TIMEOUT1  50
#define MENU_SCROLL_TIMEOUT2  20

#define ATTACH_INFO_KEY "btk-menu-child-attach-info-key"
#define ATTACHED_MENUS "btk-attached-menus"

typedef struct _BtkMenuAttachData	BtkMenuAttachData;
typedef struct _BtkMenuPrivate  	BtkMenuPrivate;

struct _BtkMenuAttachData
{
  BtkWidget *attach_widget;
  BtkMenuDetachFunc detacher;
};

struct _BtkMenuPrivate 
{
  gint x;
  gint y;
  gboolean initially_pushed_in;

  /* info used for the table */
  guint *heights;
  gint heights_length;

  gint monitor_num;

  /* Cached layout information */
  gint n_rows;
  gint n_columns;

  gchar *title;

  /* Arrow states */
  BtkStateType lower_arrow_state;
  BtkStateType upper_arrow_state;

  /* navigation rebunnyion */
  int navigation_x;
  int navigation_y;
  int navigation_width;
  int navigation_height;

  guint have_layout           : 1;
  guint seen_item_enter       : 1;
  guint have_position         : 1;
  guint ignore_button_release : 1;
  guint no_toggle_size        : 1;
};

typedef struct
{
  gint left_attach;
  gint right_attach;
  gint top_attach;
  gint bottom_attach;
  gint effective_left_attach;
  gint effective_right_attach;
  gint effective_top_attach;
  gint effective_bottom_attach;
} AttachInfo;

enum {
  MOVE_SCROLL,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_ACTIVE,
  PROP_ACCEL_GROUP,
  PROP_ACCEL_PATH,
  PROP_ATTACH_WIDGET,
  PROP_TEAROFF_STATE,
  PROP_TEAROFF_TITLE,
  PROP_MONITOR,
  PROP_RESERVE_TOGGLE_SIZE
};

enum {
  CHILD_PROP_0,
  CHILD_PROP_LEFT_ATTACH,
  CHILD_PROP_RIGHT_ATTACH,
  CHILD_PROP_TOP_ATTACH,
  CHILD_PROP_BOTTOM_ATTACH
};

static void     btk_menu_set_property      (BObject          *object,
					    guint             prop_id,
					    const BValue     *value,
					    BParamSpec       *pspec);
static void     btk_menu_get_property      (BObject          *object,
					    guint             prop_id,
					    BValue           *value,
					    BParamSpec       *pspec);
static void     btk_menu_set_child_property(BtkContainer     *container,
                                            BtkWidget        *child,
                                            guint             property_id,
                                            const BValue     *value,
                                            BParamSpec       *pspec);
static void     btk_menu_get_child_property(BtkContainer     *container,
                                            BtkWidget        *child,
                                            guint             property_id,
                                            BValue           *value,
                                            BParamSpec       *pspec);
static void     btk_menu_destroy           (BtkObject        *object);
static void     btk_menu_realize           (BtkWidget        *widget);
static void     btk_menu_unrealize         (BtkWidget        *widget);
static void     btk_menu_size_request      (BtkWidget        *widget,
					    BtkRequisition   *requisition);
static void     btk_menu_size_allocate     (BtkWidget        *widget,
					    BtkAllocation    *allocation);
static void     btk_menu_paint             (BtkWidget        *widget,
					    BdkEventExpose   *expose);
static void     btk_menu_show              (BtkWidget        *widget);
static gboolean btk_menu_expose            (BtkWidget        *widget,
					    BdkEventExpose   *event);
static gboolean btk_menu_key_press         (BtkWidget        *widget,
					    BdkEventKey      *event);
static gboolean btk_menu_scroll            (BtkWidget        *widget,
					    BdkEventScroll   *event);
static gboolean btk_menu_button_press      (BtkWidget        *widget,
					    BdkEventButton   *event);
static gboolean btk_menu_button_release    (BtkWidget        *widget,
					    BdkEventButton   *event);
static gboolean btk_menu_motion_notify     (BtkWidget        *widget,
					    BdkEventMotion   *event);
static gboolean btk_menu_enter_notify      (BtkWidget        *widget,
					    BdkEventCrossing *event);
static gboolean btk_menu_leave_notify      (BtkWidget        *widget,
					    BdkEventCrossing *event);
static void     btk_menu_scroll_to         (BtkMenu          *menu,
					    gint              offset);
static void     btk_menu_grab_notify       (BtkWidget        *widget,
					    gboolean          was_grabbed);

static void     btk_menu_stop_scrolling         (BtkMenu  *menu);
static void     btk_menu_remove_scroll_timeout  (BtkMenu  *menu);
static gboolean btk_menu_scroll_timeout         (gpointer  data);
static gboolean btk_menu_scroll_timeout_initial (gpointer  data);
static void     btk_menu_start_scrolling        (BtkMenu  *menu);

static void     btk_menu_scroll_item_visible (BtkMenuShell    *menu_shell,
					      BtkWidget       *menu_item);
static void     btk_menu_select_item       (BtkMenuShell     *menu_shell,
					    BtkWidget        *menu_item);
static void     btk_menu_real_insert       (BtkMenuShell     *menu_shell,
					    BtkWidget        *child,
					    gint              position);
static void     btk_menu_scrollbar_changed (BtkAdjustment    *adjustment,
					    BtkMenu          *menu);
static void     btk_menu_handle_scrolling  (BtkMenu          *menu,
					    gint	      event_x,
					    gint	      event_y,
					    gboolean          enter,
                                            gboolean          motion);
static void     btk_menu_set_tearoff_hints (BtkMenu          *menu,
					    gint             width);
static void     btk_menu_style_set         (BtkWidget        *widget,
					    BtkStyle         *previous_style);
static gboolean btk_menu_focus             (BtkWidget        *widget,
					    BtkDirectionType direction);
static gint     btk_menu_get_popup_delay   (BtkMenuShell     *menu_shell);
static void     btk_menu_move_current      (BtkMenuShell     *menu_shell,
                                            BtkMenuDirectionType direction);
static void     btk_menu_real_move_scroll  (BtkMenu          *menu,
					    BtkScrollType     type);

static void     btk_menu_stop_navigating_submenu       (BtkMenu          *menu);
static gboolean btk_menu_stop_navigating_submenu_cb    (gpointer          user_data);
static gboolean btk_menu_navigating_submenu            (BtkMenu          *menu,
							gint              event_x,
							gint              event_y);
static void     btk_menu_set_submenu_navigation_rebunnyion (BtkMenu          *menu,
							BtkMenuItem      *menu_item,
							BdkEventCrossing *event);
 
static void btk_menu_deactivate	    (BtkMenuShell      *menu_shell);
static void btk_menu_show_all       (BtkWidget         *widget);
static void btk_menu_hide_all       (BtkWidget         *widget);
static void btk_menu_position       (BtkMenu           *menu,
                                     gboolean           set_scroll_offset);
static void btk_menu_reparent       (BtkMenu           *menu, 
				     BtkWidget         *new_parent, 
				     gboolean           unrealize);
static void btk_menu_remove         (BtkContainer      *menu,
				     BtkWidget         *widget);

static void btk_menu_update_title   (BtkMenu           *menu);

static void       menu_grab_transfer_window_destroy (BtkMenu *menu);
static BdkWindow *menu_grab_transfer_window_get     (BtkMenu *menu);

static gboolean btk_menu_real_can_activate_accel (BtkWidget *widget,
                                                  guint      signal_id);
static void _btk_menu_refresh_accel_paths (BtkMenu *menu,
					   gboolean group_changed);

static const gchar attach_data_key[] = "btk-menu-attach-data";

static guint menu_signals[LAST_SIGNAL] = { 0 };

static BtkMenuPrivate *
btk_menu_get_private (BtkMenu *menu)
{
  return B_TYPE_INSTANCE_GET_PRIVATE (menu, BTK_TYPE_MENU, BtkMenuPrivate);
}

G_DEFINE_TYPE (BtkMenu, btk_menu, BTK_TYPE_MENU_SHELL)

static void
menu_queue_resize (BtkMenu *menu)
{
  BtkMenuPrivate *priv = btk_menu_get_private (menu);

  priv->have_layout = FALSE;
  btk_widget_queue_resize (BTK_WIDGET (menu));
}

static void
attach_info_free (AttachInfo *info)
{
  g_slice_free (AttachInfo, info);
}

static AttachInfo *
get_attach_info (BtkWidget *child)
{
  BObject *object = B_OBJECT (child);
  AttachInfo *ai = g_object_get_data (object, ATTACH_INFO_KEY);

  if (!ai)
    {
      ai = g_slice_new0 (AttachInfo);
      g_object_set_data_full (object, I_(ATTACH_INFO_KEY), ai,
                              (GDestroyNotify) attach_info_free);
    }

  return ai;
}

static gboolean
is_grid_attached (AttachInfo *ai)
{
  return (ai->left_attach >= 0 &&
	  ai->right_attach >= 0 &&
	  ai->top_attach >= 0 &&
	  ai->bottom_attach >= 0);
}

static void
menu_ensure_layout (BtkMenu *menu)
{
  BtkMenuPrivate *priv = btk_menu_get_private (menu);

  if (!priv->have_layout)
    {
      BtkMenuShell *menu_shell = BTK_MENU_SHELL (menu);
      GList *l;
      gchar *row_occupied;
      gint current_row;
      gint max_right_attach;      
      gint max_bottom_attach;

      /* Find extents of gridded portion
       */
      max_right_attach = 1;
      max_bottom_attach = 0;

      for (l = menu_shell->children; l; l = l->next)
	{
	  BtkWidget *child = l->data;
	  AttachInfo *ai = get_attach_info (child);

	  if (is_grid_attached (ai))
	    {
	      max_bottom_attach = MAX (max_bottom_attach, ai->bottom_attach);
	      max_right_attach = MAX (max_right_attach, ai->right_attach);
	    }
	}
	 
      /* Find empty rows
       */
      row_occupied = g_malloc0 (max_bottom_attach);

      for (l = menu_shell->children; l; l = l->next)
	{
	  BtkWidget *child = l->data;
	  AttachInfo *ai = get_attach_info (child);

	  if (is_grid_attached (ai))
	    {
	      gint i;

	      for (i = ai->top_attach; i < ai->bottom_attach; i++)
		row_occupied[i] = TRUE;
	    }
	}

      /* Lay non-grid-items out in those rows
       */
      current_row = 0;
      for (l = menu_shell->children; l; l = l->next)
	{
	  BtkWidget *child = l->data;
	  AttachInfo *ai = get_attach_info (child);

	  if (!is_grid_attached (ai))
	    {
	      while (current_row < max_bottom_attach && row_occupied[current_row])
		current_row++;
		
	      ai->effective_left_attach = 0;
	      ai->effective_right_attach = max_right_attach;
	      ai->effective_top_attach = current_row;
	      ai->effective_bottom_attach = current_row + 1;

	      current_row++;
	    }
	  else
	    {
	      ai->effective_left_attach = ai->left_attach;
	      ai->effective_right_attach = ai->right_attach;
	      ai->effective_top_attach = ai->top_attach;
	      ai->effective_bottom_attach = ai->bottom_attach;
	    }
	}

      g_free (row_occupied);

      priv->n_rows = MAX (current_row, max_bottom_attach);
      priv->n_columns = max_right_attach;
      priv->have_layout = TRUE;
    }
}


static gint
btk_menu_get_n_columns (BtkMenu *menu)
{
  BtkMenuPrivate *priv = btk_menu_get_private (menu);

  menu_ensure_layout (menu);

  return priv->n_columns;
}

static gint
btk_menu_get_n_rows (BtkMenu *menu)
{
  BtkMenuPrivate *priv = btk_menu_get_private (menu);

  menu_ensure_layout (menu);

  return priv->n_rows;
}

static void
get_effective_child_attach (BtkWidget *child,
			    int       *l,
			    int       *r,
			    int       *t,
			    int       *b)
{
  BtkMenu *menu = BTK_MENU (child->parent);
  AttachInfo *ai;
  
  menu_ensure_layout (menu);

  ai = get_attach_info (child);

  if (l)
    *l = ai->effective_left_attach;
  if (r)
    *r = ai->effective_right_attach;
  if (t)
    *t = ai->effective_top_attach;
  if (b)
    *b = ai->effective_bottom_attach;

}

static void
btk_menu_class_init (BtkMenuClass *class)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (class);
  BtkObjectClass *object_class = BTK_OBJECT_CLASS (class);
  BtkWidgetClass *widget_class = BTK_WIDGET_CLASS (class);
  BtkContainerClass *container_class = BTK_CONTAINER_CLASS (class);
  BtkMenuShellClass *menu_shell_class = BTK_MENU_SHELL_CLASS (class);
  BtkBindingSet *binding_set;
  
  bobject_class->set_property = btk_menu_set_property;
  bobject_class->get_property = btk_menu_get_property;

  object_class->destroy = btk_menu_destroy;
  
  widget_class->realize = btk_menu_realize;
  widget_class->unrealize = btk_menu_unrealize;
  widget_class->size_request = btk_menu_size_request;
  widget_class->size_allocate = btk_menu_size_allocate;
  widget_class->show = btk_menu_show;
  widget_class->expose_event = btk_menu_expose;
  widget_class->scroll_event = btk_menu_scroll;
  widget_class->key_press_event = btk_menu_key_press;
  widget_class->button_press_event = btk_menu_button_press;
  widget_class->button_release_event = btk_menu_button_release;
  widget_class->motion_notify_event = btk_menu_motion_notify;
  widget_class->show_all = btk_menu_show_all;
  widget_class->hide_all = btk_menu_hide_all;
  widget_class->enter_notify_event = btk_menu_enter_notify;
  widget_class->leave_notify_event = btk_menu_leave_notify;
  widget_class->style_set = btk_menu_style_set;
  widget_class->focus = btk_menu_focus;
  widget_class->can_activate_accel = btk_menu_real_can_activate_accel;
  widget_class->grab_notify = btk_menu_grab_notify;

  container_class->remove = btk_menu_remove;
  container_class->get_child_property = btk_menu_get_child_property;
  container_class->set_child_property = btk_menu_set_child_property;
  
  menu_shell_class->submenu_placement = BTK_LEFT_RIGHT;
  menu_shell_class->deactivate = btk_menu_deactivate;
  menu_shell_class->select_item = btk_menu_select_item;
  menu_shell_class->insert = btk_menu_real_insert;
  menu_shell_class->get_popup_delay = btk_menu_get_popup_delay;
  menu_shell_class->move_current = btk_menu_move_current;

  menu_signals[MOVE_SCROLL] =
    g_signal_new_class_handler (I_("move-scroll"),
                                B_OBJECT_CLASS_TYPE (object_class),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (btk_menu_real_move_scroll),
                                NULL, NULL,
                                _btk_marshal_VOID__ENUM,
                                B_TYPE_NONE, 1,
                                BTK_TYPE_SCROLL_TYPE);

  /**
   * BtkMenu:active:
   *
   * The index of the currently selected menu item, or -1 if no
   * menu item is selected.
   *
   * Since: 2.14
   **/
  g_object_class_install_property (bobject_class,
                                   PROP_ACTIVE,
                                   g_param_spec_int ("active",
				                     P_("Active"),
						     P_("The currently selected menu item"),
						     -1, G_MAXINT, -1,
						     BTK_PARAM_READWRITE));

  /**
   * BtkMenu:accel-group:
   *
   * The accel group holding accelerators for the menu.
   *
   * Since: 2.14
   **/
  g_object_class_install_property (bobject_class,
                                   PROP_ACCEL_GROUP,
                                   g_param_spec_object ("accel-group",
				                        P_("Accel Group"),
						        P_("The accel group holding accelerators for the menu"),
						        BTK_TYPE_ACCEL_GROUP,
						        BTK_PARAM_READWRITE));

  /**
   * BtkMenu:accel-path:
   *
   * An accel path used to conveniently construct accel paths of child items.
   *
   * Since: 2.14
   **/
  g_object_class_install_property (bobject_class,
                                   PROP_ACCEL_PATH,
                                   g_param_spec_string ("accel-path",
				                        P_("Accel Path"),
						        P_("An accel path used to conveniently construct accel paths of child items"),
						        NULL,
						        BTK_PARAM_READWRITE));

  /**
   * BtkMenu:attach-widget:
   *
   * The widget the menu is attached to. Setting this property attaches
   * the menu without a #BtkMenuDetachFunc. If you need to use a detacher,
   * use btk_menu_attach_to_widget() directly.
   *
   * Since: 2.14
   **/
  g_object_class_install_property (bobject_class,
                                   PROP_ATTACH_WIDGET,
                                   g_param_spec_object ("attach-widget",
				                        P_("Attach Widget"),
						        P_("The widget the menu is attached to"),
						        BTK_TYPE_WIDGET,
						        BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_TEAROFF_TITLE,
                                   g_param_spec_string ("tearoff-title",
                                                        P_("Tearoff Title"),
                                                        P_("A title that may be displayed by the window manager when this menu is torn-off"),
                                                        NULL,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkMenu:tearoff-state:
   *
   * A boolean that indicates whether the menu is torn-off.
   *
   * Since: 2.6
   **/
  g_object_class_install_property (bobject_class,
                                   PROP_TEAROFF_STATE,
                                   g_param_spec_boolean ("tearoff-state",
							 P_("Tearoff State"),
							 P_("A boolean that indicates whether the menu is torn-off"),
							 FALSE,
							 BTK_PARAM_READWRITE));

  /**
   * BtkMenu:monitor:
   *
   * The monitor the menu will be popped up on.
   *
   * Since: 2.14
   **/
  g_object_class_install_property (bobject_class,
                                   PROP_MONITOR,
                                   g_param_spec_int ("monitor",
				                     P_("Monitor"),
						     P_("The monitor the menu will be popped up on"),
						     -1, G_MAXINT, -1,
						     BTK_PARAM_READWRITE));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("vertical-padding",
							     P_("Vertical Padding"),
							     P_("Extra space at the top and bottom of the menu"),
							     0,
							     G_MAXINT,
							     1,
							     BTK_PARAM_READABLE));

  /**
   * BtkMenu:reserve-toggle-size:
   *
   * A boolean that indicates whether the menu reserves space for
   * toggles and icons, regardless of their actual presence.
   *
   * This property should only be changed from its default value
   * for special-purposes such as tabular menus. Regular menus that
   * are connected to a menu bar or context menus should reserve
   * toggle space for consistency.
   *
   * Since: 2.18
   */
  g_object_class_install_property (bobject_class,
                                   PROP_RESERVE_TOGGLE_SIZE,
                                   g_param_spec_boolean ("reserve-toggle-size",
							 P_("Reserve Toggle Size"),
							 P_("A boolean that indicates whether the menu reserves space for toggles and icons"),
							 TRUE,
							 BTK_PARAM_READWRITE));

  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("horizontal-padding",
                                                             P_("Horizontal Padding"),
                                                             P_("Extra space at the left and right edges of the menu"),
                                                             0,
                                                             G_MAXINT,
                                                             0,
                                                             BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("vertical-offset",
							     P_("Vertical Offset"),
							     P_("When the menu is a submenu, position it this number of pixels offset vertically"),
							     G_MININT,
							     G_MAXINT,
							     0,
							     BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("horizontal-offset",
							     P_("Horizontal Offset"),
							     P_("When the menu is a submenu, position it this number of pixels offset horizontally"),
							     G_MININT,
							     G_MAXINT,
							     -2,
							     BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boolean ("double-arrows",
                                                                 P_("Double Arrows"),
                                                                 P_("When scrolling, always show both arrows."),
                                                                 TRUE,
                                                                 BTK_PARAM_READABLE));

  /**
   * BtkMenu:arrow-placement:
   *
   * Indicates where scroll arrows should be placed.
   *
   * Since: 2.16
   **/
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("arrow-placement",
                                                              P_("Arrow Placement"),
                                                              P_("Indicates where scroll arrows should be placed"),
                                                              BTK_TYPE_ARROW_PLACEMENT,
                                                              BTK_ARROWS_BOTH,
                                                              BTK_PARAM_READABLE));

 btk_container_class_install_child_property (container_class,
                                             CHILD_PROP_LEFT_ATTACH,
					      g_param_spec_int ("left-attach",
                                                               P_("Left Attach"),
                                                               P_("The column number to attach the left side of the child to"),
								-1, INT_MAX, -1,
                                                               BTK_PARAM_READWRITE));

 btk_container_class_install_child_property (container_class,
                                             CHILD_PROP_RIGHT_ATTACH,
					      g_param_spec_int ("right-attach",
                                                               P_("Right Attach"),
                                                               P_("The column number to attach the right side of the child to"),
								-1, INT_MAX, -1,
                                                               BTK_PARAM_READWRITE));

 btk_container_class_install_child_property (container_class,
                                             CHILD_PROP_TOP_ATTACH,
					      g_param_spec_int ("top-attach",
                                                               P_("Top Attach"),
                                                               P_("The row number to attach the top of the child to"),
								-1, INT_MAX, -1,
                                                               BTK_PARAM_READWRITE));

 btk_container_class_install_child_property (container_class,
                                             CHILD_PROP_BOTTOM_ATTACH,
					      g_param_spec_int ("bottom-attach",
                                                               P_("Bottom Attach"),
                                                               P_("The row number to attach the bottom of the child to"),
								-1, INT_MAX, -1,
                                                               BTK_PARAM_READWRITE));

 /**
  * BtkMenu::arrow-scaling
  *
  * Arbitrary constant to scale down the size of the scroll arrow.
  *
  * Since: 2.16
  */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_float ("arrow-scaling",
                                                               P_("Arrow Scaling"),
                                                               P_("Arbitrary constant to scale down the size of the scroll arrow"),
                                                               0.0, 1.0, 0.7,
                                                               BTK_PARAM_READABLE));

  binding_set = btk_binding_set_by_class (class);
  btk_binding_entry_add_signal (binding_set,
				BDK_Up, 0,
				I_("move-current"), 1,
				BTK_TYPE_MENU_DIRECTION_TYPE,
				BTK_MENU_DIR_PREV);
  btk_binding_entry_add_signal (binding_set,
				BDK_KP_Up, 0,
				"move-current", 1,
				BTK_TYPE_MENU_DIRECTION_TYPE,
				BTK_MENU_DIR_PREV);
  btk_binding_entry_add_signal (binding_set,
				BDK_Down, 0,
				"move-current", 1,
				BTK_TYPE_MENU_DIRECTION_TYPE,
				BTK_MENU_DIR_NEXT);
  btk_binding_entry_add_signal (binding_set,
				BDK_KP_Down, 0,
				"move-current", 1,
				BTK_TYPE_MENU_DIRECTION_TYPE,
				BTK_MENU_DIR_NEXT);
  btk_binding_entry_add_signal (binding_set,
				BDK_Left, 0,
				"move-current", 1,
				BTK_TYPE_MENU_DIRECTION_TYPE,
				BTK_MENU_DIR_PARENT);
  btk_binding_entry_add_signal (binding_set,
				BDK_KP_Left, 0,
				"move-current", 1,
				BTK_TYPE_MENU_DIRECTION_TYPE,
				BTK_MENU_DIR_PARENT);
  btk_binding_entry_add_signal (binding_set,
				BDK_Right, 0,
				"move-current", 1,
				BTK_TYPE_MENU_DIRECTION_TYPE,
				BTK_MENU_DIR_CHILD);
  btk_binding_entry_add_signal (binding_set,
				BDK_KP_Right, 0,
				"move-current", 1,
				BTK_TYPE_MENU_DIRECTION_TYPE,
				BTK_MENU_DIR_CHILD);
  btk_binding_entry_add_signal (binding_set,
				BDK_Home, 0,
				"move-scroll", 1,
				BTK_TYPE_SCROLL_TYPE,
				BTK_SCROLL_START);
  btk_binding_entry_add_signal (binding_set,
				BDK_KP_Home, 0,
				"move-scroll", 1,
				BTK_TYPE_SCROLL_TYPE,
				BTK_SCROLL_START);
  btk_binding_entry_add_signal (binding_set,
				BDK_End, 0,
				"move-scroll", 1,
				BTK_TYPE_SCROLL_TYPE,
				BTK_SCROLL_END);
  btk_binding_entry_add_signal (binding_set,
				BDK_KP_End, 0,
				"move-scroll", 1,
				BTK_TYPE_SCROLL_TYPE,
				BTK_SCROLL_END);
  btk_binding_entry_add_signal (binding_set,
				BDK_Page_Up, 0,
				"move-scroll", 1,
				BTK_TYPE_SCROLL_TYPE,
				BTK_SCROLL_PAGE_UP);
  btk_binding_entry_add_signal (binding_set,
				BDK_KP_Page_Up, 0,
				"move-scroll", 1,
				BTK_TYPE_SCROLL_TYPE,
				BTK_SCROLL_PAGE_UP);
  btk_binding_entry_add_signal (binding_set,
				BDK_Page_Down, 0,
				"move-scroll", 1,
				BTK_TYPE_SCROLL_TYPE,
				BTK_SCROLL_PAGE_DOWN);
  btk_binding_entry_add_signal (binding_set,
				BDK_KP_Page_Down, 0,
				"move-scroll", 1,
				BTK_TYPE_SCROLL_TYPE,
				BTK_SCROLL_PAGE_DOWN);

  g_type_class_add_private (bobject_class, sizeof (BtkMenuPrivate));
}


static void
btk_menu_set_property (BObject      *object,
		       guint         prop_id,
		       const BValue *value,
		       BParamSpec   *pspec)
{
  BtkMenu *menu = BTK_MENU (object);

  switch (prop_id)
    {
    case PROP_ACTIVE:
      btk_menu_set_active (menu, b_value_get_int (value));
      break;
    case PROP_ACCEL_GROUP:
      btk_menu_set_accel_group (menu, b_value_get_object (value));
      break;
    case PROP_ACCEL_PATH:
      btk_menu_set_accel_path (menu, b_value_get_string (value));
      break;
    case PROP_ATTACH_WIDGET:
      {
        BtkWidget *widget;

        widget = btk_menu_get_attach_widget (menu);
        if (widget)
          btk_menu_detach (menu);

        widget = (BtkWidget*) b_value_get_object (value); 
        if (widget)
          btk_menu_attach_to_widget (menu, widget, NULL);
      }
      break;
    case PROP_TEAROFF_STATE:
      btk_menu_set_tearoff_state (menu, b_value_get_boolean (value));
      break;
    case PROP_TEAROFF_TITLE:
      btk_menu_set_title (menu, b_value_get_string (value));
      break;
    case PROP_MONITOR:
      btk_menu_set_monitor (menu, b_value_get_int (value));
      break;
    case PROP_RESERVE_TOGGLE_SIZE:
      btk_menu_set_reserve_toggle_size (menu, b_value_get_boolean (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_menu_get_property (BObject     *object,
		       guint        prop_id,
		       BValue      *value,
		       BParamSpec  *pspec)
{
  BtkMenu *menu = BTK_MENU (object);

  switch (prop_id)
    {
    case PROP_ACTIVE:
      b_value_set_int (value, g_list_index (BTK_MENU_SHELL (menu)->children, btk_menu_get_active (menu)));
      break;
    case PROP_ACCEL_GROUP:
      b_value_set_object (value, btk_menu_get_accel_group (menu));
      break;
    case PROP_ACCEL_PATH:
      b_value_set_string (value, btk_menu_get_accel_path (menu));
      break;
    case PROP_ATTACH_WIDGET:
      b_value_set_object (value, btk_menu_get_attach_widget (menu));
      break;
    case PROP_TEAROFF_STATE:
      b_value_set_boolean (value, btk_menu_get_tearoff_state (menu));
      break;
    case PROP_TEAROFF_TITLE:
      b_value_set_string (value, btk_menu_get_title (menu));
      break;
    case PROP_MONITOR:
      b_value_set_int (value, btk_menu_get_monitor (menu));
      break;
    case PROP_RESERVE_TOGGLE_SIZE:
      b_value_set_boolean (value, btk_menu_get_reserve_toggle_size (menu));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_menu_set_child_property (BtkContainer *container,
                             BtkWidget    *child,
                             guint         property_id,
                             const BValue *value,
                             BParamSpec   *pspec)
{
  BtkMenu *menu = BTK_MENU (container);
  AttachInfo *ai = get_attach_info (child);

  switch (property_id)
    {
    case CHILD_PROP_LEFT_ATTACH:
      ai->left_attach = b_value_get_int (value);
      break;
    case CHILD_PROP_RIGHT_ATTACH:
      ai->right_attach = b_value_get_int (value);
      break;
    case CHILD_PROP_TOP_ATTACH:
      ai->top_attach = b_value_get_int (value);	
      break;
    case CHILD_PROP_BOTTOM_ATTACH:
      ai->bottom_attach = b_value_get_int (value);
      break;

    default:
      BTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      return;
    }

  menu_queue_resize (menu);
}

static void
btk_menu_get_child_property (BtkContainer *container,
                             BtkWidget    *child,
                             guint         property_id,
                             BValue       *value,
                             BParamSpec   *pspec)
{
  AttachInfo *ai = get_attach_info (child);

  switch (property_id)
    {
    case CHILD_PROP_LEFT_ATTACH:
      b_value_set_int (value, ai->left_attach);
      break;
    case CHILD_PROP_RIGHT_ATTACH:
      b_value_set_int (value, ai->right_attach);
      break;
    case CHILD_PROP_TOP_ATTACH:
      b_value_set_int (value, ai->top_attach);
      break;
    case CHILD_PROP_BOTTOM_ATTACH:
      b_value_set_int (value, ai->bottom_attach);
      break;
      
    default:
      BTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      return;
    }
}

static gboolean
btk_menu_window_event (BtkWidget *window,
		       BdkEvent  *event,
		       BtkWidget *menu)
{
  gboolean handled = FALSE;

  g_object_ref (window);
  g_object_ref (menu);

  switch (event->type)
    {
    case BDK_KEY_PRESS:
    case BDK_KEY_RELEASE:
      handled = btk_widget_event (menu, event);
      break;
    default:
      break;
    }

  g_object_unref (window);
  g_object_unref (menu);

  return handled;
}

static void
btk_menu_window_size_request (BtkWidget      *window,
			      BtkRequisition *requisition,
			      BtkMenu        *menu)
{
  BtkMenuPrivate *private = btk_menu_get_private (menu);

  if (private->have_position)
    {
      BdkScreen *screen = btk_widget_get_screen (window);
      BdkRectangle monitor;
      
      bdk_screen_get_monitor_geometry (screen, private->monitor_num, &monitor);

      if (private->y + requisition->height > monitor.y + monitor.height)
	requisition->height = monitor.y + monitor.height - private->y;

      if (private->y < monitor.y)
	requisition->height -= monitor.y - private->y;
    }
}

static void
btk_menu_init (BtkMenu *menu)
{
  BtkMenuPrivate *priv = btk_menu_get_private (menu);

  menu->parent_menu_item = NULL;
  menu->old_active_menu_item = NULL;
  menu->accel_group = NULL;
  menu->position_func = NULL;
  menu->position_func_data = NULL;
  menu->toggle_size = 0;

  menu->toplevel = g_object_connect (g_object_new (BTK_TYPE_WINDOW,
						   "type", BTK_WINDOW_POPUP,
						   "child", menu,
						   NULL),
				     "signal::event", btk_menu_window_event, menu,
				     "signal::size-request", btk_menu_window_size_request, menu,
				     "signal::destroy", btk_widget_destroyed, &menu->toplevel,
				     NULL);
  btk_window_set_resizable (BTK_WINDOW (menu->toplevel), FALSE);
  btk_window_set_mnemonic_modifier (BTK_WINDOW (menu->toplevel), 0);

  /* Refloat the menu, so that reference counting for the menu isn't
   * affected by it being a child of the toplevel
   */
  g_object_force_floating (B_OBJECT (menu));
  menu->needs_destruction_ref_count = TRUE;

  menu->view_window = NULL;
  menu->bin_window = NULL;

  menu->scroll_offset = 0;
  menu->scroll_step  = 0;
  menu->timeout_id = 0;
  menu->scroll_fast = FALSE;
  
  menu->tearoff_window = NULL;
  menu->tearoff_hbox = NULL;
  menu->torn_off = FALSE;
  menu->tearoff_active = FALSE;
  menu->tearoff_adjustment = NULL;
  menu->tearoff_scrollbar = NULL;

  menu->upper_arrow_visible = FALSE;
  menu->lower_arrow_visible = FALSE;
  menu->upper_arrow_prelight = FALSE;
  menu->lower_arrow_prelight = FALSE;

  priv->upper_arrow_state = BTK_STATE_NORMAL;
  priv->lower_arrow_state = BTK_STATE_NORMAL;

  priv->have_layout = FALSE;
  priv->monitor_num = -1;
}

static void
btk_menu_destroy (BtkObject *object)
{
  BtkMenu *menu = BTK_MENU (object);
  BtkMenuAttachData *data;
  BtkMenuPrivate *priv; 

  btk_menu_remove_scroll_timeout (menu);
  
  data = g_object_get_data (B_OBJECT (object), attach_data_key);
  if (data)
    btk_menu_detach (menu);
  
  btk_menu_stop_navigating_submenu (menu);

  if (menu->old_active_menu_item)
    {
      g_object_unref (menu->old_active_menu_item);
      menu->old_active_menu_item = NULL;
    }

  /* Add back the reference count for being a child */
  if (menu->needs_destruction_ref_count)
    {
      menu->needs_destruction_ref_count = FALSE;
      g_object_ref (object);
    }
  
  if (menu->accel_group)
    {
      g_object_unref (menu->accel_group);
      menu->accel_group = NULL;
    }

  if (menu->toplevel)
    btk_widget_destroy (menu->toplevel);

  if (menu->tearoff_window)
    btk_widget_destroy (menu->tearoff_window);

  priv = btk_menu_get_private (menu);

  if (priv->heights)
    {
      g_free (priv->heights);
      priv->heights = NULL;
    }

  if (priv->title)
    {
      g_free (priv->title);
      priv->title = NULL;
    }

  BTK_OBJECT_CLASS (btk_menu_parent_class)->destroy (object);
}

static void
menu_change_screen (BtkMenu   *menu,
		    BdkScreen *new_screen)
{
  BtkMenuPrivate *private = btk_menu_get_private (menu);

  if (btk_widget_has_screen (BTK_WIDGET (menu)))
    {
      if (new_screen == btk_widget_get_screen (BTK_WIDGET (menu)))
	return;
    }

  if (menu->torn_off)
    {
      btk_window_set_screen (BTK_WINDOW (menu->tearoff_window), new_screen);
      btk_menu_position (menu, TRUE);
    }

  btk_window_set_screen (BTK_WINDOW (menu->toplevel), new_screen);
  private->monitor_num = -1;
}

static void
attach_widget_screen_changed (BtkWidget *attach_widget,
			      BdkScreen *previous_screen,
			      BtkMenu   *menu)
{
  if (btk_widget_has_screen (attach_widget) &&
      !g_object_get_data (B_OBJECT (menu), "btk-menu-explicit-screen"))
    {
      menu_change_screen (menu, btk_widget_get_screen (attach_widget));
    }
}

void
btk_menu_attach_to_widget (BtkMenu	       *menu,
			   BtkWidget	       *attach_widget,
			   BtkMenuDetachFunc	detacher)
{
  BtkMenuAttachData *data;
  GList *list;
  
  g_return_if_fail (BTK_IS_MENU (menu));
  g_return_if_fail (BTK_IS_WIDGET (attach_widget));
  
  /* keep this function in sync with btk_widget_set_parent()
   */
  
  data = g_object_get_data (B_OBJECT (menu), attach_data_key);
  if (data)
    {
      g_warning ("btk_menu_attach_to_widget(): menu already attached to %s",
		 g_type_name (B_TYPE_FROM_INSTANCE (data->attach_widget)));
     return;
    }
  
  g_object_ref_sink (menu);
  
  data = g_slice_new (BtkMenuAttachData);
  data->attach_widget = attach_widget;
  
  g_signal_connect (attach_widget, "screen-changed",
		    G_CALLBACK (attach_widget_screen_changed), menu);
  attach_widget_screen_changed (attach_widget, NULL, menu);
  
  data->detacher = detacher;
  g_object_set_data (B_OBJECT (menu), I_(attach_data_key), data);
  list = g_object_steal_data (B_OBJECT (attach_widget), ATTACHED_MENUS);
  if (!g_list_find (list, menu))
    {
      list = g_list_prepend (list, menu);
    }
  g_object_set_data_full (B_OBJECT (attach_widget), I_(ATTACHED_MENUS), list,
                          (GDestroyNotify) g_list_free);

  if (btk_widget_get_state (BTK_WIDGET (menu)) != BTK_STATE_NORMAL)
    btk_widget_set_state (BTK_WIDGET (menu), BTK_STATE_NORMAL);
  
  /* we don't need to set the style here, since
   * we are a toplevel widget.
   */

  /* Fallback title for menu comes from attach widget */
  btk_menu_update_title (menu);

  g_object_notify (B_OBJECT (menu), "attach-widget");
}

/**
 * btk_menu_get_attach_widget:
 * @menu: a #BtkMenu
 *
 * Returns the #BtkWidget that the menu is attached to.
 *
 * Returns: (transfer none): the #BtkWidget that the menu is attached to
 */
BtkWidget*
btk_menu_get_attach_widget (BtkMenu *menu)
{
  BtkMenuAttachData *data;
  
  g_return_val_if_fail (BTK_IS_MENU (menu), NULL);
  
  data = g_object_get_data (B_OBJECT (menu), attach_data_key);
  if (data)
    return data->attach_widget;
  return NULL;
}

void
btk_menu_detach (BtkMenu *menu)
{
  BtkMenuAttachData *data;
  GList *list;
  
  g_return_if_fail (BTK_IS_MENU (menu));
  
  /* keep this function in sync with btk_widget_unparent()
   */
  data = g_object_get_data (B_OBJECT (menu), attach_data_key);
  if (!data)
    {
      g_warning ("btk_menu_detach(): menu is not attached");
      return;
    }
  g_object_set_data (B_OBJECT (menu), I_(attach_data_key), NULL);
  
  g_signal_handlers_disconnect_by_func (data->attach_widget,
					(gpointer) attach_widget_screen_changed,
					menu);

  if (data->detacher)
    data->detacher (data->attach_widget, menu);
  list = g_object_steal_data (B_OBJECT (data->attach_widget), ATTACHED_MENUS);
  list = g_list_remove (list, menu);
  if (list)
    g_object_set_data_full (B_OBJECT (data->attach_widget), I_(ATTACHED_MENUS), list,
                            (GDestroyNotify) g_list_free);
  else
    g_object_set_data (B_OBJECT (data->attach_widget), I_(ATTACHED_MENUS), NULL);
  
  if (btk_widget_get_realized (BTK_WIDGET (menu)))
    btk_widget_unrealize (BTK_WIDGET (menu));
  
  g_slice_free (BtkMenuAttachData, data);
  
  /* Fallback title for menu comes from attach widget */
  btk_menu_update_title (menu);

  g_object_unref (menu);
}

static void
btk_menu_remove (BtkContainer *container,
		 BtkWidget    *widget)
{
  BtkMenu *menu = BTK_MENU (container);

  g_return_if_fail (BTK_IS_MENU_ITEM (widget));

  /* Clear out old_active_menu_item if it matches the item we are removing
   */
  if (menu->old_active_menu_item == widget)
    {
      g_object_unref (menu->old_active_menu_item);
      menu->old_active_menu_item = NULL;
    }

  BTK_CONTAINER_CLASS (btk_menu_parent_class)->remove (container, widget);
  g_object_set_data (B_OBJECT (widget), I_(ATTACH_INFO_KEY), NULL);

  menu_queue_resize (menu);
}

BtkWidget*
btk_menu_new (void)
{
  return g_object_new (BTK_TYPE_MENU, NULL);
}

static void
btk_menu_real_insert (BtkMenuShell *menu_shell,
		      BtkWidget    *child,
		      gint          position)
{
  BtkMenu *menu = BTK_MENU (menu_shell);
  AttachInfo *ai = get_attach_info (child);

  ai->left_attach = -1;
  ai->right_attach = -1;
  ai->top_attach = -1;
  ai->bottom_attach = -1;

  if (btk_widget_get_realized (BTK_WIDGET (menu_shell)))
    btk_widget_set_parent_window (child, menu->bin_window);

  BTK_MENU_SHELL_CLASS (btk_menu_parent_class)->insert (menu_shell, child, position);

  menu_queue_resize (menu);
}

static void
btk_menu_tearoff_bg_copy (BtkMenu *menu)
{
  BtkWidget *widget;
  gint width, height;

  widget = BTK_WIDGET (menu);

  if (menu->torn_off)
    {
      BdkPixmap *pixmap;
      bairo_t *cr;

      menu->tearoff_active = FALSE;
      menu->saved_scroll_offset = menu->scroll_offset;
      
      width = bdk_window_get_width (menu->tearoff_window->window);
      height = bdk_window_get_height (menu->tearoff_window->window);
      
      pixmap = bdk_pixmap_new (menu->tearoff_window->window,
			       width,
			       height,
			       -1);

      cr = bdk_bairo_create (pixmap);
      /* Let's hope that function never notices we're not passing it a pixmap */
      bdk_bairo_set_source_pixmap (cr,
                                   menu->tearoff_window->window,
                                   0, 0);
      bairo_paint (cr);
      bairo_destroy (cr);

      btk_widget_set_size_request (menu->tearoff_window,
				   width,
				   height);

      bdk_window_set_back_pixmap (menu->tearoff_window->window, pixmap, FALSE);
      g_object_unref (pixmap);
    }
}

static gboolean
popup_grab_on_window (BdkWindow *window,
		      guint32    activate_time,
		      gboolean   grab_keyboard)
{
  if ((bdk_pointer_grab (window, TRUE,
			 BDK_BUTTON_PRESS_MASK | BDK_BUTTON_RELEASE_MASK |
			 BDK_ENTER_NOTIFY_MASK | BDK_LEAVE_NOTIFY_MASK |
			 BDK_POINTER_MOTION_MASK,
			 NULL, NULL, activate_time) == 0))
    {
      if (!grab_keyboard ||
	  bdk_keyboard_grab (window, TRUE,
			     activate_time) == 0)
	return TRUE;
      else
	{
	  bdk_display_pointer_ungrab (bdk_window_get_display (window),
				      activate_time);
	  return FALSE;
	}
    }

  return FALSE;
}

/**
 * btk_menu_popup:
 * @menu: a #BtkMenu.
 * @parent_menu_shell: (allow-none): the menu shell containing the triggering menu item, or %NULL
 * @parent_menu_item: (allow-none): the menu item whose activation triggered the popup, or %NULL
 * @func: (allow-none): a user supplied function used to position the menu, or %NULL
 * @data: (allow-none): user supplied data to be passed to @func.
 * @button: the mouse button which was pressed to initiate the event.
 * @activate_time: the time at which the activation event occurred.
 *
 * Displays a menu and makes it available for selection.  Applications can use
 * this function to display context-sensitive menus, and will typically supply
 * %NULL for the @parent_menu_shell, @parent_menu_item, @func and @data 
 * parameters. The default menu positioning function will position the menu
 * at the current mouse cursor position.
 *
 * The @button parameter should be the mouse button pressed to initiate
 * the menu popup. If the menu popup was initiated by something other than
 * a mouse button press, such as a mouse button release or a keypress,
 * @button should be 0.
 *
 * The @activate_time parameter is used to conflict-resolve initiation of
 * concurrent requests for mouse/keyboard grab requests. To function
 * properly, this needs to be the time stamp of the user event (such as
 * a mouse click or key press) that caused the initiation of the popup.
 * Only if no such event is available, btk_get_current_event_time() can
 * be used instead.
 */
void
btk_menu_popup (BtkMenu		    *menu,
		BtkWidget	    *parent_menu_shell,
		BtkWidget	    *parent_menu_item,
		BtkMenuPositionFunc  func,
		gpointer	     data,
		guint		     button,
		guint32		     activate_time)
{
  BtkWidget *widget;
  BtkWidget *xgrab_shell;
  BtkWidget *parent;
  BdkEvent *current_event;
  BtkMenuShell *menu_shell;
  gboolean grab_keyboard;
  BtkMenuPrivate *priv;
  BtkWidget *parent_toplevel;

  g_return_if_fail (BTK_IS_MENU (menu));

  widget = BTK_WIDGET (menu);
  menu_shell = BTK_MENU_SHELL (menu);
  priv = btk_menu_get_private (menu);

  menu_shell->parent_menu_shell = parent_menu_shell;

  priv->seen_item_enter = FALSE;
  
  /* Find the last viewable ancestor, and make an X grab on it
   */
  parent = BTK_WIDGET (menu);
  xgrab_shell = NULL;
  while (parent)
    {
      gboolean viewable = TRUE;
      BtkWidget *tmp = parent;
      
      while (tmp)
	{
	  if (!btk_widget_get_mapped (tmp))
	    {
	      viewable = FALSE;
	      break;
	    }
	  tmp = tmp->parent;
	}
      
      if (viewable)
	xgrab_shell = parent;
      
      parent = BTK_MENU_SHELL (parent)->parent_menu_shell;
    }

  /* We want to receive events generated when we map the menu; unfortunately,
   * since there is probably already an implicit grab in place from the
   * button that the user used to pop up the menu, we won't receive then --
   * in particular, the EnterNotify when the menu pops up under the pointer.
   *
   * If we are grabbing on a parent menu shell, no problem; just grab on
   * that menu shell first before popping up the window with owner_events = TRUE.
   *
   * When grabbing on the menu itself, things get more convuluted - we
   * we do an explicit grab on a specially created window with
   * owner_events = TRUE, which we override further down with a grab
   * on the menu. (We can't grab on the menu until it is mapped; we
   * probably could just leave the grab on the other window, with a
   * little reorganization of the code in btkmenu*).
   */
  grab_keyboard = btk_menu_shell_get_take_focus (menu_shell);
  btk_window_set_accept_focus (BTK_WINDOW (menu->toplevel), grab_keyboard);

  if (xgrab_shell && xgrab_shell != widget)
    {
      if (popup_grab_on_window (xgrab_shell->window, activate_time, grab_keyboard))
	BTK_MENU_SHELL (xgrab_shell)->have_xgrab = TRUE;
    }
  else
    {
      BdkWindow *transfer_window;

      xgrab_shell = widget;
      transfer_window = menu_grab_transfer_window_get (menu);
      if (popup_grab_on_window (transfer_window, activate_time, grab_keyboard))
	BTK_MENU_SHELL (xgrab_shell)->have_xgrab = TRUE;
    }

  if (!BTK_MENU_SHELL (xgrab_shell)->have_xgrab)
    {
      /* We failed to make our pointer/keyboard grab. Rather than leaving the user
       * with a stuck up window, we just abort here. Presumably the user will
       * try again.
       */
      menu_shell->parent_menu_shell = NULL;
      menu_grab_transfer_window_destroy (menu);
      return;
    }

  menu_shell->active = TRUE;
  menu_shell->button = button;

  /* If we are popping up the menu from something other than, a button
   * press then, as a heuristic, we ignore enter events for the menu
   * until we get a MOTION_NOTIFY.  
   */

  current_event = btk_get_current_event ();
  if (current_event)
    {
      if ((current_event->type != BDK_BUTTON_PRESS) &&
	  (current_event->type != BDK_ENTER_NOTIFY))
	menu_shell->ignore_enter = TRUE;

      bdk_event_free (current_event);
    }
  else
    menu_shell->ignore_enter = TRUE;

  if (menu->torn_off)
    {
      btk_menu_tearoff_bg_copy (menu);

      btk_menu_reparent (menu, menu->toplevel, FALSE);
    }

  parent_toplevel = NULL;
  if (parent_menu_shell) 
    parent_toplevel = btk_widget_get_toplevel (parent_menu_shell);
  else if (!g_object_get_data (B_OBJECT (menu), "btk-menu-explicit-screen"))
    {
      BtkWidget *attach_widget = btk_menu_get_attach_widget (menu);
      if (attach_widget)
	parent_toplevel = btk_widget_get_toplevel (attach_widget);
    }

  /* Set transient for to get the right window group and parent relationship */
  if (BTK_IS_WINDOW (parent_toplevel))
    btk_window_set_transient_for (BTK_WINDOW (menu->toplevel),
				  BTK_WINDOW (parent_toplevel));
  
  menu->parent_menu_item = parent_menu_item;
  menu->position_func = func;
  menu->position_func_data = data;
  menu_shell->activate_time = activate_time;

  /* We need to show the menu here rather in the init function because
   * code expects to be able to tell if the menu is onscreen by
   * looking at the btk_widget_get_visible (menu)
   */
  btk_widget_show (BTK_WIDGET (menu));

  /* Position the menu, possibly changing the size request
   */
  btk_menu_position (menu, TRUE);

  /* Compute the size of the toplevel and realize it so we
   * can scroll correctly.
   */
  {
    BtkRequisition tmp_request;
    BtkAllocation tmp_allocation = { 0, };

    btk_widget_size_request (menu->toplevel, &tmp_request);
    
    tmp_allocation.width = tmp_request.width;
    tmp_allocation.height = tmp_request.height;

    btk_widget_size_allocate (menu->toplevel, &tmp_allocation);
    
    btk_widget_realize (BTK_WIDGET (menu));
  }

  btk_menu_scroll_to (menu, menu->scroll_offset);

  /* if no item is selected, select the first one */
  if (!menu_shell->active_menu_item)
    {
      gboolean touchscreen_mode;

      g_object_get (btk_widget_get_settings (BTK_WIDGET (menu)),
                    "btk-touchscreen-mode", &touchscreen_mode,
                    NULL);

      if (touchscreen_mode)
        btk_menu_shell_select_first (menu_shell, TRUE);
    }

  /* Once everything is set up correctly, map the toplevel window on
     the screen.
   */
  btk_widget_show (menu->toplevel);

  if (xgrab_shell == widget)
    popup_grab_on_window (widget->window, activate_time, grab_keyboard); /* Should always succeed */
  btk_grab_add (BTK_WIDGET (menu));

  if (parent_menu_shell)
    {
      gboolean keyboard_mode;

      keyboard_mode = _btk_menu_shell_get_keyboard_mode (BTK_MENU_SHELL (parent_menu_shell));
      _btk_menu_shell_set_keyboard_mode (menu_shell, keyboard_mode);
    }
  else if (menu_shell->button == 0) /* a keynav-activated context menu */
    _btk_menu_shell_set_keyboard_mode (menu_shell, TRUE);

  _btk_menu_shell_update_mnemonics (menu_shell);
}

void
btk_menu_popdown (BtkMenu *menu)
{
  BtkMenuPrivate *private;
  BtkMenuShell *menu_shell;

  g_return_if_fail (BTK_IS_MENU (menu));
  
  menu_shell = BTK_MENU_SHELL (menu);
  private = btk_menu_get_private (menu);

  menu_shell->parent_menu_shell = NULL;
  menu_shell->active = FALSE;
  menu_shell->ignore_enter = FALSE;

  private->have_position = FALSE;

  btk_menu_stop_scrolling (menu);
  
  btk_menu_stop_navigating_submenu (menu);
  
  if (menu_shell->active_menu_item)
    {
      if (menu->old_active_menu_item)
	g_object_unref (menu->old_active_menu_item);
      menu->old_active_menu_item = menu_shell->active_menu_item;
      g_object_ref (menu->old_active_menu_item);
    }

  btk_menu_shell_deselect (menu_shell);
  
  /* The X Grab, if present, will automatically be removed when we hide
   * the window */
  btk_widget_hide (menu->toplevel);
  btk_window_set_transient_for (BTK_WINDOW (menu->toplevel), NULL);

  if (menu->torn_off)
    {
      btk_widget_set_size_request (menu->tearoff_window, -1, -1);
      
      if (BTK_BIN (menu->toplevel)->child) 
	{
	  btk_menu_reparent (menu, menu->tearoff_hbox, TRUE);
	} 
      else
	{
	  /* We popped up the menu from the tearoff, so we need to 
	   * release the grab - we aren't actually hiding the menu.
	   */
	  if (menu_shell->have_xgrab)
	    {
	      BdkDisplay *display = btk_widget_get_display (BTK_WIDGET (menu));
	      
	      bdk_display_pointer_ungrab (display, BDK_CURRENT_TIME);
	      bdk_display_keyboard_ungrab (display, BDK_CURRENT_TIME);
	    }
	}

      /* btk_menu_popdown is called each time a menu item is selected from
       * a torn off menu. Only scroll back to the saved position if the
       * non-tearoff menu was popped down.
       */
      if (!menu->tearoff_active)
	btk_menu_scroll_to (menu, menu->saved_scroll_offset);
      menu->tearoff_active = TRUE;
    }
  else
    btk_widget_hide (BTK_WIDGET (menu));

  menu_shell->have_xgrab = FALSE;
  btk_grab_remove (BTK_WIDGET (menu));

  menu_grab_transfer_window_destroy (menu);
}

/**
 * btk_menu_get_active:
 * @menu: a #BtkMenu
 *
 * Returns the selected menu item from the menu.  This is used by the
 * #BtkOptionMenu.
 *
 * Returns: (transfer none): the #BtkMenuItem that was last selected
 *          in the menu.  If a selection has not yet been made, the
 *          first menu item is selected.
 */
BtkWidget*
btk_menu_get_active (BtkMenu *menu)
{
  BtkWidget *child;
  GList *children;
  
  g_return_val_if_fail (BTK_IS_MENU (menu), NULL);
  
  if (!menu->old_active_menu_item)
    {
      child = NULL;
      children = BTK_MENU_SHELL (menu)->children;
      
      while (children)
	{
	  child = children->data;
	  children = children->next;
	  
	  if (BTK_BIN (child)->child)
	    break;
	  child = NULL;
	}
      
      menu->old_active_menu_item = child;
      if (menu->old_active_menu_item)
	g_object_ref (menu->old_active_menu_item);
    }
  
  return menu->old_active_menu_item;
}

void
btk_menu_set_active (BtkMenu *menu,
		     guint    index)
{
  BtkWidget *child;
  GList *tmp_list;
  
  g_return_if_fail (BTK_IS_MENU (menu));
  
  tmp_list = g_list_nth (BTK_MENU_SHELL (menu)->children, index);
  if (tmp_list)
    {
      child = tmp_list->data;
      if (BTK_BIN (child)->child)
	{
	  if (menu->old_active_menu_item)
	    g_object_unref (menu->old_active_menu_item);
	  menu->old_active_menu_item = child;
	  g_object_ref (menu->old_active_menu_item);
	}
    }
}


/**
 * btk_menu_set_accel_group:
 * @accel_group: (allow-none):
 */
void
btk_menu_set_accel_group (BtkMenu	*menu,
			  BtkAccelGroup *accel_group)
{
  g_return_if_fail (BTK_IS_MENU (menu));
  
  if (menu->accel_group != accel_group)
    {
      if (menu->accel_group)
	g_object_unref (menu->accel_group);
      menu->accel_group = accel_group;
      if (menu->accel_group)
	g_object_ref (menu->accel_group);
      _btk_menu_refresh_accel_paths (menu, TRUE);
    }
}

/**
 * btk_menu_get_accel_group:
 * @menu a #BtkMenu
 *
 * Gets the #BtkAccelGroup which holds global accelerators for the
 * menu.  See btk_menu_set_accel_group().
 *
 * Returns: (transfer none): the #BtkAccelGroup associated with the menu.
 */
BtkAccelGroup*
btk_menu_get_accel_group (BtkMenu *menu)
{
  g_return_val_if_fail (BTK_IS_MENU (menu), NULL);

  return menu->accel_group;
}

static gboolean
btk_menu_real_can_activate_accel (BtkWidget *widget,
                                  guint      signal_id)
{
  /* Menu items chain here to figure whether they can activate their
   * accelerators.  Unlike ordinary widgets, menus allow accel
   * activation even if invisible since that's the usual case for
   * submenus/popup-menus. however, the state of the attach widget
   * affects the "activeness" of the menu.
   */
  BtkWidget *awidget = btk_menu_get_attach_widget (BTK_MENU (widget));

  if (awidget)
    return btk_widget_can_activate_accel (awidget, signal_id);
  else
    return btk_widget_is_sensitive (widget);
}

/**
 * btk_menu_set_accel_path
 * @menu:       a valid #BtkMenu
 * @accel_path: (allow-none): a valid accelerator path
 *
 * Sets an accelerator path for this menu from which accelerator paths
 * for its immediate children, its menu items, can be constructed.
 * The main purpose of this function is to spare the programmer the
 * inconvenience of having to call btk_menu_item_set_accel_path() on
 * each menu item that should support runtime user changable accelerators.
 * Instead, by just calling btk_menu_set_accel_path() on their parent,
 * each menu item of this menu, that contains a label describing its purpose,
 * automatically gets an accel path assigned. For example, a menu containing
 * menu items "New" and "Exit", will, after 
 * <literal>btk_menu_set_accel_path (menu, "&lt;Gnumeric-Sheet&gt;/File");</literal>
 * has been called, assign its items the accel paths:
 * <literal>"&lt;Gnumeric-Sheet&gt;/File/New"</literal> and <literal>"&lt;Gnumeric-Sheet&gt;/File/Exit"</literal>.
 * Assigning accel paths to menu items then enables the user to change
 * their accelerators at runtime. More details about accelerator paths
 * and their default setups can be found at btk_accel_map_add_entry().
 * 
 * Note that @accel_path string will be stored in a #GQuark. Therefore, if you
 * pass a static string, you can save some memory by interning it first with 
 * g_intern_static_string().
 */
void
btk_menu_set_accel_path (BtkMenu     *menu,
			 const gchar *accel_path)
{
  g_return_if_fail (BTK_IS_MENU (menu));
  if (accel_path)
    g_return_if_fail (accel_path[0] == '<' && strchr (accel_path, '/')); /* simplistic check */

  /* FIXME: accel_path should be defined as const gchar* */
  menu->accel_path = (gchar*)g_intern_string (accel_path);
  if (menu->accel_path)
    _btk_menu_refresh_accel_paths (menu, FALSE);
}

/**
 * btk_menu_get_accel_path
 * @menu: a valid #BtkMenu
 *
 * Retrieves the accelerator path set on the menu.
 *
 * Returns: the accelerator path set on the menu.
 *
 * Since: 2.14
 */
const gchar*
btk_menu_get_accel_path (BtkMenu *menu)
{
  g_return_val_if_fail (BTK_IS_MENU (menu), NULL);

  return menu->accel_path;
}

typedef struct {
  BtkMenu *menu;
  gboolean group_changed;
} AccelPropagation;

static void
refresh_accel_paths_foreach (BtkWidget *widget,
			     gpointer   data)
{
  AccelPropagation *prop = data;

  if (BTK_IS_MENU_ITEM (widget))	/* should always be true */
    _btk_menu_item_refresh_accel_path (BTK_MENU_ITEM (widget),
				       prop->menu->accel_path,
				       prop->menu->accel_group,
				       prop->group_changed);
}

static void
_btk_menu_refresh_accel_paths (BtkMenu  *menu,
			       gboolean  group_changed)
{
  g_return_if_fail (BTK_IS_MENU (menu));

  if (menu->accel_path && menu->accel_group)
    {
      AccelPropagation prop;

      prop.menu = menu;
      prop.group_changed = group_changed;
      btk_container_foreach (BTK_CONTAINER (menu),
			     refresh_accel_paths_foreach,
			     &prop);
    }
}

void
btk_menu_reposition (BtkMenu *menu)
{
  g_return_if_fail (BTK_IS_MENU (menu));

  if (!menu->torn_off && btk_widget_is_drawable (BTK_WIDGET (menu)))
    btk_menu_position (menu, FALSE);
}

static void
btk_menu_scrollbar_changed (BtkAdjustment *adjustment,
			    BtkMenu       *menu)
{
  g_return_if_fail (BTK_IS_MENU (menu));

  if (adjustment->value != menu->scroll_offset)
    btk_menu_scroll_to (menu, adjustment->value);
}

static void
btk_menu_set_tearoff_hints (BtkMenu *menu,
			    gint     width)
{
  BdkGeometry geometry_hints;
  
  if (!menu->tearoff_window)
    return;

  if (btk_widget_get_visible (menu->tearoff_scrollbar))
    {
      btk_widget_size_request (menu->tearoff_scrollbar, NULL);
      width += menu->tearoff_scrollbar->requisition.width;
    }

  geometry_hints.min_width = width;
  geometry_hints.max_width = width;
    
  geometry_hints.min_height = 0;
  geometry_hints.max_height = BTK_WIDGET (menu)->requisition.height;

  btk_window_set_geometry_hints (BTK_WINDOW (menu->tearoff_window),
				 NULL,
				 &geometry_hints,
				 BDK_HINT_MAX_SIZE|BDK_HINT_MIN_SIZE);
}

static void
btk_menu_update_title (BtkMenu *menu)
{
  if (menu->tearoff_window)
    {
      const gchar *title;
      BtkWidget *attach_widget;

      title = btk_menu_get_title (menu);
      if (!title)
	{
	  attach_widget = btk_menu_get_attach_widget (menu);
	  if (BTK_IS_MENU_ITEM (attach_widget))
	    {
	      BtkWidget *child = BTK_BIN (attach_widget)->child;
	      if (BTK_IS_LABEL (child))
		title = btk_label_get_text (BTK_LABEL (child));
	    }
	}
      
      if (title)
	btk_window_set_title (BTK_WINDOW (menu->tearoff_window), title);
    }
}

static BtkWidget*
btk_menu_get_toplevel (BtkWidget *menu)
{
  BtkWidget *attach, *toplevel;

  attach = btk_menu_get_attach_widget (BTK_MENU (menu));

  if (BTK_IS_MENU_ITEM (attach))
    attach = attach->parent;

  if (BTK_IS_MENU (attach))
    return btk_menu_get_toplevel (attach);
  else if (BTK_IS_WIDGET (attach))
    {
      toplevel = btk_widget_get_toplevel (attach);
      if (btk_widget_is_toplevel (toplevel)) 
	return toplevel;
    }

  return NULL;
}

static void
tearoff_window_destroyed (BtkWidget *widget,
			  BtkMenu   *menu)
{
  btk_menu_set_tearoff_state (menu, FALSE);
}

void       
btk_menu_set_tearoff_state (BtkMenu  *menu,
			    gboolean  torn_off)
{
  gint width, height;
  
  g_return_if_fail (BTK_IS_MENU (menu));

  if (menu->torn_off != torn_off)
    {
      menu->torn_off = torn_off;
      menu->tearoff_active = torn_off;
      
      if (menu->torn_off)
	{
	  if (btk_widget_get_visible (BTK_WIDGET (menu)))
	    btk_menu_popdown (menu);

	  if (!menu->tearoff_window)
	    {
	      BtkWidget *toplevel;

	      menu->tearoff_window = g_object_new (BTK_TYPE_WINDOW,
						     "type", BTK_WINDOW_TOPLEVEL,
						     "screen", btk_widget_get_screen (menu->toplevel),
						     "app-paintable", TRUE,
						     NULL);

	      btk_window_set_type_hint (BTK_WINDOW (menu->tearoff_window),
					BDK_WINDOW_TYPE_HINT_MENU);
	      btk_window_set_mnemonic_modifier (BTK_WINDOW (menu->tearoff_window), 0);
	      g_signal_connect (menu->tearoff_window, "destroy",
				G_CALLBACK (tearoff_window_destroyed), menu);
	      g_signal_connect (menu->tearoff_window, "event",
				G_CALLBACK (btk_menu_window_event), menu);

	      btk_menu_update_title (menu);

	      btk_widget_realize (menu->tearoff_window);

	      toplevel = btk_menu_get_toplevel (BTK_WIDGET (menu));
	      if (toplevel != NULL)
		btk_window_set_transient_for (BTK_WINDOW (menu->tearoff_window),
					      BTK_WINDOW (toplevel));
	      
	      menu->tearoff_hbox = btk_hbox_new (FALSE, FALSE);
	      btk_container_add (BTK_CONTAINER (menu->tearoff_window), menu->tearoff_hbox);

              width = bdk_window_get_width (BTK_WIDGET (menu)->window);
              height = bdk_window_get_height (BTK_WIDGET (menu)->window);

	      menu->tearoff_adjustment =
		BTK_ADJUSTMENT (btk_adjustment_new (0,
						    0,
						    BTK_WIDGET (menu)->requisition.height,
						    MENU_SCROLL_STEP2,
						    height/2,
						    height));
	      g_object_connect (menu->tearoff_adjustment,
				"signal::value-changed", btk_menu_scrollbar_changed, menu,
				NULL);
	      menu->tearoff_scrollbar = btk_vscrollbar_new (menu->tearoff_adjustment);

	      btk_box_pack_end (BTK_BOX (menu->tearoff_hbox),
				menu->tearoff_scrollbar,
				FALSE, FALSE, 0);
	      
	      if (menu->tearoff_adjustment->upper > height)
		btk_widget_show (menu->tearoff_scrollbar);
	      
	      btk_widget_show (menu->tearoff_hbox);
	    }
	  
	  btk_menu_reparent (menu, menu->tearoff_hbox, FALSE);

          width = bdk_window_get_width (BTK_WIDGET (menu)->window);

	  /* Update menu->requisition
	   */
	  btk_widget_size_request (BTK_WIDGET (menu), NULL);
  
	  btk_menu_set_tearoff_hints (menu, width);
	    
	  btk_widget_realize (menu->tearoff_window);
	  btk_menu_position (menu, TRUE);
	  
	  btk_widget_show (BTK_WIDGET (menu));
	  btk_widget_show (menu->tearoff_window);

	  btk_menu_scroll_to (menu, 0);

	}
      else
	{
	  btk_widget_hide (BTK_WIDGET (menu));
	  btk_widget_hide (menu->tearoff_window);
	  if (BTK_IS_CONTAINER (menu->toplevel))
	    btk_menu_reparent (menu, menu->toplevel, FALSE);
	  btk_widget_destroy (menu->tearoff_window);
	  
	  menu->tearoff_window = NULL;
	  menu->tearoff_hbox = NULL;
	  menu->tearoff_scrollbar = NULL;
	  menu->tearoff_adjustment = NULL;
	}

      g_object_notify (B_OBJECT (menu), "tearoff-state");
    }
}

/**
 * btk_menu_get_tearoff_state:
 * @menu: a #BtkMenu
 *
 * Returns whether the menu is torn off. See
 * btk_menu_set_tearoff_state ().
 *
 * Return value: %TRUE if the menu is currently torn off.
 **/
gboolean
btk_menu_get_tearoff_state (BtkMenu *menu)
{
  g_return_val_if_fail (BTK_IS_MENU (menu), FALSE);

  return menu->torn_off;
}

/**
 * btk_menu_set_title:
 * @menu: a #BtkMenu
 * @title: a string containing the title for the menu.
 * 
 * Sets the title string for the menu.  The title is displayed when the menu
 * is shown as a tearoff menu.  If @title is %NULL, the menu will see if it is
 * attached to a parent menu item, and if so it will try to use the same text as
 * that menu item's label.
 **/
void
btk_menu_set_title (BtkMenu     *menu,
		    const gchar *title)
{
  BtkMenuPrivate *priv;
  char *old_title;

  g_return_if_fail (BTK_IS_MENU (menu));

  priv = btk_menu_get_private (menu);

  old_title = priv->title;
  priv->title = g_strdup (title);
  g_free (old_title);
       
  btk_menu_update_title (menu);
  g_object_notify (B_OBJECT (menu), "tearoff-title");
}

/**
 * btk_menu_get_title:
 * @menu: a #BtkMenu
 *
 * Returns the title of the menu. See btk_menu_set_title().
 *
 * Return value: the title of the menu, or %NULL if the menu has no
 * title set on it. This string is owned by the widget and should
 * not be modified or freed.
 **/
const gchar *
btk_menu_get_title (BtkMenu *menu)
{
  BtkMenuPrivate *priv;

  g_return_val_if_fail (BTK_IS_MENU (menu), NULL);

  priv = btk_menu_get_private (menu);

  return priv->title;
}

void
btk_menu_reorder_child (BtkMenu   *menu,
                        BtkWidget *child,
                        gint       position)
{
  BtkMenuShell *menu_shell;

  g_return_if_fail (BTK_IS_MENU (menu));
  g_return_if_fail (BTK_IS_MENU_ITEM (child));

  menu_shell = BTK_MENU_SHELL (menu);

  if (g_list_find (menu_shell->children, child))
    {   
      menu_shell->children = g_list_remove (menu_shell->children, child);
      menu_shell->children = g_list_insert (menu_shell->children, child, position);

      menu_queue_resize (menu);
    }   
}

static void
btk_menu_style_set (BtkWidget *widget,
		    BtkStyle  *previous_style)
{
  if (btk_widget_get_realized (widget))
    {
      BtkMenu *menu = BTK_MENU (widget);
      
      btk_style_set_background (widget->style, menu->bin_window, BTK_STATE_NORMAL);
      btk_style_set_background (widget->style, menu->view_window, BTK_STATE_NORMAL);
      btk_style_set_background (widget->style, widget->window, BTK_STATE_NORMAL);
    }
}

static void
get_arrows_border (BtkMenu   *menu,
                   BtkBorder *border)
{
  guint scroll_arrow_height;
  BtkArrowPlacement arrow_placement;

  btk_widget_style_get (BTK_WIDGET (menu),
                        "scroll-arrow-vlength", &scroll_arrow_height,
                        "arrow_placement", &arrow_placement,
                        NULL);

  switch (arrow_placement)
    {
    case BTK_ARROWS_BOTH:
      border->top = menu->upper_arrow_visible ? scroll_arrow_height : 0;
      border->bottom = menu->lower_arrow_visible ? scroll_arrow_height : 0;
      break;

    case BTK_ARROWS_START:
      border->top = (menu->upper_arrow_visible ||
                     menu->lower_arrow_visible) ? scroll_arrow_height : 0;
      border->bottom = 0;
      break;

    case BTK_ARROWS_END:
      border->top = 0;
      border->bottom = (menu->upper_arrow_visible ||
                        menu->lower_arrow_visible) ? scroll_arrow_height : 0;
      break;
    }

  border->left = border->right = 0;
}

static void
btk_menu_realize (BtkWidget *widget)
{
  BdkWindowAttr attributes;
  gint attributes_mask;
  gint border_width;
  BtkMenu *menu;
  BtkWidget *child;
  GList *children;
  guint vertical_padding;
  guint horizontal_padding;
  BtkBorder arrow_border;

  g_return_if_fail (BTK_IS_MENU (widget));

  menu = BTK_MENU (widget);
  
  btk_widget_set_realized (widget, TRUE);
  
  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);
  
  attributes.event_mask = btk_widget_get_events (widget);

  attributes.event_mask |= (BDK_EXPOSURE_MASK | BDK_KEY_PRESS_MASK |
			    BDK_ENTER_NOTIFY_MASK | BDK_LEAVE_NOTIFY_MASK );
  
  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;
  widget->window = bdk_window_new (btk_widget_get_parent_window (widget), &attributes, attributes_mask);
  bdk_window_set_user_data (widget->window, widget);
  
  border_width = BTK_CONTAINER (widget)->border_width;

  btk_widget_style_get (BTK_WIDGET (menu),
			"vertical-padding", &vertical_padding,
                        "horizontal-padding", &horizontal_padding,
			NULL);

  attributes.x = border_width + widget->style->xthickness + horizontal_padding;
  attributes.y = border_width + widget->style->ythickness + vertical_padding;
  attributes.width = MAX (1, widget->allocation.width - attributes.x * 2);
  attributes.height = MAX (1, widget->allocation.height - attributes.y * 2);

  get_arrows_border (menu, &arrow_border);
  attributes.y += arrow_border.top;
  attributes.height -= arrow_border.top;
  attributes.height -= arrow_border.bottom;

  menu->view_window = bdk_window_new (widget->window, &attributes, attributes_mask);
  bdk_window_set_user_data (menu->view_window, menu);

  attributes.x = 0;
  attributes.y = 0;
  attributes.width = MAX (1, widget->allocation.width - (border_width + widget->style->xthickness + horizontal_padding) * 2);
  attributes.height = MAX (1, widget->requisition.height - (border_width + widget->style->ythickness + vertical_padding) * 2);
  
  menu->bin_window = bdk_window_new (menu->view_window, &attributes, attributes_mask);
  bdk_window_set_user_data (menu->bin_window, menu);

  children = BTK_MENU_SHELL (menu)->children;
  while (children)
    {
      child = children->data;
      children = children->next;
	  
      btk_widget_set_parent_window (child, menu->bin_window);
    }
  
  widget->style = btk_style_attach (widget->style, widget->window);
  btk_style_set_background (widget->style, menu->bin_window, BTK_STATE_NORMAL);
  btk_style_set_background (widget->style, menu->view_window, BTK_STATE_NORMAL);
  btk_style_set_background (widget->style, widget->window, BTK_STATE_NORMAL);

  if (BTK_MENU_SHELL (widget)->active_menu_item)
    btk_menu_scroll_item_visible (BTK_MENU_SHELL (widget),
				  BTK_MENU_SHELL (widget)->active_menu_item);

  bdk_window_show (menu->bin_window);
  bdk_window_show (menu->view_window);
}

static gboolean 
btk_menu_focus (BtkWidget       *widget,
                BtkDirectionType direction)
{
  /*
   * A menu or its menu items cannot have focus
   */
  return FALSE;
}

/* See notes in btk_menu_popup() for information about the "grab transfer window"
 */
static BdkWindow *
menu_grab_transfer_window_get (BtkMenu *menu)
{
  BdkWindow *window = g_object_get_data (B_OBJECT (menu), "btk-menu-transfer-window");
  if (!window)
    {
      BdkWindowAttr attributes;
      gint attributes_mask;
      
      attributes.x = -100;
      attributes.y = -100;
      attributes.width = 10;
      attributes.height = 10;
      attributes.window_type = BDK_WINDOW_TEMP;
      attributes.wclass = BDK_INPUT_ONLY;
      attributes.override_redirect = TRUE;
      attributes.event_mask = 0;

      attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_NOREDIR;
      
      window = bdk_window_new (btk_widget_get_root_window (BTK_WIDGET (menu)),
			       &attributes, attributes_mask);
      bdk_window_set_user_data (window, menu);

      bdk_window_show (window);

      g_object_set_data (B_OBJECT (menu), I_("btk-menu-transfer-window"), window);
    }

  return window;
}

static void
menu_grab_transfer_window_destroy (BtkMenu *menu)
{
  BdkWindow *window = g_object_get_data (B_OBJECT (menu), "btk-menu-transfer-window");
  if (window)
    {
      bdk_window_set_user_data (window, NULL);
      bdk_window_destroy (window);
      g_object_set_data (B_OBJECT (menu), I_("btk-menu-transfer-window"), NULL);
    }
}

static void
btk_menu_unrealize (BtkWidget *widget)
{
  BtkMenu *menu = BTK_MENU (widget);

  menu_grab_transfer_window_destroy (menu);

  bdk_window_set_user_data (menu->view_window, NULL);
  bdk_window_destroy (menu->view_window);
  menu->view_window = NULL;

  bdk_window_set_user_data (menu->bin_window, NULL);
  bdk_window_destroy (menu->bin_window);
  menu->bin_window = NULL;

  BTK_WIDGET_CLASS (btk_menu_parent_class)->unrealize (widget);
}

static void
btk_menu_size_request (BtkWidget      *widget,
		       BtkRequisition *requisition)
{
  gint i;
  BtkMenu *menu;
  BtkMenuShell *menu_shell;
  BtkWidget *child;
  GList *children;
  guint max_toggle_size;
  guint max_accel_width;
  guint vertical_padding;
  guint horizontal_padding;
  BtkRequisition child_requisition;
  BtkMenuPrivate *priv;
  
  g_return_if_fail (BTK_IS_MENU (widget));
  g_return_if_fail (requisition != NULL);
  
  menu = BTK_MENU (widget);
  menu_shell = BTK_MENU_SHELL (widget);
  priv = btk_menu_get_private (menu);
  
  requisition->width = 0;
  requisition->height = 0;
  
  max_toggle_size = 0;
  max_accel_width = 0;
  
  g_free (priv->heights);
  priv->heights = g_new0 (guint, btk_menu_get_n_rows (menu));
  priv->heights_length = btk_menu_get_n_rows (menu);

  children = menu_shell->children;
  while (children)
    {
      gint part;
      gint toggle_size;
      gint l, r, t, b;

      child = children->data;
      children = children->next;
      
      if (! btk_widget_get_visible (child))
        continue;

      get_effective_child_attach (child, &l, &r, &t, &b);

      /* It's important to size_request the child
       * before doing the toggle size request, in
       * case the toggle size request depends on the size
       * request of a child of the child (e.g. for ImageMenuItem)
       */

       BTK_MENU_ITEM (child)->show_submenu_indicator = TRUE;
       btk_widget_size_request (child, &child_requisition);

       btk_menu_item_toggle_size_request (BTK_MENU_ITEM (child), &toggle_size);
       max_toggle_size = MAX (max_toggle_size, toggle_size);
       max_accel_width = MAX (max_accel_width,
                              BTK_MENU_ITEM (child)->accelerator_width);

       part = child_requisition.width / (r - l);
       requisition->width = MAX (requisition->width, part);

       part = MAX (child_requisition.height, toggle_size) / (b - t);
       priv->heights[t] = MAX (priv->heights[t], part);
    }

  /* If the menu doesn't include any images or check items
   * reserve the space so that all menus are consistent.
   * We only do this for 'ordinary' menus, not for combobox
   * menus or multi-column menus
   */
  if (max_toggle_size == 0 && 
      btk_menu_get_n_columns (menu) == 1 &&
      !priv->no_toggle_size)
    {
      guint toggle_spacing;
      guint indicator_size;

      btk_style_get (widget->style,
                     BTK_TYPE_CHECK_MENU_ITEM,
                     "toggle-spacing", &toggle_spacing,
                     "indicator-size", &indicator_size,
                     NULL);

      max_toggle_size = indicator_size + toggle_spacing;
    }

  for (i = 0; i < btk_menu_get_n_rows (menu); i++)
    requisition->height += priv->heights[i];

  requisition->width += 2 * max_toggle_size + max_accel_width;
  requisition->width *= btk_menu_get_n_columns (menu);

  btk_widget_style_get (BTK_WIDGET (menu),
			"vertical-padding", &vertical_padding,
                        "horizontal-padding", &horizontal_padding,
			NULL);

  requisition->width += (BTK_CONTAINER (menu)->border_width + horizontal_padding +
			 widget->style->xthickness) * 2;
  requisition->height += (BTK_CONTAINER (menu)->border_width + vertical_padding +
			  widget->style->ythickness) * 2;
  
  menu->toggle_size = max_toggle_size;

  /* Don't resize the tearoff if it is not active, because it won't redraw (it is only a background pixmap).
   */
  if (menu->tearoff_active)
    btk_menu_set_tearoff_hints (menu, requisition->width);
}

static void
btk_menu_size_allocate (BtkWidget     *widget,
			BtkAllocation *allocation)
{
  BtkMenu *menu;
  BtkMenuShell *menu_shell;
  BtkWidget *child;
  BtkAllocation child_allocation;
  BtkRequisition child_requisition;
  BtkMenuPrivate *priv;
  GList *children;
  gint x, y;
  gint width, height;
  guint vertical_padding;
  guint horizontal_padding;
  
  g_return_if_fail (BTK_IS_MENU (widget));
  g_return_if_fail (allocation != NULL);
  
  menu = BTK_MENU (widget);
  menu_shell = BTK_MENU_SHELL (widget);
  priv = btk_menu_get_private (menu);

  widget->allocation = *allocation;
  btk_widget_get_child_requisition (BTK_WIDGET (menu), &child_requisition);

  btk_widget_style_get (BTK_WIDGET (menu),
			"vertical-padding", &vertical_padding,
                        "horizontal-padding", &horizontal_padding,
			NULL);

  x = BTK_CONTAINER (menu)->border_width + widget->style->xthickness + horizontal_padding;
  y = BTK_CONTAINER (menu)->border_width + widget->style->ythickness + vertical_padding;

  width = MAX (1, allocation->width - x * 2);
  height = MAX (1, allocation->height - y * 2);

  child_requisition.width -= x * 2;
  child_requisition.height -= y * 2;

  if (menu_shell->active)
    btk_menu_scroll_to (menu, menu->scroll_offset);

  if (!menu->tearoff_active)
    {
      BtkBorder arrow_border;

      get_arrows_border (menu, &arrow_border);
      y += arrow_border.top;
      height -= arrow_border.top;
      height -= arrow_border.bottom;
    }

  if (btk_widget_get_realized (widget))
    {
      bdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);

      bdk_window_move_resize (menu->view_window,
			      x,
			      y,
			      width,
			      height);
    }

  if (menu_shell->children)
    {
      gint base_width = width / btk_menu_get_n_columns (menu);

      children = menu_shell->children;
      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if (btk_widget_get_visible (child))
	    {
              gint i;
	      gint l, r, t, b;

	      get_effective_child_attach (child, &l, &r, &t, &b);

              if (btk_widget_get_direction (BTK_WIDGET (menu)) == BTK_TEXT_DIR_RTL)
                {
                  guint tmp;
		  tmp = btk_menu_get_n_columns (menu) - l;
		  l = btk_menu_get_n_columns (menu) - r;
                  r = tmp;
                }

              child_allocation.width = (r - l) * base_width;
              child_allocation.height = 0;
              child_allocation.x = l * base_width;
              child_allocation.y = 0;

              for (i = 0; i < b; i++)
                {
                  if (i < t)
                    child_allocation.y += priv->heights[i];
                  else
                    child_allocation.height += priv->heights[i];
                }

	      btk_menu_item_toggle_size_allocate (BTK_MENU_ITEM (child),
						  menu->toggle_size);

	      btk_widget_size_allocate (child, &child_allocation);
	      btk_widget_queue_draw (child);
	    }
	}
      
      /* Resize the item window */
      if (btk_widget_get_realized (widget))
	{
          gint i;
          gint width, height;

          height = 0;
	  for (i = 0; i < btk_menu_get_n_rows (menu); i++)
            height += priv->heights[i];

	  width = btk_menu_get_n_columns (menu) * base_width;
	  bdk_window_resize (menu->bin_window, width, height);
	}

      if (menu->tearoff_active)
	{
	  if (allocation->height >= widget->requisition.height)
	    {
	      if (btk_widget_get_visible (menu->tearoff_scrollbar))
		{
		  btk_widget_hide (menu->tearoff_scrollbar);
		  btk_menu_set_tearoff_hints (menu, allocation->width);

		  btk_menu_scroll_to (menu, 0);
		}
	    }
	  else
	    {
	      menu->tearoff_adjustment->upper = widget->requisition.height;
	      menu->tearoff_adjustment->page_size = allocation->height;
	      
	      if (menu->tearoff_adjustment->value + menu->tearoff_adjustment->page_size >
		  menu->tearoff_adjustment->upper)
		{
		  gint value;
		  value = menu->tearoff_adjustment->upper - menu->tearoff_adjustment->page_size;
		  if (value < 0)
		    value = 0;
		  btk_menu_scroll_to (menu, value);
		}
	      
	      btk_adjustment_changed (menu->tearoff_adjustment);
	      
	      if (!btk_widget_get_visible (menu->tearoff_scrollbar))
		{
		  btk_widget_show (menu->tearoff_scrollbar);
		  btk_menu_set_tearoff_hints (menu, allocation->width);
		}
	    }
	}
    }
}

static void
get_arrows_visible_area (BtkMenu      *menu,
                         BdkRectangle *border,
                         BdkRectangle *upper,
                         BdkRectangle *lower,
                         gint         *arrow_space)
{
  BtkWidget *widget = BTK_WIDGET (menu);
  guint vertical_padding;
  guint horizontal_padding;
  gint scroll_arrow_height;
  BtkArrowPlacement arrow_placement;

  btk_widget_style_get (widget,
                        "vertical-padding", &vertical_padding,
                        "horizontal-padding", &horizontal_padding,
                        "scroll-arrow-vlength", &scroll_arrow_height,
                        "arrow-placement", &arrow_placement,
                        NULL);

  border->x = BTK_CONTAINER (widget)->border_width + widget->style->xthickness + horizontal_padding;
  border->y = BTK_CONTAINER (widget)->border_width + widget->style->ythickness + vertical_padding;
  border->width = bdk_window_get_width (widget->window);
  border->height = bdk_window_get_height (widget->window);

  switch (arrow_placement)
    {
    case BTK_ARROWS_BOTH:
      upper->x = border->x;
      upper->y = border->y;
      upper->width = border->width - 2 * border->x;
      upper->height = scroll_arrow_height;

      lower->x = border->x;
      lower->y = border->height - border->y - scroll_arrow_height;
      lower->width = border->width - 2 * border->x;
      lower->height = scroll_arrow_height;
      break;

    case BTK_ARROWS_START:
      upper->x = border->x;
      upper->y = border->y;
      upper->width = (border->width - 2 * border->x) / 2;
      upper->height = scroll_arrow_height;

      lower->x = border->x + upper->width;
      lower->y = border->y;
      lower->width = (border->width - 2 * border->x) / 2;
      lower->height = scroll_arrow_height;
      break;

    case BTK_ARROWS_END:
      upper->x = border->x;
      upper->y = border->height - border->y - scroll_arrow_height;
      upper->width = (border->width - 2 * border->x) / 2;
      upper->height = scroll_arrow_height;

      lower->x = border->x + upper->width;
      lower->y = border->height - border->y - scroll_arrow_height;
      lower->width = (border->width - 2 * border->x) / 2;
      lower->height = scroll_arrow_height;
      break;

    default:
       g_assert_not_reached();
       upper->x = upper->y = upper->width = upper->height = 0;
       lower->x = lower->y = lower->width = lower->height = 0;
    }

  *arrow_space = scroll_arrow_height - 2 * widget->style->ythickness;
}

static void
btk_menu_paint (BtkWidget      *widget,
		BdkEventExpose *event)
{
  BtkMenu *menu;
  BtkMenuPrivate *priv;
  BdkRectangle border;
  BdkRectangle upper;
  BdkRectangle lower;
  gint arrow_space;
  
  g_return_if_fail (BTK_IS_MENU (widget));

  menu = BTK_MENU (widget);
  priv = btk_menu_get_private (menu);

  get_arrows_visible_area (menu, &border, &upper, &lower, &arrow_space);

  if (event->window == widget->window)
    {
      gfloat arrow_scaling;
      gint arrow_size;

      btk_widget_style_get (widget, "arrow-scaling", &arrow_scaling, NULL);
      arrow_size = arrow_scaling * arrow_space;

      btk_paint_box (widget->style,
		     widget->window,
		     BTK_STATE_NORMAL,
		     BTK_SHADOW_OUT,
		     &event->area, widget, "menu",
		     0, 0, -1, -1);

      if (menu->upper_arrow_visible && !menu->tearoff_active)
	{
	  btk_paint_box (widget->style,
			 widget->window,
			 priv->upper_arrow_state,
                         BTK_SHADOW_OUT,
			 &event->area, widget, "menu_scroll_arrow_up",
                         upper.x,
                         upper.y,
                         upper.width,
                         upper.height);

	  btk_paint_arrow (widget->style,
			   widget->window,
			   priv->upper_arrow_state,
			   BTK_SHADOW_OUT,
			   &event->area, widget, "menu_scroll_arrow_up",
			   BTK_ARROW_UP,
			   TRUE,
                           upper.x + (upper.width - arrow_size) / 2,
                           upper.y + widget->style->ythickness + (arrow_space - arrow_size) / 2,
			   arrow_size, arrow_size);
	}

      if (menu->lower_arrow_visible && !menu->tearoff_active)
	{
	  btk_paint_box (widget->style,
			 widget->window,
			 priv->lower_arrow_state,
                         BTK_SHADOW_OUT,
			 &event->area, widget, "menu_scroll_arrow_down",
                         lower.x,
                         lower.y,
                         lower.width,
                         lower.height);

	  btk_paint_arrow (widget->style,
			   widget->window,
			   priv->lower_arrow_state,
			   BTK_SHADOW_OUT,
			   &event->area, widget, "menu_scroll_arrow_down",
			   BTK_ARROW_DOWN,
			   TRUE,
                           lower.x + (lower.width - arrow_size) / 2,
			   lower.y + widget->style->ythickness + (arrow_space - arrow_size) / 2,
			   arrow_size, arrow_size);
	}
    }
  else if (event->window == menu->bin_window)
    {
      gint y = -border.y + menu->scroll_offset;

      if (!menu->tearoff_active)
        {
          BtkBorder arrow_border;

          get_arrows_border (menu, &arrow_border);
          y -= arrow_border.top;
        }

      btk_paint_box (widget->style,
		     menu->bin_window,
		     BTK_STATE_NORMAL,
		     BTK_SHADOW_OUT,
		     &event->area, widget, "menu",
		     - border.x, y,
		     border.width, border.height);
    }
}

static gboolean
btk_menu_expose (BtkWidget	*widget,
		 BdkEventExpose *event)
{
  g_return_val_if_fail (BTK_IS_MENU (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (btk_widget_is_drawable (widget))
    {
      btk_menu_paint (widget, event);

      BTK_WIDGET_CLASS (btk_menu_parent_class)->expose_event (widget, event);
    }
  
  return FALSE;
}

static void
btk_menu_show (BtkWidget *widget)
{
  BtkMenu *menu = BTK_MENU (widget);

  _btk_menu_refresh_accel_paths (menu, FALSE);

  BTK_WIDGET_CLASS (btk_menu_parent_class)->show (widget);
}

static gboolean
btk_menu_button_scroll (BtkMenu        *menu,
                        BdkEventButton *event)
{
  if (menu->upper_arrow_prelight || menu->lower_arrow_prelight)
    {
      gboolean touchscreen_mode;

      g_object_get (btk_widget_get_settings (BTK_WIDGET (menu)),
                    "btk-touchscreen-mode", &touchscreen_mode,
                    NULL);

      if (touchscreen_mode)
        btk_menu_handle_scrolling (menu,
                                   event->x_root, event->y_root,
                                   event->type == BDK_BUTTON_PRESS,
                                   FALSE);

      return TRUE;
    }

  return FALSE;
}

static gboolean
pointer_in_menu_window (BtkWidget *widget,
                        gdouble    x_root,
                        gdouble    y_root)
{
  BtkMenu *menu = BTK_MENU (widget);

  if (btk_widget_get_mapped (menu->toplevel))
    {
      BtkMenuShell *menu_shell;
      gint          window_x, window_y;

      bdk_window_get_position (menu->toplevel->window, &window_x, &window_y);

      if (x_root >= window_x && x_root < window_x + widget->allocation.width &&
          y_root >= window_y && y_root < window_y + widget->allocation.height)
        return TRUE;

      menu_shell = BTK_MENU_SHELL (widget);

      if (BTK_IS_MENU (menu_shell->parent_menu_shell))
        return pointer_in_menu_window (menu_shell->parent_menu_shell,
                                       x_root, y_root);
    }

  return FALSE;
}

static gboolean
btk_menu_button_press (BtkWidget      *widget,
                       BdkEventButton *event)
{
  if (event->type != BDK_BUTTON_PRESS)
    return FALSE;

  /* Don't pass down to menu shell for presses over scroll arrows
   */
  if (btk_menu_button_scroll (BTK_MENU (widget), event))
    return TRUE;

  /*  Don't pass down to menu shell if a non-menuitem part of the menu
   *  was clicked. The check for the event_widget being a BtkMenuShell
   *  works because we have the pointer grabbed on menu_shell->window
   *  with owner_events=TRUE, so all events that are either outside
   *  the menu or on its border are delivered relative to
   *  menu_shell->window.
   */
  if (BTK_IS_MENU_SHELL (btk_get_event_widget ((BdkEvent *) event)) &&
      pointer_in_menu_window (widget, event->x_root, event->y_root))
    return TRUE;

  return BTK_WIDGET_CLASS (btk_menu_parent_class)->button_press_event (widget, event);
}

static gboolean
btk_menu_button_release (BtkWidget      *widget,
			 BdkEventButton *event)
{
  BtkMenuPrivate *priv = btk_menu_get_private (BTK_MENU (widget));

  if (priv->ignore_button_release)
    {
      priv->ignore_button_release = FALSE;
      return FALSE;
    }

  if (event->type != BDK_BUTTON_RELEASE)
    return FALSE;

  /* Don't pass down to menu shell for releases over scroll arrows
   */
  if (btk_menu_button_scroll (BTK_MENU (widget), event))
    return TRUE;

  /*  Don't pass down to menu shell if a non-menuitem part of the menu
   *  was clicked (see comment in button_press()).
   */
  if (BTK_IS_MENU_SHELL (btk_get_event_widget ((BdkEvent *) event)) &&
      pointer_in_menu_window (widget, event->x_root, event->y_root))
    {
      /*  Ugly: make sure menu_shell->button gets reset to 0 when we
       *  bail out early here so it is in a consistent state for the
       *  next button_press/button_release in BtkMenuShell.
       *  See bug #449371.
       */
      if (BTK_MENU_SHELL (widget)->active)
        BTK_MENU_SHELL (widget)->button = 0;

      return TRUE;
    }

  return BTK_WIDGET_CLASS (btk_menu_parent_class)->button_release_event (widget, event);
}

static const gchar *
get_accel_path (BtkWidget *menu_item,
		gboolean  *locked)
{
  const gchar *path;
  BtkWidget *label;
  GClosure *accel_closure;
  BtkAccelGroup *accel_group;    

  path = _btk_widget_get_accel_path (menu_item, locked);
  if (!path)
    {
      path = BTK_MENU_ITEM (menu_item)->accel_path;
      
      if (locked)
	{
	  *locked = TRUE;

	  label = BTK_BIN (menu_item)->child;
	  
	  if (BTK_IS_ACCEL_LABEL (label))
	    {
	      g_object_get (label, 
			    "accel-closure", &accel_closure, 
			    NULL);
	      if (accel_closure)
		{
		  accel_group = btk_accel_group_from_accel_closure (accel_closure);
		  
		  *locked = accel_group->lock_count > 0;
		}
	    }
	}
    }

  return path;
}

static gboolean
btk_menu_key_press (BtkWidget	*widget,
		    BdkEventKey *event)
{
  BtkMenuShell *menu_shell;
  BtkMenu *menu;
  gboolean delete = FALSE;
  gboolean can_change_accels;
  gchar *accel = NULL;
  guint accel_key, accel_mods;
  BdkModifierType consumed_modifiers;
  BdkDisplay *display;
  
  g_return_val_if_fail (BTK_IS_MENU (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
      
  menu_shell = BTK_MENU_SHELL (widget);
  menu = BTK_MENU (widget);
  
  btk_menu_stop_navigating_submenu (menu);

  if (BTK_WIDGET_CLASS (btk_menu_parent_class)->key_press_event (widget, event))
    return TRUE;

  display = btk_widget_get_display (widget);
    
  g_object_get (btk_widget_get_settings (widget),
                "btk-menu-bar-accel", &accel,
		"btk-can-change-accels", &can_change_accels,
                NULL);

  if (accel && *accel)
    {
      guint keyval = 0;
      BdkModifierType mods = 0;
      
      btk_accelerator_parse (accel, &keyval, &mods);

      if (keyval == 0)
        g_warning ("Failed to parse menu bar accelerator '%s'\n", accel);

      /* FIXME this is wrong, needs to be in the global accel resolution
       * thing, to properly consider i18n etc., but that probably requires
       * AccelGroup changes etc.
       */
      if (event->keyval == keyval && (mods & event->state) == mods)
        {
	  btk_menu_shell_cancel (menu_shell);
          g_free (accel);
          return TRUE;
        }
    }

  g_free (accel);
  
  switch (event->keyval)
    {
    case BDK_Delete:
    case BDK_KP_Delete:
    case BDK_BackSpace:
      delete = TRUE;
      break;
    default:
      break;
    }

  /* Figure out what modifiers went into determining the key symbol */
  _btk_translate_keyboard_accel_state (bdk_keymap_get_for_display (display),
                                       event->hardware_keycode,
                                       event->state,
                                       btk_accelerator_get_default_mod_mask (),
                                       event->group,
                                       &accel_key, NULL, NULL, &consumed_modifiers);

  accel_key = bdk_keyval_to_lower (accel_key);
  accel_mods = event->state & btk_accelerator_get_default_mod_mask () & ~consumed_modifiers;

  /* If lowercasing affects the keysym, then we need to include SHIFT
   * in the modifiers, We re-upper case when we match against the
   * keyval, but display and save in caseless form.
   */
  if (accel_key != event->keyval)
    accel_mods |= BDK_SHIFT_MASK;


  /* Modify the accelerators */
  if (can_change_accels &&
      menu_shell->active_menu_item &&
      BTK_BIN (menu_shell->active_menu_item)->child &&			/* no separators */
      BTK_MENU_ITEM (menu_shell->active_menu_item)->submenu == NULL &&	/* no submenus */
      (delete || btk_accelerator_valid (accel_key, accel_mods)))
    {
      BtkWidget *menu_item = menu_shell->active_menu_item;
      gboolean locked, replace_accels = TRUE;
      const gchar *path;

      path = get_accel_path (menu_item, &locked);
      if (!path || locked)
	{
	  /* can't change accelerators on menu_items without paths
	   * (basically, those items are accelerator-locked).
	   */
	  /* g_print("item has no path or is locked, menu prefix: %s\n", menu->accel_path); */
	  btk_widget_error_bell (widget);
	}
      else
	{
	  gboolean changed;

	  /* For the keys that act to delete the current setting, we delete
	   * the current setting if there is one, otherwise, we set the
	   * key as the accelerator.
	   */
	  if (delete)
	    {
	      BtkAccelKey key;
	      
	      if (btk_accel_map_lookup_entry (path, &key) &&
		  (key.accel_key || key.accel_mods))
		{
		  accel_key = 0;
		  accel_mods = 0;
		}
	    }
	  changed = btk_accel_map_change_entry (path, accel_key, accel_mods, replace_accels);

	  if (!changed)
	    {
	      /* we failed, probably because this key is in use and
	       * locked already
	       */
	      /* g_print("failed to change\n"); */
	      btk_widget_error_bell (widget);
	    }
	}
    }
  
  return TRUE;
}

static gboolean
check_threshold (BtkWidget *widget,
                 gint       start_x,
                 gint       start_y,
                 gint       x,
                 gint       y)
{
#define THRESHOLD 8
  
  return
    ABS (start_x - x) > THRESHOLD  ||
    ABS (start_y - y) > THRESHOLD;
}

static gboolean
definitely_within_item (BtkWidget *widget,
                        gint       x,
                        gint       y)
{
  BdkWindow *window = BTK_MENU_ITEM (widget)->event_window;
  int w, h;

  w = bdk_window_get_width (window);
  h = bdk_window_get_height (window);
  
  return
    check_threshold (widget, 0, 0, x, y) &&
    check_threshold (widget, w - 1, 0, x, y) &&
    check_threshold (widget, w - 1, h - 1, x, y) &&
    check_threshold (widget, 0, h - 1, x, y);
}

static gboolean
btk_menu_has_navigation_triangle (BtkMenu *menu)
{
  BtkMenuPrivate *priv;

  priv = btk_menu_get_private (menu);

  return priv->navigation_height && priv->navigation_width;
}

static gboolean
btk_menu_motion_notify (BtkWidget      *widget,
                        BdkEventMotion *event)
{
  BtkWidget *menu_item;
  BtkMenu *menu;
  BtkMenuShell *menu_shell;

  gboolean need_enter;

  if (BTK_IS_MENU (widget))
    {
      BtkMenuPrivate *priv = btk_menu_get_private (BTK_MENU (widget));

      if (priv->ignore_button_release)
        priv->ignore_button_release = FALSE;

      btk_menu_handle_scrolling (BTK_MENU (widget), event->x_root, event->y_root,
                                 TRUE, TRUE);
    }

  /* We received the event for one of two reasons:
   *
   * a) We are the active menu, and did btk_grab_add()
   * b) The widget is a child of ours, and the event was propagated
   *
   * Since for computation of navigation rebunnyions, we want the menu which
   * is the parent of the menu item, for a), we need to find that menu,
   * which may be different from 'widget'.
   */
  menu_item = btk_get_event_widget ((BdkEvent*) event);
  if (!BTK_IS_MENU_ITEM (menu_item) ||
      !BTK_IS_MENU (menu_item->parent))
    return FALSE;

  menu_shell = BTK_MENU_SHELL (menu_item->parent);
  menu = BTK_MENU (menu_shell);

  if (definitely_within_item (menu_item, event->x, event->y))
    menu_shell->activate_time = 0;

  need_enter = (btk_menu_has_navigation_triangle (menu) || menu_shell->ignore_enter);

  /* Check to see if we are within an active submenu's navigation rebunnyion
   */
  if (btk_menu_navigating_submenu (menu, event->x_root, event->y_root))
    return TRUE; 

  /* Make sure we pop down if we enter a non-selectable menu item, so we
   * don't show a submenu when the cursor is outside the stay-up triangle.
   */
  if (!_btk_menu_item_is_selectable (menu_item))
    {
      /* We really want to deselect, but this gives the menushell code
       * a chance to do some bookkeeping about the menuitem.
       */
      btk_menu_shell_select_item (menu_shell, menu_item);
      return FALSE;
    }

  if (need_enter)
    {
      /* The menu is now sensitive to enter events on its items, but
       * was previously sensitive.  So we fake an enter event.
       */
      gint width, height;
      
      menu_shell->ignore_enter = FALSE; 
      
      width = bdk_window_get_width (event->window);
      height = bdk_window_get_height (event->window);
      if (event->x >= 0 && event->x < width &&
	  event->y >= 0 && event->y < height)
	{
	  BdkEvent *send_event = bdk_event_new (BDK_ENTER_NOTIFY);
	  gboolean result;

	  send_event->crossing.window = g_object_ref (event->window);
	  send_event->crossing.time = event->time;
	  send_event->crossing.send_event = TRUE;
	  send_event->crossing.x_root = event->x_root;
	  send_event->crossing.y_root = event->y_root;
	  send_event->crossing.x = event->x;
	  send_event->crossing.y = event->y;
          send_event->crossing.state = event->state;

	  /* We send the event to 'widget', the currently active menu,
	   * instead of 'menu', the menu that the pointer is in. This
	   * will ensure that the event will be ignored unless the
	   * menuitem is a child of the active menu or some parent
	   * menu of the active menu.
	   */
	  result = btk_widget_event (widget, send_event);
	  bdk_event_free (send_event);

	  return result;
	}
    }

  return FALSE;
}

static gboolean
get_double_arrows (BtkMenu *menu)
{
  BtkMenuPrivate   *priv = btk_menu_get_private (menu);
  gboolean          double_arrows;
  BtkArrowPlacement arrow_placement;

  btk_widget_style_get (BTK_WIDGET (menu),
                        "double-arrows", &double_arrows,
                        "arrow-placement", &arrow_placement,
                        NULL);

  if (arrow_placement != BTK_ARROWS_BOTH)
    return TRUE;

  return double_arrows || (priv->initially_pushed_in &&
                           menu->scroll_offset != 0);
}

static void
btk_menu_scroll_by (BtkMenu *menu, 
		    gint     step)
{
  BtkWidget *widget;
  gint offset;
  gint view_width, view_height;
  gboolean double_arrows;
  BtkBorder arrow_border;
  
  widget = BTK_WIDGET (menu);
  offset = menu->scroll_offset + step;

  get_arrows_border (menu, &arrow_border);

  double_arrows = get_double_arrows (menu);

  /* If we scroll upward and the non-visible top part
   * is smaller than the scroll arrow it would be
   * pretty stupid to show the arrow and taking more
   * screen space than just scrolling to the top.
   */
  if (!double_arrows)
    if ((step < 0) && (offset < arrow_border.top))
      offset = 0;

  /* Don't scroll over the top if we weren't before: */
  if ((menu->scroll_offset >= 0) && (offset < 0))
    offset = 0;

  view_width = bdk_window_get_width (widget->window);
  view_height = bdk_window_get_height (widget->window);

  if (menu->scroll_offset == 0 &&
      view_height >= widget->requisition.height)
    return;

  /* Don't scroll past the bottom if we weren't before: */
  if (menu->scroll_offset > 0)
    view_height -= arrow_border.top;

  /* When both arrows are always shown, reduce
   * view height even more.
   */
  if (double_arrows)
    view_height -= arrow_border.bottom;

  if ((menu->scroll_offset + view_height <= widget->requisition.height) &&
      (offset + view_height > widget->requisition.height))
    offset = widget->requisition.height - view_height;

  if (offset != menu->scroll_offset)
    btk_menu_scroll_to (menu, offset);
}

static void
btk_menu_do_timeout_scroll (BtkMenu  *menu,
                            gboolean  touchscreen_mode)
{
  gboolean upper_visible;
  gboolean lower_visible;

  upper_visible = menu->upper_arrow_visible;
  lower_visible = menu->lower_arrow_visible;

  btk_menu_scroll_by (menu, menu->scroll_step);

  if (touchscreen_mode &&
      (upper_visible != menu->upper_arrow_visible ||
       lower_visible != menu->lower_arrow_visible))
    {
      /* We are about to hide a scroll arrow while the mouse is pressed,
       * this would cause the uncovered menu item to be activated on button
       * release. Therefore we need to ignore button release here
       */
      BTK_MENU_SHELL (menu)->ignore_enter = TRUE;
      btk_menu_get_private (menu)->ignore_button_release = TRUE;
    }
}

static gboolean
btk_menu_scroll_timeout (gpointer data)
{
  BtkMenu  *menu;
  gboolean  touchscreen_mode;

  menu = BTK_MENU (data);

  g_object_get (btk_widget_get_settings (BTK_WIDGET (menu)),
                "btk-touchscreen-mode", &touchscreen_mode,
                NULL);

  btk_menu_do_timeout_scroll (menu, touchscreen_mode);

  return TRUE;
}

static gboolean
btk_menu_scroll_timeout_initial (gpointer data)
{
  BtkMenu  *menu;
  guint     timeout;
  gboolean  touchscreen_mode;

  menu = BTK_MENU (data);

  g_object_get (btk_widget_get_settings (BTK_WIDGET (menu)),
                "btk-timeout-repeat", &timeout,
                "btk-touchscreen-mode", &touchscreen_mode,
                NULL);

  btk_menu_do_timeout_scroll (menu, touchscreen_mode);

  btk_menu_remove_scroll_timeout (menu);

  menu->timeout_id = bdk_threads_add_timeout (timeout,
                                              btk_menu_scroll_timeout,
                                              menu);

  return FALSE;
}

static void
btk_menu_start_scrolling (BtkMenu *menu)
{
  guint    timeout;
  gboolean touchscreen_mode;

  g_object_get (btk_widget_get_settings (BTK_WIDGET (menu)),
                "btk-timeout-repeat", &timeout,
                "btk-touchscreen-mode", &touchscreen_mode,
                NULL);

  btk_menu_do_timeout_scroll (menu, touchscreen_mode);

  menu->timeout_id = bdk_threads_add_timeout (timeout,
                                              btk_menu_scroll_timeout_initial,
                                              menu);
}

static gboolean
btk_menu_scroll (BtkWidget	*widget,
		 BdkEventScroll *event)
{
  BtkMenu *menu = BTK_MENU (widget);

  switch (event->direction)
    {
    case BDK_SCROLL_RIGHT:
    case BDK_SCROLL_DOWN:
      btk_menu_scroll_by (menu, MENU_SCROLL_STEP2);
      break;
    case BDK_SCROLL_LEFT:
    case BDK_SCROLL_UP:
      btk_menu_scroll_by (menu, - MENU_SCROLL_STEP2);
      break;
    }

  return TRUE;
}

static void
get_arrows_sensitive_area (BtkMenu      *menu,
                           BdkRectangle *upper,
                           BdkRectangle *lower)
{
  gint width, height;
  gint border;
  guint vertical_padding;
  gint win_x, win_y;
  gint scroll_arrow_height;
  BtkArrowPlacement arrow_placement;

  width = bdk_window_get_width (BTK_WIDGET (menu)->window);
  height = bdk_window_get_height (BTK_WIDGET (menu)->window);

  btk_widget_style_get (BTK_WIDGET (menu),
                        "vertical-padding", &vertical_padding,
                        "scroll-arrow-vlength", &scroll_arrow_height,
                        "arrow-placement", &arrow_placement,
                        NULL);

  border = BTK_CONTAINER (menu)->border_width +
    BTK_WIDGET (menu)->style->ythickness + vertical_padding;

  bdk_window_get_position (BTK_WIDGET (menu)->window, &win_x, &win_y);

  switch (arrow_placement)
    {
    case BTK_ARROWS_BOTH:
      if (upper)
        {
          upper->x = win_x;
          upper->y = win_y;
          upper->width = width;
          upper->height = scroll_arrow_height + border;
        }

      if (lower)
        {
          lower->x = win_x;
          lower->y = win_y + height - border - scroll_arrow_height;
          lower->width = width;
          lower->height = scroll_arrow_height + border;
        }
      break;

    case BTK_ARROWS_START:
      if (upper)
        {
          upper->x = win_x;
          upper->y = win_y;
          upper->width = width / 2;
          upper->height = scroll_arrow_height + border;
        }

      if (lower)
        {
          lower->x = win_x + width / 2;
          lower->y = win_y;
          lower->width = width / 2;
          lower->height = scroll_arrow_height + border;
        }
      break;

    case BTK_ARROWS_END:
      if (upper)
        {
          upper->x = win_x;
          upper->y = win_y + height - border - scroll_arrow_height;
          upper->width = width / 2;
          upper->height = scroll_arrow_height + border;
        }

      if (lower)
        {
          lower->x = win_x + width / 2;
          lower->y = win_y + height - border - scroll_arrow_height;
          lower->width = width / 2;
          lower->height = scroll_arrow_height + border;
        }
      break;
    }
}


static void
btk_menu_handle_scrolling (BtkMenu *menu,
			   gint     x,
			   gint     y,
			   gboolean enter,
                           gboolean motion)
{
  BtkMenuShell *menu_shell;
  BtkMenuPrivate *priv;
  BdkRectangle rect;
  gboolean in_arrow;
  gboolean scroll_fast = FALSE;
  gint top_x, top_y;
  gboolean touchscreen_mode;

  priv = btk_menu_get_private (menu);

  menu_shell = BTK_MENU_SHELL (menu);

  g_object_get (btk_widget_get_settings (BTK_WIDGET (menu)),
                "btk-touchscreen-mode", &touchscreen_mode,
                NULL);

  bdk_window_get_position (menu->toplevel->window, &top_x, &top_y);
  x -= top_x;
  y -= top_y;

  /*  upper arrow handling  */

  get_arrows_sensitive_area (menu, &rect, NULL);

  in_arrow = FALSE;
  if (menu->upper_arrow_visible && !menu->tearoff_active &&
      (x >= rect.x) && (x < rect.x + rect.width) &&
      (y >= rect.y) && (y < rect.y + rect.height))
    {
      in_arrow = TRUE;
    }

  if (touchscreen_mode)
    menu->upper_arrow_prelight = in_arrow;

  if (priv->upper_arrow_state != BTK_STATE_INSENSITIVE)
    {
      gboolean arrow_pressed = FALSE;

      if (menu->upper_arrow_visible && !menu->tearoff_active)
        {
          if (touchscreen_mode)
            {
              if (enter && menu->upper_arrow_prelight)
                {
                  if (menu->timeout_id == 0)
                    {
                      /* Deselect the active item so that
                       * any submenus are popped down
                       */
                      btk_menu_shell_deselect (menu_shell);

                      btk_menu_remove_scroll_timeout (menu);
                      menu->scroll_step = -MENU_SCROLL_STEP2; /* always fast */

                      if (!motion)
                        {
                          /* Only do stuff on click. */
                          btk_menu_start_scrolling (menu);
                          arrow_pressed = TRUE;
                        }
                    }
                  else
                    {
                      arrow_pressed = TRUE;
                    }
                }
              else if (!enter)
                {
                  btk_menu_stop_scrolling (menu);
                }
            }
          else /* !touchscreen_mode */
            {
              scroll_fast = (y < rect.y + MENU_SCROLL_FAST_ZONE);

              if (enter && in_arrow &&
                  (!menu->upper_arrow_prelight ||
                   menu->scroll_fast != scroll_fast))
                {
                  menu->upper_arrow_prelight = TRUE;
                  menu->scroll_fast = scroll_fast;

                  /* Deselect the active item so that
                   * any submenus are popped down
                   */
                  btk_menu_shell_deselect (menu_shell);

                  btk_menu_remove_scroll_timeout (menu);
                  menu->scroll_step = scroll_fast ?
                    -MENU_SCROLL_STEP2 : -MENU_SCROLL_STEP1;

                  menu->timeout_id =
                    bdk_threads_add_timeout (scroll_fast ?
                                             MENU_SCROLL_TIMEOUT2 :
                                             MENU_SCROLL_TIMEOUT1,
                                             btk_menu_scroll_timeout, menu);
                }
              else if (!enter && !in_arrow && menu->upper_arrow_prelight)
                {
                  btk_menu_stop_scrolling (menu);
                }
            }
        }

      /*  btk_menu_start_scrolling() might have hit the top of the
       *  menu, so check if the button isn't insensitive before
       *  changing it to something else.
       */
      if (priv->upper_arrow_state != BTK_STATE_INSENSITIVE)
        {
          BtkStateType arrow_state = BTK_STATE_NORMAL;

          if (arrow_pressed)
            arrow_state = BTK_STATE_ACTIVE;
          else if (menu->upper_arrow_prelight)
            arrow_state = BTK_STATE_PRELIGHT;

          if (arrow_state != priv->upper_arrow_state)
            {
              priv->upper_arrow_state = arrow_state;

              bdk_window_invalidate_rect (BTK_WIDGET (menu)->window,
                                          &rect, FALSE);
            }
        }
    }

  /*  lower arrow handling  */

  get_arrows_sensitive_area (menu, NULL, &rect);

  in_arrow = FALSE;
  if (menu->lower_arrow_visible && !menu->tearoff_active &&
      (x >= rect.x) && (x < rect.x + rect.width) &&
      (y >= rect.y) && (y < rect.y + rect.height))
    {
      in_arrow = TRUE;
    }

  if (touchscreen_mode)
    menu->lower_arrow_prelight = in_arrow;

  if (priv->lower_arrow_state != BTK_STATE_INSENSITIVE)
    {
      gboolean arrow_pressed = FALSE;

      if (menu->lower_arrow_visible && !menu->tearoff_active)
        {
          if (touchscreen_mode)
            {
              if (enter && menu->lower_arrow_prelight)
                {
                  if (menu->timeout_id == 0)
                    {
                      /* Deselect the active item so that
                       * any submenus are popped down
                       */
                      btk_menu_shell_deselect (menu_shell);

                      btk_menu_remove_scroll_timeout (menu);
                      menu->scroll_step = MENU_SCROLL_STEP2; /* always fast */

                      if (!motion)
                        {
                          /* Only do stuff on click. */
                          btk_menu_start_scrolling (menu);
                          arrow_pressed = TRUE;
                        }
                    }
                  else
                    {
                      arrow_pressed = TRUE;
                    }
                }
              else if (!enter)
                {
                  btk_menu_stop_scrolling (menu);
                }
            }
          else /* !touchscreen_mode */
            {
              scroll_fast = (y > rect.y + rect.height - MENU_SCROLL_FAST_ZONE);

              if (enter && in_arrow &&
                  (!menu->lower_arrow_prelight ||
                   menu->scroll_fast != scroll_fast))
                {
                  menu->lower_arrow_prelight = TRUE;
                  menu->scroll_fast = scroll_fast;

                  /* Deselect the active item so that
                   * any submenus are popped down
                   */
                  btk_menu_shell_deselect (menu_shell);

                  btk_menu_remove_scroll_timeout (menu);
                  menu->scroll_step = scroll_fast ?
                    MENU_SCROLL_STEP2 : MENU_SCROLL_STEP1;

                  menu->timeout_id =
                    bdk_threads_add_timeout (scroll_fast ?
                                             MENU_SCROLL_TIMEOUT2 :
                                             MENU_SCROLL_TIMEOUT1,
                                             btk_menu_scroll_timeout, menu);
                }
              else if (!enter && !in_arrow && menu->lower_arrow_prelight)
                {
                  btk_menu_stop_scrolling (menu);
                }
            }
        }

      /*  btk_menu_start_scrolling() might have hit the bottom of the
       *  menu, so check if the button isn't insensitive before
       *  changing it to something else.
       */
      if (priv->lower_arrow_state != BTK_STATE_INSENSITIVE)
        {
          BtkStateType arrow_state = BTK_STATE_NORMAL;

          if (arrow_pressed)
            arrow_state = BTK_STATE_ACTIVE;
          else if (menu->lower_arrow_prelight)
            arrow_state = BTK_STATE_PRELIGHT;

          if (arrow_state != priv->lower_arrow_state)
            {
              priv->lower_arrow_state = arrow_state;

              bdk_window_invalidate_rect (BTK_WIDGET (menu)->window,
                                          &rect, FALSE);
            }
        }
    }
}

static gboolean
btk_menu_enter_notify (BtkWidget        *widget,
		       BdkEventCrossing *event)
{
  BtkWidget *menu_item;
  gboolean   touchscreen_mode;

  if (event->mode == BDK_CROSSING_BTK_GRAB ||
      event->mode == BDK_CROSSING_BTK_UNGRAB ||
      event->mode == BDK_CROSSING_STATE_CHANGED)
    return TRUE;

  g_object_get (btk_widget_get_settings (widget),
                "btk-touchscreen-mode", &touchscreen_mode,
                NULL);

  menu_item = btk_get_event_widget ((BdkEvent*) event);
  if (BTK_IS_MENU (widget))
    {
      BtkMenuShell *menu_shell = BTK_MENU_SHELL (widget);

      if (!menu_shell->ignore_enter)
	btk_menu_handle_scrolling (BTK_MENU (widget),
                                   event->x_root, event->y_root, TRUE, TRUE);
    }

  if (!touchscreen_mode && BTK_IS_MENU_ITEM (menu_item))
    {
      BtkWidget *menu = menu_item->parent;
      
      if (BTK_IS_MENU (menu))
	{
	  BtkMenuPrivate *priv = btk_menu_get_private (BTK_MENU (menu));
	  BtkMenuShell *menu_shell = BTK_MENU_SHELL (menu);

	  if (priv->seen_item_enter)
	    {
	      /* This is the second enter we see for an item
	       * on this menu. This means a release should always
	       * mean activate.
	       */
	      menu_shell->activate_time = 0;
	    }
	  else if ((event->detail != BDK_NOTIFY_NONLINEAR &&
		    event->detail != BDK_NOTIFY_NONLINEAR_VIRTUAL))
	    {
	      if (definitely_within_item (menu_item, event->x, event->y))
		{
		  /* This is an actual user-enter (ie. not a pop-under)
		   * In this case, the user must either have entered
		   * sufficiently far enough into the item, or he must move
		   * far enough away from the enter point. (see
		   * btk_menu_motion_notify())
		   */
		  menu_shell->activate_time = 0;
		}
	    }
	    
	  priv->seen_item_enter = TRUE;
	}
    }
  
  /* If this is a faked enter (see btk_menu_motion_notify), 'widget'
   * will not correspond to the event widget's parent.  Check to see
   * if we are in the parent's navigation rebunnyion.
   */
  if (BTK_IS_MENU_ITEM (menu_item) && BTK_IS_MENU (menu_item->parent) &&
      btk_menu_navigating_submenu (BTK_MENU (menu_item->parent),
                                   event->x_root, event->y_root))
    return TRUE;

  return BTK_WIDGET_CLASS (btk_menu_parent_class)->enter_notify_event (widget, event); 
}

static gboolean
btk_menu_leave_notify (BtkWidget        *widget,
		       BdkEventCrossing *event)
{
  BtkMenuShell *menu_shell;
  BtkMenu *menu;
  BtkMenuItem *menu_item;
  BtkWidget *event_widget;

  if (event->mode == BDK_CROSSING_BTK_GRAB ||
      event->mode == BDK_CROSSING_BTK_UNGRAB ||
      event->mode == BDK_CROSSING_STATE_CHANGED)
    return TRUE;

  menu = BTK_MENU (widget);
  menu_shell = BTK_MENU_SHELL (widget); 
  
  if (btk_menu_navigating_submenu (menu, event->x_root, event->y_root))
    return TRUE; 

  btk_menu_handle_scrolling (menu, event->x_root, event->y_root, FALSE, TRUE);

  event_widget = btk_get_event_widget ((BdkEvent*) event);
  
  if (!BTK_IS_MENU_ITEM (event_widget))
    return TRUE;
  
  menu_item = BTK_MENU_ITEM (event_widget); 

  /* Here we check to see if we're leaving an active menu item with a submenu, 
   * in which case we enter submenu navigation mode. 
   */
  if (menu_shell->active_menu_item != NULL
      && menu_item->submenu != NULL
      && menu_item->submenu_placement == BTK_LEFT_RIGHT)
    {
      if (BTK_MENU_SHELL (menu_item->submenu)->active)
	{
	  btk_menu_set_submenu_navigation_rebunnyion (menu, menu_item, event);
	  return TRUE;
	}
      else if (menu_item == BTK_MENU_ITEM (menu_shell->active_menu_item))
	{
	  /* We are leaving an active menu item with nonactive submenu.
	   * Deselect it so we don't surprise the user with by popping
	   * up a submenu _after_ he left the item.
	   */
	  btk_menu_shell_deselect (menu_shell);
	  return TRUE;
	}
    }
  
  return BTK_WIDGET_CLASS (btk_menu_parent_class)->leave_notify_event (widget, event); 
}

static void 
btk_menu_stop_navigating_submenu (BtkMenu *menu)
{
  BtkMenuPrivate *priv = btk_menu_get_private (menu);

  priv->navigation_x = 0;
  priv->navigation_y = 0;
  priv->navigation_width = 0;
  priv->navigation_height = 0;

  if (menu->navigation_timeout)
    {
      g_source_remove (menu->navigation_timeout);
      menu->navigation_timeout = 0;
    }
}

/* When the timeout is elapsed, the navigation rebunnyion is destroyed
 * and the menuitem under the pointer (if any) is selected.
 */
static gboolean
btk_menu_stop_navigating_submenu_cb (gpointer user_data)
{
  BtkMenu *menu = user_data;
  BdkWindow *child_window;

  btk_menu_stop_navigating_submenu (menu);
  
  if (btk_widget_get_realized (BTK_WIDGET (menu)))
    {
      child_window = bdk_window_get_pointer (menu->bin_window, NULL, NULL, NULL);

      if (child_window)
	{
	  BdkEvent *send_event = bdk_event_new (BDK_ENTER_NOTIFY);

	  send_event->crossing.window = g_object_ref (child_window);
	  send_event->crossing.time = BDK_CURRENT_TIME; /* Bogus */
	  send_event->crossing.send_event = TRUE;

	  BTK_WIDGET_CLASS (btk_menu_parent_class)->enter_notify_event (BTK_WIDGET (menu), (BdkEventCrossing *)send_event);

	  bdk_event_free (send_event);
	}
    }

  return FALSE; 
}

static gboolean
btk_menu_navigating_submenu (BtkMenu *menu,
			     gint     event_x,
			     gint     event_y)
{
  BtkMenuPrivate *priv;
  int width, height;

  if (!btk_menu_has_navigation_triangle (menu))
    return FALSE;

  priv = btk_menu_get_private (menu);
  width = priv->navigation_width;
  height = priv->navigation_height;

  /* check if x/y are in the triangle spanned by the navigation parameters */

  /* 1) Move the coordinates so the triangle starts at 0,0 */
  event_x -= priv->navigation_x;
  event_y -= priv->navigation_y;

  /* 2) Ensure both legs move along the positive axis */
  if (width < 0)
    {
      event_x = -event_x;
      width = -width;
    }
  if (height < 0)
    {
      event_y = -event_y;
      height = -height;
    }

  /* 3) Check that the given coordinate is inside the triangle. The formula
   * is a transformed form of this formula: x/w + y/h <= 1
   */
  if (event_x >= 0 && event_y >= 0 &&
      event_x * height + event_y * width <= width * height)
    {
      return TRUE;
    }
  else
    {
      btk_menu_stop_navigating_submenu (menu);
      return FALSE;
    }
}

static void
btk_menu_set_submenu_navigation_rebunnyion (BtkMenu          *menu,
					BtkMenuItem      *menu_item,
					BdkEventCrossing *event)
{
  gint submenu_left = 0;
  gint submenu_right = 0;
  gint submenu_top = 0;
  gint submenu_bottom = 0;
  gint width = 0;
  gint height = 0;
  BtkWidget *event_widget;
  BtkMenuPrivate *priv;

  g_return_if_fail (menu_item->submenu != NULL);
  g_return_if_fail (event != NULL);
  
  priv = btk_menu_get_private (menu);

  event_widget = btk_get_event_widget ((BdkEvent*) event);
  
  bdk_window_get_origin (menu_item->submenu->window, &submenu_left, &submenu_top);
  width = bdk_window_get_width (menu_item->submenu->window);
  height = bdk_window_get_height (menu_item->submenu->window);
  
  submenu_right = submenu_left + width;
  submenu_bottom = submenu_top + height;
  
  width = bdk_window_get_width (event_widget->window);
  height = bdk_window_get_height (event_widget->window);
  
  if (event->x >= 0 && event->x < width)
    {
      gint popdown_delay;
      
      btk_menu_stop_navigating_submenu (menu);

      /* The navigation rebunnyion is the triangle closest to the x/y
       * location of the rectangle. This is why the width or height
       * can be negative.
       */

      if (menu_item->submenu_direction == BTK_DIRECTION_RIGHT)
	{
	  /* right */
          priv->navigation_x = submenu_left;
          priv->navigation_width = event->x_root - submenu_left;
	}
      else
	{
	  /* left */
          priv->navigation_x = submenu_right;
          priv->navigation_width = event->x_root - submenu_right;
	}

      if (event->y < 0)
	{
	  /* top */
          priv->navigation_y = event->y_root;
          priv->navigation_height = submenu_top - event->y_root - NAVIGATION_REBUNNYION_OVERSHOOT;

	  if (priv->navigation_height >= 0)
	    return;
	}
      else
	{
	  /* bottom */
          priv->navigation_y = event->y_root;
          priv->navigation_height = submenu_bottom - event->y_root + NAVIGATION_REBUNNYION_OVERSHOOT;

	  if (priv->navigation_height <= 0)
	    return;
	}

      g_object_get (btk_widget_get_settings (BTK_WIDGET (menu)),
		    "btk-menu-popdown-delay", &popdown_delay,
		    NULL);

      menu->navigation_timeout = bdk_threads_add_timeout (popdown_delay,
                                                          btk_menu_stop_navigating_submenu_cb,
                                                          menu);
    }
}

static void
btk_menu_deactivate (BtkMenuShell *menu_shell)
{
  BtkWidget *parent;
  
  g_return_if_fail (BTK_IS_MENU (menu_shell));
  
  parent = menu_shell->parent_menu_shell;
  
  menu_shell->activate_time = 0;
  btk_menu_popdown (BTK_MENU (menu_shell));
  
  if (parent)
    btk_menu_shell_deactivate (BTK_MENU_SHELL (parent));
}

static void
btk_menu_position (BtkMenu  *menu,
                   gboolean  set_scroll_offset)
{
  BtkWidget *widget;
  BtkRequisition requisition;
  BtkMenuPrivate *private;
  gint x, y;
  gint scroll_offset;
  gint menu_height;
  BdkScreen *screen;
  BdkScreen *pointer_screen;
  BdkRectangle monitor;
  
  g_return_if_fail (BTK_IS_MENU (menu));

  widget = BTK_WIDGET (menu);

  screen = btk_widget_get_screen (widget);
  bdk_display_get_pointer (bdk_screen_get_display (screen),
			   &pointer_screen, &x, &y, NULL);
  
  /* We need the requisition to figure out the right place to
   * popup the menu. In fact, we always need to ask here, since
   * if a size_request was queued while we weren't popped up,
   * the requisition won't have been recomputed yet.
   */
  btk_widget_size_request (widget, &requisition);

  if (pointer_screen != screen)
    {
      /* Pointer is on a different screen; roughly center the
       * menu on the screen. If someone was using multiscreen
       * + Xinerama together they'd probably want something
       * fancier; but that is likely to be vanishingly rare.
       */
      x = MAX (0, (bdk_screen_get_width (screen) - requisition.width) / 2);
      y = MAX (0, (bdk_screen_get_height (screen) - requisition.height) / 2);
    }

  private = btk_menu_get_private (menu);
  private->monitor_num = bdk_screen_get_monitor_at_point (screen, x, y);

  private->initially_pushed_in = FALSE;

  /* Set the type hint here to allow custom position functions to set a different hint */
  if (!btk_widget_get_visible (menu->toplevel))
    btk_window_set_type_hint (BTK_WINDOW (menu->toplevel), BDK_WINDOW_TYPE_HINT_POPUP_MENU);
  
  if (menu->position_func)
    {
      (* menu->position_func) (menu, &x, &y, &private->initially_pushed_in,
                               menu->position_func_data);

      if (private->monitor_num < 0) 
	private->monitor_num = bdk_screen_get_monitor_at_point (screen, x, y);

      bdk_screen_get_monitor_geometry (screen, private->monitor_num, &monitor);
    }
  else
    {
      gint space_left, space_right, space_above, space_below;
      gint needed_width;
      gint needed_height;
      gint xthickness = widget->style->xthickness;
      gint ythickness = widget->style->ythickness;
      gboolean rtl = (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL);

      /* The placement of popup menus horizontally works like this (with
       * RTL in parentheses)
       *
       * - If there is enough room to the right (left) of the mouse cursor,
       *   position the menu there.
       * 
       * - Otherwise, if if there is enough room to the left (right) of the 
       *   mouse cursor, position the menu there.
       * 
       * - Otherwise if the menu is smaller than the monitor, position it
       *   on the side of the mouse cursor that has the most space available
       *
       * - Otherwise (if there is simply not enough room for the menu on the
       *   monitor), position it as far left (right) as possible.
       *
       * Positioning in the vertical direction is similar: first try below
       * mouse cursor, then above.
       */
      bdk_screen_get_monitor_geometry (screen, private->monitor_num, &monitor);

      space_left = x - monitor.x;
      space_right = monitor.x + monitor.width - x - 1;
      space_above = y - monitor.y;
      space_below = monitor.y + monitor.height - y - 1;

      /* position horizontally */

      /* the amount of space we need to position the menu. Note the
       * menu is offset "xthickness" pixels 
       */
      needed_width = requisition.width - xthickness;

      if (needed_width <= space_left ||
	  needed_width <= space_right)
	{
	  if ((rtl  && needed_width <= space_left) ||
	      (!rtl && needed_width >  space_right))
	    {
	      /* position left */
	      x = x + xthickness - requisition.width + 1;
	    }
	  else
	    {
	      /* position right */
	      x = x - xthickness;
	    }

	  /* x is clamped on-screen further down */
	}
      else if (requisition.width <= monitor.width)
	{
	  /* the menu is too big to fit on either side of the mouse
	   * cursor, but smaller than the monitor. Position it on
	   * the side that has the most space
	   */
	  if (space_left > space_right)
	    {
	      /* left justify */
	      x = monitor.x;
	    }
	  else
	    {
	      /* right justify */
	      x = monitor.x + monitor.width - requisition.width;
	    }
	}
      else /* menu is simply too big for the monitor */
	{
	  if (rtl)
	    {
	      /* right justify */
	      x = monitor.x + monitor.width - requisition.width;
	    }
	  else
	    {
	      /* left justify */
	      x = monitor.x;
	    }
	}

      /* Position vertically. The algorithm is the same as above, but
       * simpler because we don't have to take RTL into account.
       */
      needed_height = requisition.height - ythickness;

      if (needed_height <= space_above ||
	  needed_height <= space_below)
	{
	  if (needed_height <= space_below)
	    y = y - ythickness;
	  else
	    y = y + ythickness - requisition.height + 1;
	  
	  y = CLAMP (y, monitor.y,
		     monitor.y + monitor.height - requisition.height);
	}
      else if (needed_height > space_below && needed_height > space_above)
	{
	  if (space_below >= space_above)
	    y = monitor.y + monitor.height - requisition.height;
	  else
	    y = monitor.y;
	}
      else
	{
	  y = monitor.y;
	}
    }

  scroll_offset = 0;

  if (private->initially_pushed_in)
    {
      menu_height = BTK_WIDGET (menu)->requisition.height;

      if (y + menu_height > monitor.y + monitor.height)
	{
	  scroll_offset -= y + menu_height - (monitor.y + monitor.height);
	  y = (monitor.y + monitor.height) - menu_height;
	}
  
      if (y < monitor.y)
	{
	  scroll_offset += monitor.y - y;
	  y = monitor.y;
	}
    }

  /* FIXME: should this be done in the various position_funcs ? */
  x = CLAMP (x, monitor.x, MAX (monitor.x, monitor.x + monitor.width - requisition.width));
 
  if (BTK_MENU_SHELL (menu)->active)
    {
      private->have_position = TRUE;
      private->x = x;
      private->y = y;
    }
  
  if (y + requisition.height > monitor.y + monitor.height)
    requisition.height = (monitor.y + monitor.height) - y;
  
  if (y < monitor.y)
    {
      scroll_offset += monitor.y - y;
      requisition.height -= monitor.y - y;
      y = monitor.y;
    }

  if (scroll_offset > 0)
    {
      BtkBorder arrow_border;

      get_arrows_border (menu, &arrow_border);
      scroll_offset += arrow_border.top;
    }
  
  btk_window_move (BTK_WINDOW (BTK_MENU_SHELL (menu)->active ? menu->toplevel : menu->tearoff_window), 
		   x, y);

  if (!BTK_MENU_SHELL (menu)->active)
    {
      btk_window_resize (BTK_WINDOW (menu->tearoff_window),
			 requisition.width, requisition.height);
    }

  if (set_scroll_offset)
    menu->scroll_offset = scroll_offset;
}

static void
btk_menu_remove_scroll_timeout (BtkMenu *menu)
{
  if (menu->timeout_id)
    {
      g_source_remove (menu->timeout_id);
      menu->timeout_id = 0;
    }
}

static void
btk_menu_stop_scrolling (BtkMenu *menu)
{
  gboolean touchscreen_mode;

  btk_menu_remove_scroll_timeout (menu);

  g_object_get (btk_widget_get_settings (BTK_WIDGET (menu)),
		"btk-touchscreen-mode", &touchscreen_mode,
		NULL);

  if (!touchscreen_mode)
    {
      menu->upper_arrow_prelight = FALSE;
      menu->lower_arrow_prelight = FALSE;
    }
}

static void
btk_menu_scroll_to (BtkMenu *menu,
		    gint    offset)
{
  BtkWidget *widget;
  gint x, y;
  gint view_width, view_height;
  gint border_width;
  gint menu_height;
  guint vertical_padding;
  guint horizontal_padding;
  gboolean double_arrows;
  BtkBorder arrow_border;
  
  widget = BTK_WIDGET (menu);

  if (menu->tearoff_active &&
      menu->tearoff_adjustment &&
      (menu->tearoff_adjustment->value != offset))
    {
      menu->tearoff_adjustment->value =
	CLAMP (offset,
	       0, menu->tearoff_adjustment->upper - menu->tearoff_adjustment->page_size);
      btk_adjustment_value_changed (menu->tearoff_adjustment);
    }
  
  /* Move/resize the viewport according to arrows: */
  view_width = widget->allocation.width;
  view_height = widget->allocation.height;

  btk_widget_style_get (BTK_WIDGET (menu),
                        "vertical-padding", &vertical_padding,
                        "horizontal-padding", &horizontal_padding,
                        NULL);

  double_arrows = get_double_arrows (menu);

  border_width = BTK_CONTAINER (menu)->border_width;
  view_width -= (border_width + widget->style->xthickness + horizontal_padding) * 2;
  view_height -= (border_width + widget->style->ythickness + vertical_padding) * 2;
  menu_height = widget->requisition.height -
    (border_width + widget->style->ythickness + vertical_padding) * 2;

  x = border_width + widget->style->xthickness + horizontal_padding;
  y = border_width + widget->style->ythickness + vertical_padding;

  if (double_arrows && !menu->tearoff_active)
    {
      if (view_height < menu_height               ||
          (offset > 0 && menu->scroll_offset > 0) ||
          (offset < 0 && menu->scroll_offset < 0))
        {
          BtkMenuPrivate *priv = btk_menu_get_private (menu);
          BtkStateType    upper_arrow_previous_state = priv->upper_arrow_state;
          BtkStateType    lower_arrow_previous_state = priv->lower_arrow_state;

          if (!menu->upper_arrow_visible || !menu->lower_arrow_visible)
            btk_widget_queue_draw (BTK_WIDGET (menu));

          menu->upper_arrow_visible = menu->lower_arrow_visible = TRUE;

	  get_arrows_border (menu, &arrow_border);
	  y += arrow_border.top;
	  view_height -= arrow_border.top;
	  view_height -= arrow_border.bottom;

          if (offset <= 0)
            priv->upper_arrow_state = BTK_STATE_INSENSITIVE;
          else if (priv->upper_arrow_state == BTK_STATE_INSENSITIVE)
            priv->upper_arrow_state = menu->upper_arrow_prelight ?
              BTK_STATE_PRELIGHT : BTK_STATE_NORMAL;

          if (offset >= menu_height - view_height)
            priv->lower_arrow_state = BTK_STATE_INSENSITIVE;
          else if (priv->lower_arrow_state == BTK_STATE_INSENSITIVE)
            priv->lower_arrow_state = menu->lower_arrow_prelight ?
              BTK_STATE_PRELIGHT : BTK_STATE_NORMAL;

          if ((priv->upper_arrow_state != upper_arrow_previous_state) ||
              (priv->lower_arrow_state != lower_arrow_previous_state))
            btk_widget_queue_draw (BTK_WIDGET (menu));

          if (upper_arrow_previous_state != BTK_STATE_INSENSITIVE &&
              priv->upper_arrow_state == BTK_STATE_INSENSITIVE)
            {
              /* At the upper border, possibly remove timeout */
              if (menu->scroll_step < 0)
                {
                  btk_menu_stop_scrolling (menu);
                  btk_widget_queue_draw (BTK_WIDGET (menu));
                }
            }

          if (lower_arrow_previous_state != BTK_STATE_INSENSITIVE &&
              priv->lower_arrow_state == BTK_STATE_INSENSITIVE)
            {
              /* At the lower border, possibly remove timeout */
              if (menu->scroll_step > 0)
                {
                  btk_menu_stop_scrolling (menu);
                  btk_widget_queue_draw (BTK_WIDGET (menu));
                }
            }
        }
      else if (menu->upper_arrow_visible || menu->lower_arrow_visible)
        {
          offset = 0;

          menu->upper_arrow_visible = menu->lower_arrow_visible = FALSE;
          menu->upper_arrow_prelight = menu->lower_arrow_prelight = FALSE;

          btk_menu_stop_scrolling (menu);
          btk_widget_queue_draw (BTK_WIDGET (menu));
        }
    }
  else if (!menu->tearoff_active)
    {
      gboolean last_visible;

      last_visible = menu->upper_arrow_visible;
      menu->upper_arrow_visible = offset > 0;
      
      /* upper_arrow_visible may have changed, so requery the border */
      get_arrows_border (menu, &arrow_border);
      view_height -= arrow_border.top;
      
      if ((last_visible != menu->upper_arrow_visible) &&
          !menu->upper_arrow_visible)
	{
          menu->upper_arrow_prelight = FALSE;

	  /* If we hid the upper arrow, possibly remove timeout */
	  if (menu->scroll_step < 0)
	    {
	      btk_menu_stop_scrolling (menu);
	      btk_widget_queue_draw (BTK_WIDGET (menu));
	    }
	}

      last_visible = menu->lower_arrow_visible;
      menu->lower_arrow_visible = offset < menu_height - view_height;
      
      /* lower_arrow_visible may have changed, so requery the border */
      get_arrows_border (menu, &arrow_border);
      view_height -= arrow_border.bottom;
      
      if ((last_visible != menu->lower_arrow_visible) &&
	   !menu->lower_arrow_visible)
	{
          menu->lower_arrow_prelight = FALSE;

	  /* If we hid the lower arrow, possibly remove timeout */
	  if (menu->scroll_step > 0)
	    {
	      btk_menu_stop_scrolling (menu);
	      btk_widget_queue_draw (BTK_WIDGET (menu));
	    }
	}
      
      y += arrow_border.top;
    }

  /* Scroll the menu: */
  if (btk_widget_get_realized (widget))
    bdk_window_move (menu->bin_window, 0, -offset);

  if (btk_widget_get_realized (widget))
    bdk_window_move_resize (menu->view_window,
			    x,
			    y,
			    view_width,
			    view_height);

  menu->scroll_offset = offset;
}

static gboolean
compute_child_offset (BtkMenu   *menu,
		      BtkWidget *menu_item,
		      gint      *offset,
		      gint      *height,
		      gboolean  *is_last_child)
{
  BtkMenuPrivate *priv = btk_menu_get_private (menu);
  gint item_top_attach;
  gint item_bottom_attach;
  gint child_offset = 0;
  gint i;

  get_effective_child_attach (menu_item, NULL, NULL,
			      &item_top_attach, &item_bottom_attach);

  /* there is a possibility that we get called before _size_request, so
   * check the height table for safety.
   */
  if (!priv->heights || priv->heights_length < btk_menu_get_n_rows (menu))
    return FALSE;

  /* when we have a row with only invisible children, it's height will
   * be zero, so there's no need to check WIDGET_VISIBLE here
   */
  for (i = 0; i < item_top_attach; i++)
    child_offset += priv->heights[i];

  if (is_last_child)
    *is_last_child = (item_bottom_attach == btk_menu_get_n_rows (menu));
  if (offset)
    *offset = child_offset;
  if (height)
    *height = priv->heights[item_top_attach];

  return TRUE;
}

static void
btk_menu_scroll_item_visible (BtkMenuShell *menu_shell,
			      BtkWidget    *menu_item)
{
  BtkMenu *menu;
  gint child_offset, child_height;
  gint width, height;
  gint y;
  gint arrow_height;
  gboolean last_child = 0;
  
  menu = BTK_MENU (menu_shell);

  /* We need to check if the selected item fully visible.
   * If not we need to scroll the menu so that it becomes fully
   * visible.
   */

  if (compute_child_offset (menu, menu_item,
			    &child_offset, &child_height, &last_child))
    {
      guint vertical_padding;
      gboolean double_arrows;
      
      y = menu->scroll_offset;
      width = bdk_window_get_width (BTK_WIDGET (menu)->window);
      height = bdk_window_get_height (BTK_WIDGET (menu)->window);

      btk_widget_style_get (BTK_WIDGET (menu),
			    "vertical-padding", &vertical_padding,
                            NULL);

      double_arrows = get_double_arrows (menu);

      height -= 2*BTK_CONTAINER (menu)->border_width + 2*BTK_WIDGET (menu)->style->ythickness + 2*vertical_padding;
      
      if (child_offset < y)
	{
	  /* Ignore the enter event we might get if the pointer is on the menu
	   */
	  menu_shell->ignore_enter = TRUE;
	  btk_menu_scroll_to (menu, child_offset);
	}
      else
	{
          BtkBorder arrow_border;

          arrow_height = 0;

          get_arrows_border (menu, &arrow_border);
          if (!menu->tearoff_active)
            arrow_height = arrow_border.top + arrow_border.bottom;
	  
	  if (child_offset + child_height > y + height - arrow_height)
	    {
	      arrow_height = 0;
	      if ((!last_child && !menu->tearoff_active) || double_arrows)
		arrow_height += arrow_border.bottom;

	      y = child_offset + child_height - height + arrow_height;
	      if (((y > 0) && !menu->tearoff_active) || double_arrows)
		{
		  /* Need upper arrow */
		  arrow_height += arrow_border.top;
		  y = child_offset + child_height - height + arrow_height;
		}
	      /* Ignore the enter event we might get if the pointer is on the menu
	       */
	      menu_shell->ignore_enter = TRUE;
	      btk_menu_scroll_to (menu, y);
	    }
	}    
      
    }
}

static void
btk_menu_select_item (BtkMenuShell *menu_shell,
		      BtkWidget    *menu_item)
{
  BtkMenu *menu = BTK_MENU (menu_shell);

  if (btk_widget_get_realized (BTK_WIDGET (menu)))
    btk_menu_scroll_item_visible (menu_shell, menu_item);

  BTK_MENU_SHELL_CLASS (btk_menu_parent_class)->select_item (menu_shell, menu_item);
}


/* Reparent the menu, taking care of the refcounting
 *
 * If unrealize is true we force a unrealize while reparenting the parent.
 * This can help eliminate flicker in some cases.
 *
 * What happens is that when the menu is unrealized and then re-realized,
 * the allocations are as follows:
 *
 *  parent - 1x1 at (0,0) 
 *  child1 - 100x20 at (0,0)
 *  child2 - 100x20 at (0,20)
 *  child3 - 100x20 at (0,40)
 *
 * That is, the parent is small but the children are full sized. Then,
 * when the queued_resize gets processed, the parent gets resized to
 * full size. 
 *
 * But in order to eliminate flicker when scrolling, bdkgeometry-x11.c
 * contains the following logic:
 * 
 * - if a move or resize operation on a window would change the clip 
 *   rebunnyion on the children, then before the window is resized
 *   the background for children is temporarily set to None, the
 *   move/resize done, and the background for the children restored.
 *
 * So, at the point where the parent is resized to final size, the
 * background for the children is temporarily None, and thus they
 * are not cleared to the background color and the previous background
 * (the image of the menu) is left in place.
 */
static void 
btk_menu_reparent (BtkMenu   *menu,
                   BtkWidget *new_parent,
                   gboolean   unrealize)
{
  BtkObject *object = BTK_OBJECT (menu);
  BtkWidget *widget = BTK_WIDGET (menu);
  gboolean was_floating = g_object_is_floating (object);

  g_object_ref_sink (object);

  if (unrealize)
    {
      g_object_ref (object);
      btk_container_remove (BTK_CONTAINER (widget->parent), widget);
      btk_container_add (BTK_CONTAINER (new_parent), widget);
      g_object_unref (object);
    }
  else
    btk_widget_reparent (BTK_WIDGET (menu), new_parent);
  
  if (was_floating)
    g_object_force_floating (B_OBJECT (object));
  else
    g_object_unref (object);
}

static void
btk_menu_show_all (BtkWidget *widget)
{
  /* Show children, but not self. */
  btk_container_foreach (BTK_CONTAINER (widget), (BtkCallback) btk_widget_show_all, NULL);
}


static void
btk_menu_hide_all (BtkWidget *widget)
{
  /* Hide children, but not self. */
  btk_container_foreach (BTK_CONTAINER (widget), (BtkCallback) btk_widget_hide_all, NULL);
}

/**
 * btk_menu_set_screen:
 * @menu: a #BtkMenu.
 * @screen: (allow-none): a #BdkScreen, or %NULL if the screen should be
 *          determined by the widget the menu is attached to.
 *
 * Sets the #BdkScreen on which the menu will be displayed.
 *
 * Since: 2.2
 **/
void
btk_menu_set_screen (BtkMenu   *menu, 
		     BdkScreen *screen)
{
  g_return_if_fail (BTK_IS_MENU (menu));
  g_return_if_fail (!screen || BDK_IS_SCREEN (screen));

  g_object_set_data (B_OBJECT (menu), I_("btk-menu-explicit-screen"), screen);

  if (screen)
    {
      menu_change_screen (menu, screen);
    }
  else
    {
      BtkWidget *attach_widget = btk_menu_get_attach_widget (menu);
      if (attach_widget)
	attach_widget_screen_changed (attach_widget, NULL, menu);
    }
}

/**
 * btk_menu_attach:
 * @menu: a #BtkMenu.
 * @child: a #BtkMenuItem.
 * @left_attach: The column number to attach the left side of the item to.
 * @right_attach: The column number to attach the right side of the item to.
 * @top_attach: The row number to attach the top of the item to.
 * @bottom_attach: The row number to attach the bottom of the item to.
 *
 * Adds a new #BtkMenuItem to a (table) menu. The number of 'cells' that
 * an item will occupy is specified by @left_attach, @right_attach,
 * @top_attach and @bottom_attach. These each represent the leftmost,
 * rightmost, uppermost and lower column and row numbers of the table.
 * (Columns and rows are indexed from zero).
 *
 * Note that this function is not related to btk_menu_detach().
 *
 * Since: 2.4
 **/
void
btk_menu_attach (BtkMenu   *menu,
                 BtkWidget *child,
                 guint      left_attach,
                 guint      right_attach,
                 guint      top_attach,
                 guint      bottom_attach)
{
  BtkMenuShell *menu_shell;
  
  g_return_if_fail (BTK_IS_MENU (menu));
  g_return_if_fail (BTK_IS_MENU_ITEM (child));
  g_return_if_fail (child->parent == NULL || 
		    child->parent == BTK_WIDGET (menu));
  g_return_if_fail (left_attach < right_attach);
  g_return_if_fail (top_attach < bottom_attach);

  menu_shell = BTK_MENU_SHELL (menu);
  
  if (!child->parent)
    {
      AttachInfo *ai = get_attach_info (child);
      
      ai->left_attach = left_attach;
      ai->right_attach = right_attach;
      ai->top_attach = top_attach;
      ai->bottom_attach = bottom_attach;
      
      menu_shell->children = g_list_append (menu_shell->children, child);

      btk_widget_set_parent (child, BTK_WIDGET (menu));

      menu_queue_resize (menu);
    }
  else
    {
      btk_container_child_set (BTK_CONTAINER (child->parent), child,
			       "left-attach",   left_attach,
			       "right-attach",  right_attach,
			       "top-attach",    top_attach,
			       "bottom-attach", bottom_attach,
			       NULL);
    }
}

static gint
btk_menu_get_popup_delay (BtkMenuShell *menu_shell)
{
  gint popup_delay;

  g_object_get (btk_widget_get_settings (BTK_WIDGET (menu_shell)),
		"btk-menu-popup-delay", &popup_delay,
		NULL);

  return popup_delay;
}

static BtkWidget *
find_child_containing (BtkMenuShell *menu_shell,
                       int           left,
                       int           right,
                       int           top,
                       int           bottom)
{
  GList *list;

  /* find a child which includes the area given by
   * left, right, top, bottom.
   */

  for (list = menu_shell->children; list; list = list->next)
    {
      gint l, r, t, b;

      if (!_btk_menu_item_is_selectable (list->data))
        continue;

      get_effective_child_attach (list->data, &l, &r, &t, &b);

      if (l <= left && right <= r
          && t <= top && bottom <= b)
        return BTK_WIDGET (list->data);
    }

  return NULL;
}

static void
btk_menu_move_current (BtkMenuShell         *menu_shell,
                       BtkMenuDirectionType  direction)
{
  BtkMenu *menu = BTK_MENU (menu_shell);
  gint i;
  gint l, r, t, b;
  BtkWidget *match = NULL;

  if (btk_widget_get_direction (BTK_WIDGET (menu_shell)) == BTK_TEXT_DIR_RTL)
    {
      switch (direction)
	{
	case BTK_MENU_DIR_CHILD:
	  direction = BTK_MENU_DIR_PARENT;
	  break;
	case BTK_MENU_DIR_PARENT:
	  direction = BTK_MENU_DIR_CHILD;
	  break;
	default: ;
	}
    }

  /* use special table menu key bindings */
  if (menu_shell->active_menu_item && btk_menu_get_n_columns (menu) > 1)
    {
      get_effective_child_attach (menu_shell->active_menu_item, &l, &r, &t, &b);

      if (direction == BTK_MENU_DIR_NEXT)
        {
	  for (i = b; i < btk_menu_get_n_rows (menu); i++)
            {
              match = find_child_containing (menu_shell, l, l + 1, i, i + 1);
              if (match)
                break;
            }

	  if (!match)
	    {
	      /* wrap around */
	      for (i = 0; i < t; i++)
	        {
                  match = find_child_containing (menu_shell,
                                                 l, l + 1, i, i + 1);
                  if (match)
                    break;
		}
	    }
	}
      else if (direction == BTK_MENU_DIR_PREV)
        {
          for (i = t; i > 0; i--)
            {
              match = find_child_containing (menu_shell, l, l + 1, i - 1, i);
              if (match)
                break;
            }

	  if (!match)
	    {
	      /* wrap around */
	      for (i = btk_menu_get_n_rows (menu); i > b; i--)
	        {
                  match = find_child_containing (menu_shell,
                                                 l, l + 1, i - 1, i);
                  if (match)
		    break;
		}
	    }
	}
      else if (direction == BTK_MENU_DIR_PARENT)
        {
          /* we go one left if possible */
          if (l > 0)
            match = find_child_containing (menu_shell, l - 1, l, t, t + 1);

          if (!match)
            {
              BtkWidget *parent = menu_shell->parent_menu_shell;

              if (!parent
                  || g_list_length (BTK_MENU_SHELL (parent)->children) <= 1)
                match = menu_shell->active_menu_item;
            }
        }
      else if (direction == BTK_MENU_DIR_CHILD)
        {
          /* we go one right if possible */
	  if (r < btk_menu_get_n_columns (menu))
            match = find_child_containing (menu_shell, r, r + 1, t, t + 1);

          if (!match)
            {
              BtkWidget *parent = menu_shell->parent_menu_shell;

              if (! BTK_MENU_ITEM (menu_shell->active_menu_item)->submenu &&
                  (!parent ||
                   g_list_length (BTK_MENU_SHELL (parent)->children) <= 1))
                match = menu_shell->active_menu_item;
            }
        }

      if (match)
        {
	  btk_menu_shell_select_item (menu_shell, match);
          return;
        }
    }

  BTK_MENU_SHELL_CLASS (btk_menu_parent_class)->move_current (menu_shell, direction);
}

static gint
get_visible_size (BtkMenu *menu)
{
  BtkWidget *widget = BTK_WIDGET (menu);
  BtkContainer *container = BTK_CONTAINER (menu);
  
  gint menu_height = (widget->allocation.height
		      - 2 * (container->border_width
			     + widget->style->ythickness));

  if (!menu->tearoff_active)
    {
      BtkBorder arrow_border;

      get_arrows_border (menu, &arrow_border);
      menu_height -= arrow_border.top;
      menu_height -= arrow_border.bottom;
    }
  
  return menu_height;
}

/* Find the sensitive on-screen child containing @y, or if none,
 * the nearest selectable onscreen child. (%NULL if none)
 */
static BtkWidget *
child_at (BtkMenu *menu,
	  gint     y)
{
  BtkMenuShell *menu_shell = BTK_MENU_SHELL (menu);
  BtkWidget *child = NULL;
  gint child_offset = 0;
  GList *children;
  gint menu_height;
  gint lower, upper;		/* Onscreen bounds */

  menu_height = get_visible_size (menu);
  lower = menu->scroll_offset;
  upper = menu->scroll_offset + menu_height;
  
  for (children = menu_shell->children; children; children = children->next)
    {
      if (btk_widget_get_visible (children->data))
	{
	  BtkRequisition child_requisition;

	  btk_widget_size_request (children->data, &child_requisition);

	  if (_btk_menu_item_is_selectable (children->data) &&
	      child_offset >= lower &&
	      child_offset + child_requisition.height <= upper)
	    {
	      child = children->data;
	      
	      if (child_offset + child_requisition.height > y &&
		  !BTK_IS_TEAROFF_MENU_ITEM (child))
		return child;
	    }
      
	  child_offset += child_requisition.height;
	}
    }

  return child;
}

static gint
get_menu_height (BtkMenu *menu)
{
  gint height;
  BtkWidget *widget = BTK_WIDGET (menu);

  height = widget->requisition.height;
  height -= (BTK_CONTAINER (widget)->border_width + widget->style->ythickness) * 2;

  if (!menu->tearoff_active)
    {
      BtkBorder arrow_border;

      get_arrows_border (menu, &arrow_border);
      height -= arrow_border.top;
      height -= arrow_border.bottom;
    }

  return height;
}

static void
btk_menu_real_move_scroll (BtkMenu       *menu,
			   BtkScrollType  type)
{
  gint page_size = get_visible_size (menu);
  gint end_position = get_menu_height (menu);
  BtkMenuShell *menu_shell = BTK_MENU_SHELL (menu);
  
  switch (type)
    {
    case BTK_SCROLL_PAGE_UP:
    case BTK_SCROLL_PAGE_DOWN:
      {
	gint old_offset;
        gint new_offset;
	gint child_offset = 0;
	gboolean old_upper_arrow_visible;
	gint step;

	if (type == BTK_SCROLL_PAGE_UP)
	  step = - page_size;
	else
	  step = page_size;

	if (menu_shell->active_menu_item)
	  {
	    gint child_height;
	    
	    compute_child_offset (menu, menu_shell->active_menu_item,
				  &child_offset, &child_height, NULL);
	    child_offset += child_height / 2;
	  }

	menu_shell->ignore_enter = TRUE;
	old_upper_arrow_visible = menu->upper_arrow_visible && !menu->tearoff_active;
	old_offset = menu->scroll_offset;

        new_offset = menu->scroll_offset + step;
        new_offset = CLAMP (new_offset, 0, end_position - page_size);

        btk_menu_scroll_to (menu, new_offset);
	
	if (menu_shell->active_menu_item)
	  {
	    BtkWidget *new_child;
	    gboolean new_upper_arrow_visible = menu->upper_arrow_visible && !menu->tearoff_active;
            BtkBorder arrow_border;

	    get_arrows_border (menu, &arrow_border);

	    if (menu->scroll_offset != old_offset)
	      step = menu->scroll_offset - old_offset;

	    step -= (new_upper_arrow_visible - old_upper_arrow_visible) * arrow_border.top;

	    new_child = child_at (menu, child_offset + step);
	    if (new_child)
	      btk_menu_shell_select_item (menu_shell, new_child);
	  }
      }
      break;
    case BTK_SCROLL_START:
      /* Ignore the enter event we might get if the pointer is on the menu
       */
      menu_shell->ignore_enter = TRUE;
      btk_menu_scroll_to (menu, 0);
      btk_menu_shell_select_first (menu_shell, TRUE);
      break;
    case BTK_SCROLL_END:
      /* Ignore the enter event we might get if the pointer is on the menu
       */
      menu_shell->ignore_enter = TRUE;
      btk_menu_scroll_to (menu, end_position - page_size);
      _btk_menu_shell_select_last (menu_shell, TRUE);
      break;
    default:
      break;
    }
}


/**
 * btk_menu_set_monitor:
 * @menu: a #BtkMenu
 * @monitor_num: the number of the monitor on which the menu should
 *    be popped up
 * 
 * Informs BTK+ on which monitor a menu should be popped up. 
 * See bdk_screen_get_monitor_geometry().
 *
 * This function should be called from a #BtkMenuPositionFunc if the
 * menu should not appear on the same monitor as the pointer. This 
 * information can't be reliably inferred from the coordinates returned
 * by a #BtkMenuPositionFunc, since, for very long menus, these coordinates 
 * may extend beyond the monitor boundaries or even the screen boundaries. 
 *
 * Since: 2.4
 **/
void
btk_menu_set_monitor (BtkMenu *menu,
		      gint     monitor_num)
{
  BtkMenuPrivate *priv;
  g_return_if_fail (BTK_IS_MENU (menu));

  priv = btk_menu_get_private (menu);
  
  priv->monitor_num = monitor_num;
}

/**
 * btk_menu_get_monitor:
 * @menu: a #BtkMenu
 *
 * Retrieves the number of the monitor on which to show the menu.
 *
 * Returns: the number of the monitor on which the menu should
 *    be popped up or -1, if no monitor has been set
 *
 * Since: 2.14
 **/
gint
btk_menu_get_monitor (BtkMenu *menu)
{
  BtkMenuPrivate *priv;
  g_return_val_if_fail (BTK_IS_MENU (menu), -1);

  priv = btk_menu_get_private (menu);
  
  return priv->monitor_num;
}

/**
 * btk_menu_get_for_attach_widget:
 * @widget: a #BtkWidget
 *
 * Returns a list of the menus which are attached to this widget.
 * This list is owned by BTK+ and must not be modified.
 *
 * Return value: (element-type BtkWidget) (transfer none): the list of menus attached to his widget.
 *
 * Since: 2.6
 **/
GList*
btk_menu_get_for_attach_widget (BtkWidget *widget)
{
  GList *list;
  
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);
  
  list = g_object_get_data (B_OBJECT (widget), ATTACHED_MENUS);
  return list;
}

static void
btk_menu_grab_notify (BtkWidget *widget,
		      gboolean   was_grabbed)
{
  BtkWidget *toplevel;
  BtkWindowGroup *group;
  BtkWidget *grab;

  toplevel = btk_widget_get_toplevel (widget);
  group = btk_window_get_group (BTK_WINDOW (toplevel));
  grab = btk_window_group_get_current_grab (group);

  if (!was_grabbed)
    {
      if (BTK_MENU_SHELL (widget)->active && !BTK_IS_MENU_SHELL (grab))
        btk_menu_shell_cancel (BTK_MENU_SHELL (widget));
    }
}

/**
 * btk_menu_set_reserve_toggle_size:
 * @menu: a #BtkMenu
 * @reserve_toggle_size: whether to reserve size for toggles
 *
 * Sets whether the menu should reserve space for drawing toggles 
 * or icons, regardless of their actual presence.
 *
 * Since: 2.18
 */
void
btk_menu_set_reserve_toggle_size (BtkMenu  *menu,
                                  gboolean  reserve_toggle_size)
{
  BtkMenuPrivate *priv = btk_menu_get_private (menu);
  gboolean no_toggle_size;
  
  no_toggle_size = !reserve_toggle_size;

  if (priv->no_toggle_size != no_toggle_size)
    {
      priv->no_toggle_size = no_toggle_size;

      g_object_notify (B_OBJECT (menu), "reserve-toggle-size");
    }
}

/**
 * btk_menu_get_reserve_toggle_size:
 * @menu: a #BtkMenu
 *
 * Returns whether the menu reserves space for toggles and
 * icons, regardless of their actual presence.
 *
 * Returns: Whether the menu reserves toggle space
 *
 * Since: 2.18
 */
gboolean
btk_menu_get_reserve_toggle_size (BtkMenu *menu)
{
  BtkMenuPrivate *priv = btk_menu_get_private (menu);

  return !priv->no_toggle_size;
}

#define __BTK_MENU_C__
#include "btkaliasdef.c"
