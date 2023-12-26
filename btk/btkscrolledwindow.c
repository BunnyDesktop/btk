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
#include <math.h>
#include <bdk/bdkkeysyms.h>
#include "btkbindings.h"
#include "btkmarshalers.h"
#include "btkscrolledwindow.h"
#include "btkwindow.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"


/* scrolled window policy and size requisition handling:
 *
 * btk size requisition works as follows:
 *   a widget upon size-request reports the width and height that it finds
 *   to be best suited to display its contents, including children.
 *   the width and/or height reported from a widget upon size requisition
 *   may be overidden by the user by specifying a width and/or height
 *   other than 0 through btk_widget_set_size_request().
 *
 * a scrolled window needs (for implementing all three policy types) to
 * request its width and height based on two different rationales.
 * 1)   the user wants the scrolled window to just fit into the space
 *      that it gets allocated for a specifc dimension.
 * 1.1) this does not apply if the user specified a concrete value
 *      value for that specific dimension by either specifying usize for the
 *      scrolled window or for its child.
 * 2)   the user wants the scrolled window to take as much space up as
 *      is desired by the child for a specifc dimension (i.e. POLICY_NEVER).
 *
 * also, kinda obvious:
 * 3)   a user would certainly not have choosen a scrolled window as a container
 *      for the child, if the resulting allocation takes up more space than the
 *      child would have allocated without the scrolled window.
 *
 * conclusions:
 * A) from 1) follows: the scrolled window shouldn't request more space for a
 *    specifc dimension than is required at minimum.
 * B) from 1.1) follows: the requisition may be overidden by usize of the scrolled
 *    window (done automatically) or by usize of the child (needs to be checked).
 * C) from 2) follows: for POLICY_NEVER, the scrolled window simply reports the
 *    child's dimension.
 * D) from 3) follows: the scrolled window child's minimum width and minimum height
 *    under A) at least correspond to the space taken up by its scrollbars.
 */

#define DEFAULT_SCROLLBAR_SPACING  3

typedef struct {
	gboolean window_placement_set;
	BtkCornerType real_window_placement;
} BtkScrolledWindowPrivate;

#define BTK_SCROLLED_WINDOW_GET_PRIVATE(obj) (B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_SCROLLED_WINDOW, BtkScrolledWindowPrivate))

enum {
  PROP_0,
  PROP_HADJUSTMENT,
  PROP_VADJUSTMENT,
  PROP_HSCROLLBAR_POLICY,
  PROP_VSCROLLBAR_POLICY,
  PROP_WINDOW_PLACEMENT,
  PROP_WINDOW_PLACEMENT_SET,
  PROP_SHADOW_TYPE
};

/* Signals */
enum
{
  SCROLL_CHILD,
  MOVE_FOCUS_OUT,
  LAST_SIGNAL
};

static void     btk_scrolled_window_destroy            (BtkObject         *object);
static void     btk_scrolled_window_set_property       (BObject           *object,
                                                        guint              prop_id,
                                                        const BValue      *value,
                                                        BParamSpec        *pspec);
static void     btk_scrolled_window_get_property       (BObject           *object,
                                                        guint              prop_id,
                                                        BValue            *value,
                                                        BParamSpec        *pspec);

static void     btk_scrolled_window_screen_changed     (BtkWidget         *widget,
                                                        BdkScreen         *previous_screen);
static gboolean btk_scrolled_window_expose             (BtkWidget         *widget,
                                                        BdkEventExpose    *event);
static void     btk_scrolled_window_size_request       (BtkWidget         *widget,
                                                        BtkRequisition    *requisition);
static void     btk_scrolled_window_size_allocate      (BtkWidget         *widget,
                                                        BtkAllocation     *allocation);
static gboolean btk_scrolled_window_scroll_event       (BtkWidget         *widget,
                                                        BdkEventScroll    *event);
static gboolean btk_scrolled_window_focus              (BtkWidget         *widget,
                                                        BtkDirectionType   direction);
static void     btk_scrolled_window_add                (BtkContainer      *container,
                                                        BtkWidget         *widget);
static void     btk_scrolled_window_remove             (BtkContainer      *container,
                                                        BtkWidget         *widget);
static void     btk_scrolled_window_forall             (BtkContainer      *container,
                                                        gboolean           include_internals,
                                                        BtkCallback        callback,
                                                        gpointer           callback_data);
static gboolean btk_scrolled_window_scroll_child       (BtkScrolledWindow *scrolled_window,
                                                        BtkScrollType      scroll,
                                                        gboolean           horizontal);
static void     btk_scrolled_window_move_focus_out     (BtkScrolledWindow *scrolled_window,
                                                        BtkDirectionType   direction_type);

static void     btk_scrolled_window_relative_allocation(BtkWidget         *widget,
                                                        BtkAllocation     *allocation);
static void     btk_scrolled_window_adjustment_changed (BtkAdjustment     *adjustment,
                                                        gpointer           data);

static void  btk_scrolled_window_update_real_placement (BtkScrolledWindow *scrolled_window);

static guint signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE (BtkScrolledWindow, btk_scrolled_window, BTK_TYPE_BIN)

static void
add_scroll_binding (BtkBindingSet  *binding_set,
		    guint           keyval,
		    BdkModifierType mask,
		    BtkScrollType   scroll,
		    gboolean        horizontal)
{
  guint keypad_keyval = keyval - BDK_Left + BDK_KP_Left;
  
  btk_binding_entry_add_signal (binding_set, keyval, mask,
                                "scroll-child", 2,
                                BTK_TYPE_SCROLL_TYPE, scroll,
				B_TYPE_BOOLEAN, horizontal);
  btk_binding_entry_add_signal (binding_set, keypad_keyval, mask,
                                "scroll-child", 2,
                                BTK_TYPE_SCROLL_TYPE, scroll,
				B_TYPE_BOOLEAN, horizontal);
}

static void
add_tab_bindings (BtkBindingSet    *binding_set,
		  BdkModifierType   modifiers,
		  BtkDirectionType  direction)
{
  btk_binding_entry_add_signal (binding_set, BDK_Tab, modifiers,
                                "move-focus-out", 1,
                                BTK_TYPE_DIRECTION_TYPE, direction);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Tab, modifiers,
                                "move-focus-out", 1,
                                BTK_TYPE_DIRECTION_TYPE, direction);
}

static void
btk_scrolled_window_class_init (BtkScrolledWindowClass *class)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (class);
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;
  BtkContainerClass *container_class;
  BtkBindingSet *binding_set;

  object_class = (BtkObjectClass*) class;
  widget_class = (BtkWidgetClass*) class;
  container_class = (BtkContainerClass*) class;

  bobject_class->set_property = btk_scrolled_window_set_property;
  bobject_class->get_property = btk_scrolled_window_get_property;

  object_class->destroy = btk_scrolled_window_destroy;

  widget_class->screen_changed = btk_scrolled_window_screen_changed;
  widget_class->expose_event = btk_scrolled_window_expose;
  widget_class->size_request = btk_scrolled_window_size_request;
  widget_class->size_allocate = btk_scrolled_window_size_allocate;
  widget_class->scroll_event = btk_scrolled_window_scroll_event;
  widget_class->focus = btk_scrolled_window_focus;

  container_class->add = btk_scrolled_window_add;
  container_class->remove = btk_scrolled_window_remove;
  container_class->forall = btk_scrolled_window_forall;

  class->scrollbar_spacing = -1;

  class->scroll_child = btk_scrolled_window_scroll_child;
  class->move_focus_out = btk_scrolled_window_move_focus_out;
  
  g_object_class_install_property (bobject_class,
				   PROP_HADJUSTMENT,
				   g_param_spec_object ("hadjustment",
							P_("Horizontal Adjustment"),
							P_("The BtkAdjustment for the horizontal position"),
							BTK_TYPE_ADJUSTMENT,
							BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (bobject_class,
				   PROP_VADJUSTMENT,
				   g_param_spec_object ("vadjustment",
							P_("Vertical Adjustment"),
							P_("The BtkAdjustment for the vertical position"),
							BTK_TYPE_ADJUSTMENT,
							BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (bobject_class,
                                   PROP_HSCROLLBAR_POLICY,
                                   g_param_spec_enum ("hscrollbar-policy",
                                                      P_("Horizontal Scrollbar Policy"),
                                                      P_("When the horizontal scrollbar is displayed"),
						      BTK_TYPE_POLICY_TYPE,
						      BTK_POLICY_ALWAYS,
                                                      BTK_PARAM_READABLE | BTK_PARAM_WRITABLE));
  g_object_class_install_property (bobject_class,
                                   PROP_VSCROLLBAR_POLICY,
                                   g_param_spec_enum ("vscrollbar-policy",
                                                      P_("Vertical Scrollbar Policy"),
                                                      P_("When the vertical scrollbar is displayed"),
						      BTK_TYPE_POLICY_TYPE,
						      BTK_POLICY_ALWAYS,
                                                      BTK_PARAM_READABLE | BTK_PARAM_WRITABLE));

  g_object_class_install_property (bobject_class,
                                   PROP_WINDOW_PLACEMENT,
                                   g_param_spec_enum ("window-placement",
                                                      P_("Window Placement"),
                                                      P_("Where the contents are located with respect to the scrollbars. This property only takes effect if \"window-placement-set\" is TRUE."),
						      BTK_TYPE_CORNER_TYPE,
						      BTK_CORNER_TOP_LEFT,
                                                      BTK_PARAM_READABLE | BTK_PARAM_WRITABLE));
  
  /**
   * BtkScrolledWindow:window-placement-set:
   *
   * Whether "window-placement" should be used to determine the location 
   * of the contents with respect to the scrollbars. Otherwise, the 
   * "btk-scrolled-window-placement" setting is used.
   *
   * Since: 2.10
   */
  g_object_class_install_property (bobject_class,
                                   PROP_WINDOW_PLACEMENT_SET,
                                   g_param_spec_boolean ("window-placement-set",
					   		 P_("Window Placement Set"),
							 P_("Whether \"window-placement\" should be used to determine the location of the contents with respect to the scrollbars."),
							 FALSE,
							 BTK_PARAM_READABLE | BTK_PARAM_WRITABLE));
  g_object_class_install_property (bobject_class,
                                   PROP_SHADOW_TYPE,
                                   g_param_spec_enum ("shadow-type",
                                                      P_("Shadow Type"),
                                                      P_("Style of bevel around the contents"),
						      BTK_TYPE_SHADOW_TYPE,
						      BTK_SHADOW_NONE,
                                                      BTK_PARAM_READABLE | BTK_PARAM_WRITABLE));

  /**
   * BtkScrolledWindow:scrollbars-within-bevel:
   *
   * Whether to place scrollbars within the scrolled window's bevel.
   *
   * Since: 2.12
   */
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean ("scrollbars-within-bevel",
							         P_("Scrollbars within bevel"),
							         P_("Place scrollbars within the scrolled window's bevel"),
							         FALSE,
							         BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("scrollbar-spacing",
							     P_("Scrollbar spacing"),
							     P_("Number of pixels between the scrollbars and the scrolled window"),
							     0,
							     G_MAXINT,
							     DEFAULT_SCROLLBAR_SPACING,
							     BTK_PARAM_READABLE));

  /**
   * BtkScrolledWindow::scroll-child:
   * @scrolled_window: a #BtkScrolledWindow
   * @scroll: a #BtkScrollType describing how much to scroll
   * @horizontal: whether the keybinding scrolls the child
   *   horizontally or not
   *
   * The ::scroll-child signal is a
   * <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted when a keybinding that scrolls is pressed.
   * The horizontal or vertical adjustment is updated which triggers a
   * signal that the scrolled windows child may listen to and scroll itself.
   */
  signals[SCROLL_CHILD] =
    g_signal_new (I_("scroll-child"),
                  B_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BtkScrolledWindowClass, scroll_child),
                  NULL, NULL,
                  _btk_marshal_BOOLEAN__ENUM_BOOLEAN,
                  B_TYPE_BOOLEAN, 2,
                  BTK_TYPE_SCROLL_TYPE,
		  B_TYPE_BOOLEAN);
  signals[MOVE_FOCUS_OUT] =
    g_signal_new (I_("move-focus-out"),
                  B_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BtkScrolledWindowClass, move_focus_out),
                  NULL, NULL,
                  _btk_marshal_VOID__ENUM,
                  B_TYPE_NONE, 1,
                  BTK_TYPE_DIRECTION_TYPE);
  
  binding_set = btk_binding_set_by_class (class);

  add_scroll_binding (binding_set, BDK_Left,  BDK_CONTROL_MASK, BTK_SCROLL_STEP_BACKWARD, TRUE);
  add_scroll_binding (binding_set, BDK_Right, BDK_CONTROL_MASK, BTK_SCROLL_STEP_FORWARD,  TRUE);
  add_scroll_binding (binding_set, BDK_Up,    BDK_CONTROL_MASK, BTK_SCROLL_STEP_BACKWARD, FALSE);
  add_scroll_binding (binding_set, BDK_Down,  BDK_CONTROL_MASK, BTK_SCROLL_STEP_FORWARD,  FALSE);

  add_scroll_binding (binding_set, BDK_Page_Up,   BDK_CONTROL_MASK, BTK_SCROLL_PAGE_BACKWARD, TRUE);
  add_scroll_binding (binding_set, BDK_Page_Down, BDK_CONTROL_MASK, BTK_SCROLL_PAGE_FORWARD,  TRUE);
  add_scroll_binding (binding_set, BDK_Page_Up,   0,                BTK_SCROLL_PAGE_BACKWARD, FALSE);
  add_scroll_binding (binding_set, BDK_Page_Down, 0,                BTK_SCROLL_PAGE_FORWARD,  FALSE);

  add_scroll_binding (binding_set, BDK_Home, BDK_CONTROL_MASK, BTK_SCROLL_START, TRUE);
  add_scroll_binding (binding_set, BDK_End,  BDK_CONTROL_MASK, BTK_SCROLL_END,   TRUE);
  add_scroll_binding (binding_set, BDK_Home, 0,                BTK_SCROLL_START, FALSE);
  add_scroll_binding (binding_set, BDK_End,  0,                BTK_SCROLL_END,   FALSE);

  add_tab_bindings (binding_set, BDK_CONTROL_MASK, BTK_DIR_TAB_FORWARD);
  add_tab_bindings (binding_set, BDK_CONTROL_MASK | BDK_SHIFT_MASK, BTK_DIR_TAB_BACKWARD);

  g_type_class_add_private (class, sizeof (BtkScrolledWindowPrivate));
}

static void
btk_scrolled_window_init (BtkScrolledWindow *scrolled_window)
{
  btk_widget_set_has_window (BTK_WIDGET (scrolled_window), FALSE);
  btk_widget_set_can_focus (BTK_WIDGET (scrolled_window), TRUE);

  scrolled_window->hscrollbar = NULL;
  scrolled_window->vscrollbar = NULL;
  scrolled_window->hscrollbar_policy = BTK_POLICY_ALWAYS;
  scrolled_window->vscrollbar_policy = BTK_POLICY_ALWAYS;
  scrolled_window->hscrollbar_visible = FALSE;
  scrolled_window->vscrollbar_visible = FALSE;
  scrolled_window->focus_out = FALSE;
  scrolled_window->window_placement = BTK_CORNER_TOP_LEFT;
  btk_scrolled_window_update_real_placement (scrolled_window);
}

/**
 * btk_scrolled_window_new:
 * @hadjustment: (allow-none): horizontal adjustment
 * @vadjustment: (allow-none): vertical adjustment
 *
 * Creates a new scrolled window.
 *
 * The two arguments are the scrolled window's adjustments; these will be
 * shared with the scrollbars and the child widget to keep the bars in sync 
 * with the child. Usually you want to pass %NULL for the adjustments, which 
 * will cause the scrolled window to create them for you.
 *
 * Returns: a new scrolled window
 */
BtkWidget*
btk_scrolled_window_new (BtkAdjustment *hadjustment,
			 BtkAdjustment *vadjustment)
{
  BtkWidget *scrolled_window;

  if (hadjustment)
    g_return_val_if_fail (BTK_IS_ADJUSTMENT (hadjustment), NULL);

  if (vadjustment)
    g_return_val_if_fail (BTK_IS_ADJUSTMENT (vadjustment), NULL);

  scrolled_window = g_object_new (BTK_TYPE_SCROLLED_WINDOW,
				    "hadjustment", hadjustment,
				    "vadjustment", vadjustment,
				    NULL);

  return scrolled_window;
}

/**
 * btk_scrolled_window_set_hadjustment:
 * @scrolled_window: a #BtkScrolledWindow
 * @hadjustment: horizontal scroll adjustment
 *
 * Sets the #BtkAdjustment for the horizontal scrollbar.
 */
void
btk_scrolled_window_set_hadjustment (BtkScrolledWindow *scrolled_window,
				     BtkAdjustment     *hadjustment)
{
  BtkBin *bin;

  g_return_if_fail (BTK_IS_SCROLLED_WINDOW (scrolled_window));
  if (hadjustment)
    g_return_if_fail (BTK_IS_ADJUSTMENT (hadjustment));
  else
    hadjustment = (BtkAdjustment*) g_object_new (BTK_TYPE_ADJUSTMENT, NULL);

  bin = BTK_BIN (scrolled_window);

  if (!scrolled_window->hscrollbar)
    {
      btk_widget_push_composite_child ();
      scrolled_window->hscrollbar = btk_hscrollbar_new (hadjustment);
      btk_widget_set_composite_name (scrolled_window->hscrollbar, "hscrollbar");
      btk_widget_pop_composite_child ();

      btk_widget_set_parent (scrolled_window->hscrollbar, BTK_WIDGET (scrolled_window));
      g_object_ref (scrolled_window->hscrollbar);
      btk_widget_show (scrolled_window->hscrollbar);
    }
  else
    {
      BtkAdjustment *old_adjustment;
      
      old_adjustment = btk_range_get_adjustment (BTK_RANGE (scrolled_window->hscrollbar));
      if (old_adjustment == hadjustment)
	return;

      g_signal_handlers_disconnect_by_func (old_adjustment,
					    btk_scrolled_window_adjustment_changed,
					    scrolled_window);
      btk_range_set_adjustment (BTK_RANGE (scrolled_window->hscrollbar),
				hadjustment);
    }
  hadjustment = btk_range_get_adjustment (BTK_RANGE (scrolled_window->hscrollbar));
  g_signal_connect (hadjustment,
		    "changed",
		    G_CALLBACK (btk_scrolled_window_adjustment_changed),
		    scrolled_window);
  btk_scrolled_window_adjustment_changed (hadjustment, scrolled_window);
  
  if (bin->child)
    btk_widget_set_scroll_adjustments (bin->child,
				       btk_range_get_adjustment (BTK_RANGE (scrolled_window->hscrollbar)),
				       btk_range_get_adjustment (BTK_RANGE (scrolled_window->vscrollbar)));

  g_object_notify (B_OBJECT (scrolled_window), "hadjustment");
}

/**
 * btk_scrolled_window_set_vadjustment:
 * @scrolled_window: a #BtkScrolledWindow
 * @vadjustment: vertical scroll adjustment
 *
 * Sets the #BtkAdjustment for the vertical scrollbar.
 */
void
btk_scrolled_window_set_vadjustment (BtkScrolledWindow *scrolled_window,
				     BtkAdjustment     *vadjustment)
{
  BtkBin *bin;

  g_return_if_fail (BTK_IS_SCROLLED_WINDOW (scrolled_window));
  if (vadjustment)
    g_return_if_fail (BTK_IS_ADJUSTMENT (vadjustment));
  else
    vadjustment = (BtkAdjustment*) g_object_new (BTK_TYPE_ADJUSTMENT, NULL);

  bin = BTK_BIN (scrolled_window);

  if (!scrolled_window->vscrollbar)
    {
      btk_widget_push_composite_child ();
      scrolled_window->vscrollbar = btk_vscrollbar_new (vadjustment);
      btk_widget_set_composite_name (scrolled_window->vscrollbar, "vscrollbar");
      btk_widget_pop_composite_child ();

      btk_widget_set_parent (scrolled_window->vscrollbar, BTK_WIDGET (scrolled_window));
      g_object_ref (scrolled_window->vscrollbar);
      btk_widget_show (scrolled_window->vscrollbar);
    }
  else
    {
      BtkAdjustment *old_adjustment;
      
      old_adjustment = btk_range_get_adjustment (BTK_RANGE (scrolled_window->vscrollbar));
      if (old_adjustment == vadjustment)
	return;

      g_signal_handlers_disconnect_by_func (old_adjustment,
					    btk_scrolled_window_adjustment_changed,
					    scrolled_window);
      btk_range_set_adjustment (BTK_RANGE (scrolled_window->vscrollbar),
				vadjustment);
    }
  vadjustment = btk_range_get_adjustment (BTK_RANGE (scrolled_window->vscrollbar));
  g_signal_connect (vadjustment,
		    "changed",
		    G_CALLBACK (btk_scrolled_window_adjustment_changed),
		    scrolled_window);
  btk_scrolled_window_adjustment_changed (vadjustment, scrolled_window);

  if (bin->child)
    btk_widget_set_scroll_adjustments (bin->child,
				       btk_range_get_adjustment (BTK_RANGE (scrolled_window->hscrollbar)),
				       btk_range_get_adjustment (BTK_RANGE (scrolled_window->vscrollbar)));

  g_object_notify (B_OBJECT (scrolled_window), "vadjustment");
}

/**
 * btk_scrolled_window_get_hadjustment:
 * @scrolled_window: a #BtkScrolledWindow
 *
 * Returns the horizontal scrollbar's adjustment, used to connect the
 * horizontal scrollbar to the child widget's horizontal scroll
 * functionality.
 *
 * Returns: (transfer none): the horizontal #BtkAdjustment
 */
BtkAdjustment*
btk_scrolled_window_get_hadjustment (BtkScrolledWindow *scrolled_window)
{
  g_return_val_if_fail (BTK_IS_SCROLLED_WINDOW (scrolled_window), NULL);

  return (scrolled_window->hscrollbar ?
	  btk_range_get_adjustment (BTK_RANGE (scrolled_window->hscrollbar)) :
	  NULL);
}

/**
 * btk_scrolled_window_get_vadjustment:
 * @scrolled_window: a #BtkScrolledWindow
 * 
 * Returns the vertical scrollbar's adjustment, used to connect the
 * vertical scrollbar to the child widget's vertical scroll functionality.
 * 
 * Returns: (transfer none): the vertical #BtkAdjustment
 */
BtkAdjustment*
btk_scrolled_window_get_vadjustment (BtkScrolledWindow *scrolled_window)
{
  g_return_val_if_fail (BTK_IS_SCROLLED_WINDOW (scrolled_window), NULL);

  return (scrolled_window->vscrollbar ?
	  btk_range_get_adjustment (BTK_RANGE (scrolled_window->vscrollbar)) :
	  NULL);
}

/**
 * btk_scrolled_window_get_hscrollbar:
 * @scrolled_window: a #BtkScrolledWindow
 *
 * Returns the horizontal scrollbar of @scrolled_window.
 *
 * Returns: (transfer none): the horizontal scrollbar of the scrolled window,
 *     or %NULL if it does not have one.
 *
 * Since: 2.8
 */
BtkWidget*
btk_scrolled_window_get_hscrollbar (BtkScrolledWindow *scrolled_window)
{
  g_return_val_if_fail (BTK_IS_SCROLLED_WINDOW (scrolled_window), NULL);
  
  return scrolled_window->hscrollbar;
}

/**
 * btk_scrolled_window_get_vscrollbar:
 * @scrolled_window: a #BtkScrolledWindow
 * 
 * Returns the vertical scrollbar of @scrolled_window.
 *
 * Returns: (transfer none): the vertical scrollbar of the scrolled window,
 *     or %NULL if it does not have one.
 *
 * Since: 2.8
 */
BtkWidget*
btk_scrolled_window_get_vscrollbar (BtkScrolledWindow *scrolled_window)
{
  g_return_val_if_fail (BTK_IS_SCROLLED_WINDOW (scrolled_window), NULL);

  return scrolled_window->vscrollbar;
}

/**
 * btk_scrolled_window_set_policy:
 * @scrolled_window: a #BtkScrolledWindow
 * @hscrollbar_policy: policy for horizontal bar
 * @vscrollbar_policy: policy for vertical bar
 * 
 * Sets the scrollbar policy for the horizontal and vertical scrollbars.
 *
 * The policy determines when the scrollbar should appear; it is a value
 * from the #BtkPolicyType enumeration. If %BTK_POLICY_ALWAYS, the
 * scrollbar is always present; if %BTK_POLICY_NEVER, the scrollbar is
 * never present; if %BTK_POLICY_AUTOMATIC, the scrollbar is present only
 * if needed (that is, if the slider part of the bar would be smaller
 * than the trough - the display is larger than the page size).
 */
void
btk_scrolled_window_set_policy (BtkScrolledWindow *scrolled_window,
				BtkPolicyType      hscrollbar_policy,
				BtkPolicyType      vscrollbar_policy)
{
  BObject *object = B_OBJECT (scrolled_window);
  
  g_return_if_fail (BTK_IS_SCROLLED_WINDOW (scrolled_window));

  if ((scrolled_window->hscrollbar_policy != hscrollbar_policy) ||
      (scrolled_window->vscrollbar_policy != vscrollbar_policy))
    {
      scrolled_window->hscrollbar_policy = hscrollbar_policy;
      scrolled_window->vscrollbar_policy = vscrollbar_policy;

      btk_widget_queue_resize (BTK_WIDGET (scrolled_window));

      g_object_freeze_notify (object);
      g_object_notify (object, "hscrollbar-policy");
      g_object_notify (object, "vscrollbar-policy");
      g_object_thaw_notify (object);
    }
}

/**
 * btk_scrolled_window_get_policy:
 * @scrolled_window: a #BtkScrolledWindow
 * @hscrollbar_policy: (out) (allow-none): location to store the policy 
 *     for the horizontal scrollbar, or %NULL.
 * @vscrollbar_policy: (out) (allow-none): location to store the policy
 *     for the vertical scrollbar, or %NULL.
 * 
 * Retrieves the current policy values for the horizontal and vertical
 * scrollbars. See btk_scrolled_window_set_policy().
 */
void
btk_scrolled_window_get_policy (BtkScrolledWindow *scrolled_window,
				BtkPolicyType     *hscrollbar_policy,
				BtkPolicyType     *vscrollbar_policy)
{
  g_return_if_fail (BTK_IS_SCROLLED_WINDOW (scrolled_window));

  if (hscrollbar_policy)
    *hscrollbar_policy = scrolled_window->hscrollbar_policy;
  if (vscrollbar_policy)
    *vscrollbar_policy = scrolled_window->vscrollbar_policy;
}

static void
btk_scrolled_window_update_real_placement (BtkScrolledWindow *scrolled_window)
{
  BtkScrolledWindowPrivate *priv = BTK_SCROLLED_WINDOW_GET_PRIVATE (scrolled_window);
  BtkSettings *settings;

  settings = btk_widget_get_settings (BTK_WIDGET (scrolled_window));

  if (priv->window_placement_set || settings == NULL)
    priv->real_window_placement = scrolled_window->window_placement;
  else
    g_object_get (settings,
		  "btk-scrolled-window-placement",
		  &priv->real_window_placement,
		  NULL);
}

static void
btk_scrolled_window_set_placement_internal (BtkScrolledWindow *scrolled_window,
					    BtkCornerType      window_placement)
{
  if (scrolled_window->window_placement != window_placement)
    {
      scrolled_window->window_placement = window_placement;

      btk_scrolled_window_update_real_placement (scrolled_window);
      btk_widget_queue_resize (BTK_WIDGET (scrolled_window));
      
      g_object_notify (B_OBJECT (scrolled_window), "window-placement");
    }
}

static void
btk_scrolled_window_set_placement_set (BtkScrolledWindow *scrolled_window,
				       gboolean           placement_set,
				       gboolean           emit_resize)
{
  BtkScrolledWindowPrivate *priv = BTK_SCROLLED_WINDOW_GET_PRIVATE (scrolled_window);

  if (priv->window_placement_set != placement_set)
    {
      priv->window_placement_set = placement_set;

      btk_scrolled_window_update_real_placement (scrolled_window);
      if (emit_resize)
        btk_widget_queue_resize (BTK_WIDGET (scrolled_window));

      g_object_notify (B_OBJECT (scrolled_window), "window-placement-set");
    }
}

/**
 * btk_scrolled_window_set_placement:
 * @scrolled_window: a #BtkScrolledWindow
 * @window_placement: position of the child window
 *
 * Sets the placement of the contents with respect to the scrollbars
 * for the scrolled window.
 * 
 * The default is %BTK_CORNER_TOP_LEFT, meaning the child is
 * in the top left, with the scrollbars underneath and to the right.
 * Other values in #BtkCornerType are %BTK_CORNER_TOP_RIGHT,
 * %BTK_CORNER_BOTTOM_LEFT, and %BTK_CORNER_BOTTOM_RIGHT.
 *
 * See also btk_scrolled_window_get_placement() and
 * btk_scrolled_window_unset_placement().
 */
void
btk_scrolled_window_set_placement (BtkScrolledWindow *scrolled_window,
				   BtkCornerType      window_placement)
{
  g_return_if_fail (BTK_IS_SCROLLED_WINDOW (scrolled_window));

  btk_scrolled_window_set_placement_set (scrolled_window, TRUE, FALSE);
  btk_scrolled_window_set_placement_internal (scrolled_window, window_placement);
}

/**
 * btk_scrolled_window_get_placement:
 * @scrolled_window: a #BtkScrolledWindow
 *
 * Gets the placement of the contents with respect to the scrollbars
 * for the scrolled window. See btk_scrolled_window_set_placement().
 *
 * Return value: the current placement value.
 *
 * See also btk_scrolled_window_set_placement() and
 * btk_scrolled_window_unset_placement().
 **/
BtkCornerType
btk_scrolled_window_get_placement (BtkScrolledWindow *scrolled_window)
{
  g_return_val_if_fail (BTK_IS_SCROLLED_WINDOW (scrolled_window), BTK_CORNER_TOP_LEFT);

  return scrolled_window->window_placement;
}

/**
 * btk_scrolled_window_unset_placement:
 * @scrolled_window: a #BtkScrolledWindow
 *
 * Unsets the placement of the contents with respect to the scrollbars
 * for the scrolled window. If no window placement is set for a scrolled
 * window, it obeys the "btk-scrolled-window-placement" XSETTING.
 *
 * See also btk_scrolled_window_set_placement() and
 * btk_scrolled_window_get_placement().
 *
 * Since: 2.10
 **/
void
btk_scrolled_window_unset_placement (BtkScrolledWindow *scrolled_window)
{
  BtkScrolledWindowPrivate *priv = BTK_SCROLLED_WINDOW_GET_PRIVATE (scrolled_window);

  g_return_if_fail (BTK_IS_SCROLLED_WINDOW (scrolled_window));

  if (priv->window_placement_set)
    {
      priv->window_placement_set = FALSE;

      btk_widget_queue_resize (BTK_WIDGET (scrolled_window));

      g_object_notify (B_OBJECT (scrolled_window), "window-placement-set");
    }
}

/**
 * btk_scrolled_window_set_shadow_type:
 * @scrolled_window: a #BtkScrolledWindow
 * @type: kind of shadow to draw around scrolled window contents
 *
 * Changes the type of shadow drawn around the contents of
 * @scrolled_window.
 * 
 **/
void
btk_scrolled_window_set_shadow_type (BtkScrolledWindow *scrolled_window,
				     BtkShadowType      type)
{
  g_return_if_fail (BTK_IS_SCROLLED_WINDOW (scrolled_window));
  g_return_if_fail (type >= BTK_SHADOW_NONE && type <= BTK_SHADOW_ETCHED_OUT);
  
  if (scrolled_window->shadow_type != type)
    {
      scrolled_window->shadow_type = type;

      if (btk_widget_is_drawable (BTK_WIDGET (scrolled_window)))
	btk_widget_queue_draw (BTK_WIDGET (scrolled_window));

      btk_widget_queue_resize (BTK_WIDGET (scrolled_window));

      g_object_notify (B_OBJECT (scrolled_window), "shadow-type");
    }
}

/**
 * btk_scrolled_window_get_shadow_type:
 * @scrolled_window: a #BtkScrolledWindow
 *
 * Gets the shadow type of the scrolled window. See 
 * btk_scrolled_window_set_shadow_type().
 *
 * Return value: the current shadow type
 **/
BtkShadowType
btk_scrolled_window_get_shadow_type (BtkScrolledWindow *scrolled_window)
{
  g_return_val_if_fail (BTK_IS_SCROLLED_WINDOW (scrolled_window), BTK_SHADOW_NONE);

  return scrolled_window->shadow_type;
}

static void
btk_scrolled_window_destroy (BtkObject *object)
{
  BtkScrolledWindow *scrolled_window = BTK_SCROLLED_WINDOW (object);

  if (scrolled_window->hscrollbar)
    {
      g_signal_handlers_disconnect_by_func (btk_range_get_adjustment (BTK_RANGE (scrolled_window->hscrollbar)),
					    btk_scrolled_window_adjustment_changed,
					    scrolled_window);
      btk_widget_unparent (scrolled_window->hscrollbar);
      btk_widget_destroy (scrolled_window->hscrollbar);
      g_object_unref (scrolled_window->hscrollbar);
      scrolled_window->hscrollbar = NULL;
    }
  if (scrolled_window->vscrollbar)
    {
      g_signal_handlers_disconnect_by_func (btk_range_get_adjustment (BTK_RANGE (scrolled_window->vscrollbar)),
					    btk_scrolled_window_adjustment_changed,
					    scrolled_window);
      btk_widget_unparent (scrolled_window->vscrollbar);
      btk_widget_destroy (scrolled_window->vscrollbar);
      g_object_unref (scrolled_window->vscrollbar);
      scrolled_window->vscrollbar = NULL;
    }

  BTK_OBJECT_CLASS (btk_scrolled_window_parent_class)->destroy (object);
}

static void
btk_scrolled_window_set_property (BObject      *object,
				  guint         prop_id,
				  const BValue *value,
				  BParamSpec   *pspec)
{
  BtkScrolledWindow *scrolled_window = BTK_SCROLLED_WINDOW (object);
  
  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      btk_scrolled_window_set_hadjustment (scrolled_window,
					   b_value_get_object (value));
      break;
    case PROP_VADJUSTMENT:
      btk_scrolled_window_set_vadjustment (scrolled_window,
					   b_value_get_object (value));
      break;
    case PROP_HSCROLLBAR_POLICY:
      btk_scrolled_window_set_policy (scrolled_window,
				      b_value_get_enum (value),
				      scrolled_window->vscrollbar_policy);
      break;
    case PROP_VSCROLLBAR_POLICY:
      btk_scrolled_window_set_policy (scrolled_window,
				      scrolled_window->hscrollbar_policy,
				      b_value_get_enum (value));
      break;
    case PROP_WINDOW_PLACEMENT:
      btk_scrolled_window_set_placement_internal (scrolled_window,
		      				  b_value_get_enum (value));
      break;
    case PROP_WINDOW_PLACEMENT_SET:
      btk_scrolled_window_set_placement_set (scrolled_window,
		      			     b_value_get_boolean (value),
					     TRUE);
      break;
    case PROP_SHADOW_TYPE:
      btk_scrolled_window_set_shadow_type (scrolled_window,
					   b_value_get_enum (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_scrolled_window_get_property (BObject    *object,
				  guint       prop_id,
				  BValue     *value,
				  BParamSpec *pspec)
{
  BtkScrolledWindow *scrolled_window = BTK_SCROLLED_WINDOW (object);
  BtkScrolledWindowPrivate *priv = BTK_SCROLLED_WINDOW_GET_PRIVATE (scrolled_window);
  
  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      b_value_set_object (value,
			  B_OBJECT (btk_scrolled_window_get_hadjustment (scrolled_window)));
      break;
    case PROP_VADJUSTMENT:
      b_value_set_object (value,
			  B_OBJECT (btk_scrolled_window_get_vadjustment (scrolled_window)));
      break;
    case PROP_HSCROLLBAR_POLICY:
      b_value_set_enum (value, scrolled_window->hscrollbar_policy);
      break;
    case PROP_VSCROLLBAR_POLICY:
      b_value_set_enum (value, scrolled_window->vscrollbar_policy);
      break;
    case PROP_WINDOW_PLACEMENT:
      b_value_set_enum (value, scrolled_window->window_placement);
      break;
    case PROP_WINDOW_PLACEMENT_SET:
      b_value_set_boolean (value, priv->window_placement_set);
      break;
    case PROP_SHADOW_TYPE:
      b_value_set_enum (value, scrolled_window->shadow_type);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
traverse_container (BtkWidget *widget,
		    gpointer   data)
{
  if (BTK_IS_SCROLLED_WINDOW (widget))
    {
      btk_scrolled_window_update_real_placement (BTK_SCROLLED_WINDOW (widget));
      btk_widget_queue_resize (widget);
    }
  else if (BTK_IS_CONTAINER (widget))
    btk_container_forall (BTK_CONTAINER (widget), traverse_container, NULL);
}

static void
btk_scrolled_window_settings_changed (BtkSettings *settings)
{
  GList *list, *l;

  list = btk_window_list_toplevels ();

  for (l = list; l; l = l->next)
    btk_container_forall (BTK_CONTAINER (l->data), 
			  traverse_container, NULL);

  g_list_free (list);
}

static void
btk_scrolled_window_screen_changed (BtkWidget *widget,
				    BdkScreen *previous_screen)
{
  BtkSettings *settings;
  guint window_placement_connection;

  btk_scrolled_window_update_real_placement (BTK_SCROLLED_WINDOW (widget));

  if (!btk_widget_has_screen (widget))
    return;

  settings = btk_widget_get_settings (widget);

  window_placement_connection = 
    GPOINTER_TO_UINT (g_object_get_data (B_OBJECT (settings), 
					 "btk-scrolled-window-connection"));
  
  if (window_placement_connection)
    return;

  window_placement_connection =
    g_signal_connect (settings, "notify::btk-scrolled-window-placement",
		      G_CALLBACK (btk_scrolled_window_settings_changed), NULL);
  g_object_set_data (B_OBJECT (settings), 
		     I_("btk-scrolled-window-connection"),
		     GUINT_TO_POINTER (window_placement_connection));
}

static void
btk_scrolled_window_paint (BtkWidget    *widget,
			   BdkRectangle *area)
{
  BtkScrolledWindow *scrolled_window = BTK_SCROLLED_WINDOW (widget);

  if (scrolled_window->shadow_type != BTK_SHADOW_NONE)
    {
      BtkAllocation relative_allocation;
      gboolean scrollbars_within_bevel;

      btk_widget_style_get (widget, "scrollbars-within-bevel", &scrollbars_within_bevel, NULL);
      
      if (!scrollbars_within_bevel)
        {
          btk_scrolled_window_relative_allocation (widget, &relative_allocation);

          relative_allocation.x -= widget->style->xthickness;
          relative_allocation.y -= widget->style->ythickness;
          relative_allocation.width += 2 * widget->style->xthickness;
          relative_allocation.height += 2 * widget->style->ythickness;
        }
      else
        {
          BtkContainer *container = BTK_CONTAINER (widget);

          relative_allocation.x = container->border_width;
          relative_allocation.y = container->border_width;
          relative_allocation.width = widget->allocation.width - 2 * container->border_width;
          relative_allocation.height = widget->allocation.height - 2 * container->border_width;
        }

      btk_paint_shadow (widget->style, widget->window,
			BTK_STATE_NORMAL, scrolled_window->shadow_type,
			area, widget, "scrolled_window",
			widget->allocation.x + relative_allocation.x,
			widget->allocation.y + relative_allocation.y,
			relative_allocation.width,
			relative_allocation.height);
    }
}

static gboolean
btk_scrolled_window_expose (BtkWidget      *widget,
			    BdkEventExpose *event)
{
  if (btk_widget_is_drawable (widget))
    {
      btk_scrolled_window_paint (widget, &event->area);

      BTK_WIDGET_CLASS (btk_scrolled_window_parent_class)->expose_event (widget, event);
    }

  return FALSE;
}

static void
btk_scrolled_window_forall (BtkContainer *container,
			    gboolean	  include_internals,
			    BtkCallback   callback,
			    gpointer      callback_data)
{
  g_return_if_fail (BTK_IS_SCROLLED_WINDOW (container));
  g_return_if_fail (callback != NULL);

  BTK_CONTAINER_CLASS (btk_scrolled_window_parent_class)->forall (container,
					      include_internals,
					      callback,
					      callback_data);
  if (include_internals)
    {
      BtkScrolledWindow *scrolled_window;

      scrolled_window = BTK_SCROLLED_WINDOW (container);
      
      if (scrolled_window->vscrollbar)
	callback (scrolled_window->vscrollbar, callback_data);
      if (scrolled_window->hscrollbar)
	callback (scrolled_window->hscrollbar, callback_data);
    }
}

static gboolean
btk_scrolled_window_scroll_child (BtkScrolledWindow *scrolled_window,
				  BtkScrollType      scroll,
				  gboolean           horizontal)
{
  BtkAdjustment *adjustment = NULL;
  
  switch (scroll)
    {
    case BTK_SCROLL_STEP_UP:
      scroll = BTK_SCROLL_STEP_BACKWARD;
      horizontal = FALSE;
      break;
    case BTK_SCROLL_STEP_DOWN:
      scroll = BTK_SCROLL_STEP_FORWARD;
      horizontal = FALSE;
      break;
    case BTK_SCROLL_STEP_LEFT:
      scroll = BTK_SCROLL_STEP_BACKWARD;
      horizontal = TRUE;
      break;
    case BTK_SCROLL_STEP_RIGHT:
      scroll = BTK_SCROLL_STEP_FORWARD;
      horizontal = TRUE;
      break;
    case BTK_SCROLL_PAGE_UP:
      scroll = BTK_SCROLL_PAGE_BACKWARD;
      horizontal = FALSE;
      break;
    case BTK_SCROLL_PAGE_DOWN:
      scroll = BTK_SCROLL_PAGE_FORWARD;
      horizontal = FALSE;
      break;
    case BTK_SCROLL_PAGE_LEFT:
      scroll = BTK_SCROLL_STEP_BACKWARD;
      horizontal = TRUE;
      break;
    case BTK_SCROLL_PAGE_RIGHT:
      scroll = BTK_SCROLL_STEP_FORWARD;
      horizontal = TRUE;
      break;
    case BTK_SCROLL_STEP_BACKWARD:
    case BTK_SCROLL_STEP_FORWARD:
    case BTK_SCROLL_PAGE_BACKWARD:
    case BTK_SCROLL_PAGE_FORWARD:
    case BTK_SCROLL_START:
    case BTK_SCROLL_END:
      break;
    default:
      g_warning ("Invalid scroll type %u for BtkScrolledWindow::scroll-child", scroll);
      return FALSE;
    }

  if ((horizontal && (!scrolled_window->hscrollbar || !scrolled_window->hscrollbar_visible)) ||
      (!horizontal && (!scrolled_window->vscrollbar || !scrolled_window->vscrollbar_visible)))
    return FALSE;

  if (horizontal)
    {
      if (scrolled_window->hscrollbar)
	adjustment = btk_range_get_adjustment (BTK_RANGE (scrolled_window->hscrollbar));
    }
  else
    {
      if (scrolled_window->vscrollbar)
	adjustment = btk_range_get_adjustment (BTK_RANGE (scrolled_window->vscrollbar));
    }

  if (adjustment)
    {
      gdouble value = adjustment->value;
      
      switch (scroll)
	{
	case BTK_SCROLL_STEP_FORWARD:
	  value += adjustment->step_increment;
	  break;
	case BTK_SCROLL_STEP_BACKWARD:
	  value -= adjustment->step_increment;
	  break;
	case BTK_SCROLL_PAGE_FORWARD:
	  value += adjustment->page_increment;
	  break;
	case BTK_SCROLL_PAGE_BACKWARD:
	  value -= adjustment->page_increment;
	  break;
	case BTK_SCROLL_START:
	  value = adjustment->lower;
	  break;
	case BTK_SCROLL_END:
	  value = adjustment->upper;
	  break;
	default:
	  g_assert_not_reached ();
	  break;
	}

      value = CLAMP (value, adjustment->lower, adjustment->upper - adjustment->page_size);
      
      btk_adjustment_set_value (adjustment, value);

      return TRUE;
    }

  return FALSE;
}

static void
btk_scrolled_window_move_focus_out (BtkScrolledWindow *scrolled_window,
				    BtkDirectionType   direction_type)
{
  BtkWidget *toplevel;
  
  /* Focus out of the scrolled window entirely. We do this by setting
   * a flag, then propagating the focus motion to the notebook.
   */
  toplevel = btk_widget_get_toplevel (BTK_WIDGET (scrolled_window));
  if (!btk_widget_is_toplevel (toplevel))
    return;

  g_object_ref (scrolled_window);
  
  scrolled_window->focus_out = TRUE;
  g_signal_emit_by_name (toplevel, "move-focus", direction_type);
  scrolled_window->focus_out = FALSE;
  
  g_object_unref (scrolled_window);
}

static void
btk_scrolled_window_size_request (BtkWidget      *widget,
				  BtkRequisition *requisition)
{
  BtkScrolledWindow *scrolled_window;
  BtkBin *bin;
  gint extra_width;
  gint extra_height;
  gint scrollbar_spacing;
  BtkRequisition hscrollbar_requisition;
  BtkRequisition vscrollbar_requisition;
  BtkRequisition child_requisition;

  g_return_if_fail (BTK_IS_SCROLLED_WINDOW (widget));
  g_return_if_fail (requisition != NULL);

  scrolled_window = BTK_SCROLLED_WINDOW (widget);
  bin = BTK_BIN (scrolled_window);

  scrollbar_spacing = _btk_scrolled_window_get_scrollbar_spacing (scrolled_window);

  extra_width = 0;
  extra_height = 0;
  requisition->width = 0;
  requisition->height = 0;
  
  btk_widget_size_request (scrolled_window->hscrollbar,
			   &hscrollbar_requisition);
  btk_widget_size_request (scrolled_window->vscrollbar,
			   &vscrollbar_requisition);
  
  if (bin->child && btk_widget_get_visible (bin->child))
    {
      btk_widget_size_request (bin->child, &child_requisition);

      if (scrolled_window->hscrollbar_policy == BTK_POLICY_NEVER)
	requisition->width += child_requisition.width;
      else
	{
	  BtkWidgetAuxInfo *aux_info = _btk_widget_get_aux_info (bin->child, FALSE);

	  if (aux_info && aux_info->width > 0)
	    {
	      requisition->width += aux_info->width;
	      extra_width = -1;
	    }
	  else
	    requisition->width += vscrollbar_requisition.width;
	}

      if (scrolled_window->vscrollbar_policy == BTK_POLICY_NEVER)
	requisition->height += child_requisition.height;
      else
	{
	  BtkWidgetAuxInfo *aux_info = _btk_widget_get_aux_info (bin->child, FALSE);

	  if (aux_info && aux_info->height > 0)
	    {
	      requisition->height += aux_info->height;
	      extra_height = -1;
	    }
	  else
	    requisition->height += hscrollbar_requisition.height;
	}
    }

  if (scrolled_window->hscrollbar_policy == BTK_POLICY_AUTOMATIC ||
      scrolled_window->hscrollbar_policy == BTK_POLICY_ALWAYS)
    {
      requisition->width = MAX (requisition->width, hscrollbar_requisition.width);
      if (!extra_height || scrolled_window->hscrollbar_policy == BTK_POLICY_ALWAYS)
	extra_height = scrollbar_spacing + hscrollbar_requisition.height;
    }

  if (scrolled_window->vscrollbar_policy == BTK_POLICY_AUTOMATIC ||
      scrolled_window->vscrollbar_policy == BTK_POLICY_ALWAYS)
    {
      requisition->height = MAX (requisition->height, vscrollbar_requisition.height);
      if (!extra_height || scrolled_window->vscrollbar_policy == BTK_POLICY_ALWAYS)
	extra_width = scrollbar_spacing + vscrollbar_requisition.width;
    }

  requisition->width += BTK_CONTAINER (widget)->border_width * 2 + MAX (0, extra_width);
  requisition->height += BTK_CONTAINER (widget)->border_width * 2 + MAX (0, extra_height);

  if (scrolled_window->shadow_type != BTK_SHADOW_NONE)
    {
      requisition->width += 2 * widget->style->xthickness;
      requisition->height += 2 * widget->style->ythickness;
    }
}

static void
btk_scrolled_window_relative_allocation (BtkWidget     *widget,
					 BtkAllocation *allocation)
{
  BtkScrolledWindow *scrolled_window;
  BtkScrolledWindowPrivate *priv;
  gint scrollbar_spacing;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (allocation != NULL);

  scrolled_window = BTK_SCROLLED_WINDOW (widget);
  scrollbar_spacing = _btk_scrolled_window_get_scrollbar_spacing (scrolled_window);

  priv = BTK_SCROLLED_WINDOW_GET_PRIVATE (scrolled_window);

  allocation->x = BTK_CONTAINER (widget)->border_width;
  allocation->y = BTK_CONTAINER (widget)->border_width;

  if (scrolled_window->shadow_type != BTK_SHADOW_NONE)
    {
      allocation->x += widget->style->xthickness;
      allocation->y += widget->style->ythickness;
    }
  
  allocation->width = MAX (1, (gint)widget->allocation.width - allocation->x * 2);
  allocation->height = MAX (1, (gint)widget->allocation.height - allocation->y * 2);

  if (scrolled_window->vscrollbar_visible)
    {
      BtkRequisition vscrollbar_requisition;
      gboolean is_rtl;

      btk_widget_get_child_requisition (scrolled_window->vscrollbar,
					&vscrollbar_requisition);
      is_rtl = btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL;
  
      if ((!is_rtl && 
	   (priv->real_window_placement == BTK_CORNER_TOP_RIGHT ||
	    priv->real_window_placement == BTK_CORNER_BOTTOM_RIGHT)) ||
	  (is_rtl && 
	   (priv->real_window_placement == BTK_CORNER_TOP_LEFT ||
	    priv->real_window_placement == BTK_CORNER_BOTTOM_LEFT)))
	allocation->x += (vscrollbar_requisition.width +  scrollbar_spacing);

      allocation->width = MAX (1, allocation->width - (vscrollbar_requisition.width + scrollbar_spacing));
    }
  if (scrolled_window->hscrollbar_visible)
    {
      BtkRequisition hscrollbar_requisition;
      btk_widget_get_child_requisition (scrolled_window->hscrollbar,
					&hscrollbar_requisition);
  
      if (priv->real_window_placement == BTK_CORNER_BOTTOM_LEFT ||
	  priv->real_window_placement == BTK_CORNER_BOTTOM_RIGHT)
	allocation->y += (hscrollbar_requisition.height + scrollbar_spacing);

      allocation->height = MAX (1, allocation->height - (hscrollbar_requisition.height + scrollbar_spacing));
    }
}

static void
btk_scrolled_window_size_allocate (BtkWidget     *widget,
				   BtkAllocation *allocation)
{
  BtkScrolledWindow *scrolled_window;
  BtkScrolledWindowPrivate *priv;
  BtkBin *bin;
  BtkAllocation relative_allocation;
  BtkAllocation child_allocation;
  gboolean scrollbars_within_bevel;
  gint scrollbar_spacing;
  
  g_return_if_fail (BTK_IS_SCROLLED_WINDOW (widget));
  g_return_if_fail (allocation != NULL);

  scrolled_window = BTK_SCROLLED_WINDOW (widget);
  bin = BTK_BIN (scrolled_window);

  scrollbar_spacing = _btk_scrolled_window_get_scrollbar_spacing (scrolled_window);
  btk_widget_style_get (widget, "scrollbars-within-bevel", &scrollbars_within_bevel, NULL);

  priv = BTK_SCROLLED_WINDOW_GET_PRIVATE (scrolled_window);

  widget->allocation = *allocation;

  if (scrolled_window->hscrollbar_policy == BTK_POLICY_ALWAYS)
    scrolled_window->hscrollbar_visible = TRUE;
  else if (scrolled_window->hscrollbar_policy == BTK_POLICY_NEVER)
    scrolled_window->hscrollbar_visible = FALSE;
  if (scrolled_window->vscrollbar_policy == BTK_POLICY_ALWAYS)
    scrolled_window->vscrollbar_visible = TRUE;
  else if (scrolled_window->vscrollbar_policy == BTK_POLICY_NEVER)
    scrolled_window->vscrollbar_visible = FALSE;

  if (bin->child && btk_widget_get_visible (bin->child))
    {
      gboolean previous_hvis;
      gboolean previous_vvis;
      guint count = 0;
      
      do
	{
	  btk_scrolled_window_relative_allocation (widget, &relative_allocation);
	  
	  child_allocation.x = relative_allocation.x + allocation->x;
	  child_allocation.y = relative_allocation.y + allocation->y;
	  child_allocation.width = relative_allocation.width;
	  child_allocation.height = relative_allocation.height;
	  
	  previous_hvis = scrolled_window->hscrollbar_visible;
	  previous_vvis = scrolled_window->vscrollbar_visible;
	  
	  btk_widget_size_allocate (bin->child, &child_allocation);

	  /* If, after the first iteration, the hscrollbar and the
	   * vscrollbar flip visiblity, then we need both.
	   */
	  if (count &&
	      previous_hvis != scrolled_window->hscrollbar_visible &&
	      previous_vvis != scrolled_window->vscrollbar_visible)
	    {
	      scrolled_window->hscrollbar_visible = TRUE;
	      scrolled_window->vscrollbar_visible = TRUE;

	      /* a new resize is already queued at this point,
	       * so we will immediatedly get reinvoked
	       */
	      return;
	    }
	  
	  count++;
	}
      while (previous_hvis != scrolled_window->hscrollbar_visible ||
	     previous_vvis != scrolled_window->vscrollbar_visible);
    }
  else
    {
      scrolled_window->hscrollbar_visible = scrolled_window->hscrollbar_policy == BTK_POLICY_ALWAYS;
      scrolled_window->vscrollbar_visible = scrolled_window->vscrollbar_policy == BTK_POLICY_ALWAYS;
      btk_scrolled_window_relative_allocation (widget, &relative_allocation);
    }
  
  if (scrolled_window->hscrollbar_visible)
    {
      BtkRequisition hscrollbar_requisition;
      btk_widget_get_child_requisition (scrolled_window->hscrollbar,
					&hscrollbar_requisition);
  
      if (!btk_widget_get_visible (scrolled_window->hscrollbar))
	btk_widget_show (scrolled_window->hscrollbar);

      child_allocation.x = relative_allocation.x;
      if (priv->real_window_placement == BTK_CORNER_TOP_LEFT ||
	  priv->real_window_placement == BTK_CORNER_TOP_RIGHT)
	child_allocation.y = (relative_allocation.y +
			      relative_allocation.height +
			      scrollbar_spacing +
			      (scrolled_window->shadow_type == BTK_SHADOW_NONE ?
			       0 : widget->style->ythickness));
      else
	child_allocation.y = BTK_CONTAINER (scrolled_window)->border_width;

      child_allocation.width = relative_allocation.width;
      child_allocation.height = hscrollbar_requisition.height;
      child_allocation.x += allocation->x;
      child_allocation.y += allocation->y;

      if (scrolled_window->shadow_type != BTK_SHADOW_NONE)
	{
          if (!scrollbars_within_bevel)
            {
              child_allocation.x -= widget->style->xthickness;
              child_allocation.width += 2 * widget->style->xthickness;
            }
          else if (BTK_CORNER_TOP_RIGHT == priv->real_window_placement ||
                   BTK_CORNER_TOP_LEFT == priv->real_window_placement)
            {
              child_allocation.y -= widget->style->ythickness;
            }
          else
            {
              child_allocation.y += widget->style->ythickness;
            }
	}

      btk_widget_size_allocate (scrolled_window->hscrollbar, &child_allocation);
    }
  else if (btk_widget_get_visible (scrolled_window->hscrollbar))
    btk_widget_hide (scrolled_window->hscrollbar);

  if (scrolled_window->vscrollbar_visible)
    {
      BtkRequisition vscrollbar_requisition;
      if (!btk_widget_get_visible (scrolled_window->vscrollbar))
	btk_widget_show (scrolled_window->vscrollbar);

      btk_widget_get_child_requisition (scrolled_window->vscrollbar,
					&vscrollbar_requisition);

      if ((btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL && 
	   (priv->real_window_placement == BTK_CORNER_TOP_RIGHT ||
	    priv->real_window_placement == BTK_CORNER_BOTTOM_RIGHT)) ||
	  (btk_widget_get_direction (widget) == BTK_TEXT_DIR_LTR && 
	   (priv->real_window_placement == BTK_CORNER_TOP_LEFT ||
	    priv->real_window_placement == BTK_CORNER_BOTTOM_LEFT)))
	child_allocation.x = (relative_allocation.x +
			      relative_allocation.width +
			      scrollbar_spacing +
			      (scrolled_window->shadow_type == BTK_SHADOW_NONE ?
			       0 : widget->style->xthickness));
      else
	child_allocation.x = BTK_CONTAINER (scrolled_window)->border_width;

      child_allocation.y = relative_allocation.y;
      child_allocation.width = vscrollbar_requisition.width;
      child_allocation.height = relative_allocation.height;
      child_allocation.x += allocation->x;
      child_allocation.y += allocation->y;

      if (scrolled_window->shadow_type != BTK_SHADOW_NONE)
	{
          if (!scrollbars_within_bevel)
            {
              child_allocation.y -= widget->style->ythickness;
	      child_allocation.height += 2 * widget->style->ythickness;
            }
          else if (BTK_CORNER_BOTTOM_LEFT == priv->real_window_placement ||
                   BTK_CORNER_TOP_LEFT == priv->real_window_placement)
            {
              child_allocation.x -= widget->style->xthickness;
            }
          else
            {
              child_allocation.x += widget->style->xthickness;
            }
	}

      btk_widget_size_allocate (scrolled_window->vscrollbar, &child_allocation);
    }
  else if (btk_widget_get_visible (scrolled_window->vscrollbar))
    btk_widget_hide (scrolled_window->vscrollbar);
}

static gboolean
btk_scrolled_window_scroll_event (BtkWidget      *widget,
				  BdkEventScroll *event)
{
  BtkWidget *range;

  g_return_val_if_fail (BTK_IS_SCROLLED_WINDOW (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);  

  if (event->direction == BDK_SCROLL_UP || event->direction == BDK_SCROLL_DOWN)
    range = BTK_SCROLLED_WINDOW (widget)->vscrollbar;
  else
    range = BTK_SCROLLED_WINDOW (widget)->hscrollbar;

  if (range && btk_widget_get_visible (range))
    {
      BtkAdjustment *adj = BTK_RANGE (range)->adjustment;
      gdouble delta, new_value;

      delta = _btk_range_get_wheel_delta (BTK_RANGE (range), event->direction);

      new_value = CLAMP (adj->value + delta, adj->lower, adj->upper - adj->page_size);
      
      btk_adjustment_set_value (adj, new_value);

      return TRUE;
    }

  return FALSE;
}

static gboolean
btk_scrolled_window_focus (BtkWidget        *widget,
			   BtkDirectionType  direction)
{
  BtkScrolledWindow *scrolled_window = BTK_SCROLLED_WINDOW (widget);
  gboolean had_focus_child = BTK_CONTAINER (widget)->focus_child != NULL;
  
  if (scrolled_window->focus_out)
    {
      scrolled_window->focus_out = FALSE; /* Clear this to catch the wrap-around case */
      return FALSE;
    }
  
  if (btk_widget_is_focus (widget))
    return FALSE;

  /* We only put the scrolled window itself in the focus chain if it
   * isn't possible to focus any children.
   */
  if (BTK_BIN (widget)->child)
    {
      if (btk_widget_child_focus (BTK_BIN (widget)->child, direction))
	return TRUE;
    }

  if (!had_focus_child && btk_widget_get_can_focus (widget))
    {
      btk_widget_grab_focus (widget);
      return TRUE;
    }
  else
    return FALSE;
}

static void
btk_scrolled_window_adjustment_changed (BtkAdjustment *adjustment,
					gpointer       data)
{
  BtkScrolledWindow *scrolled_win;

  g_return_if_fail (adjustment != NULL);
  g_return_if_fail (data != NULL);

  scrolled_win = BTK_SCROLLED_WINDOW (data);

  if (scrolled_win->hscrollbar &&
      adjustment == btk_range_get_adjustment (BTK_RANGE (scrolled_win->hscrollbar)))
    {
      if (scrolled_win->hscrollbar_policy == BTK_POLICY_AUTOMATIC)
	{
	  gboolean visible;
	  
	  visible = scrolled_win->hscrollbar_visible;
	  scrolled_win->hscrollbar_visible = (adjustment->upper - adjustment->lower >
					      adjustment->page_size);
	  if (scrolled_win->hscrollbar_visible != visible)
	    btk_widget_queue_resize (BTK_WIDGET (scrolled_win));
	}
    }
  else if (scrolled_win->vscrollbar &&
	   adjustment == btk_range_get_adjustment (BTK_RANGE (scrolled_win->vscrollbar)))
    {
      if (scrolled_win->vscrollbar_policy == BTK_POLICY_AUTOMATIC)
	{
	  gboolean visible;

	  visible = scrolled_win->vscrollbar_visible;
	  scrolled_win->vscrollbar_visible = (adjustment->upper - adjustment->lower >
					      adjustment->page_size);
	  if (scrolled_win->vscrollbar_visible != visible)
	    btk_widget_queue_resize (BTK_WIDGET (scrolled_win));
	}
    }
}

static void
btk_scrolled_window_add (BtkContainer *container,
			 BtkWidget    *child)
{
  BtkScrolledWindow *scrolled_window;
  BtkBin *bin;

  bin = BTK_BIN (container);
  g_return_if_fail (bin->child == NULL);

  scrolled_window = BTK_SCROLLED_WINDOW (container);

  bin->child = child;
  btk_widget_set_parent (child, BTK_WIDGET (bin));

  /* this is a temporary message */
  if (!btk_widget_set_scroll_adjustments (child,
					  btk_range_get_adjustment (BTK_RANGE (scrolled_window->hscrollbar)),
					  btk_range_get_adjustment (BTK_RANGE (scrolled_window->vscrollbar))))
    g_warning ("btk_scrolled_window_add(): cannot add non scrollable widget "
	       "use btk_scrolled_window_add_with_viewport() instead");
}

static void
btk_scrolled_window_remove (BtkContainer *container,
			    BtkWidget    *child)
{
  g_return_if_fail (BTK_IS_SCROLLED_WINDOW (container));
  g_return_if_fail (child != NULL);
  g_return_if_fail (BTK_BIN (container)->child == child);
  
  btk_widget_set_scroll_adjustments (child, NULL, NULL);

  /* chain parent class handler to remove child */
  BTK_CONTAINER_CLASS (btk_scrolled_window_parent_class)->remove (container, child);
}

/**
 * btk_scrolled_window_add_with_viewport:
 * @scrolled_window: a #BtkScrolledWindow
 * @child: the widget you want to scroll
 *
 * Used to add children without native scrolling capabilities. This
 * is simply a convenience function; it is equivalent to adding the
 * unscrollable child to a viewport, then adding the viewport to the
 * scrolled window. If a child has native scrolling, use
 * btk_container_add() instead of this function.
 *
 * The viewport scrolls the child by moving its #BdkWindow, and takes
 * the size of the child to be the size of its toplevel #BdkWindow. 
 * This will be very wrong for most widgets that support native scrolling;
 * for example, if you add a widget such as #BtkTreeView with a viewport,
 * the whole widget will scroll, including the column headings. Thus, 
 * widgets with native scrolling support should not be used with the 
 * #BtkViewport proxy.
 *
 * A widget supports scrolling natively if the 
 * set_scroll_adjustments_signal field in #BtkWidgetClass is non-zero,
 * i.e. has been filled in with a valid signal identifier.
 */
void
btk_scrolled_window_add_with_viewport (BtkScrolledWindow *scrolled_window,
				       BtkWidget         *child)
{
  BtkBin *bin;
  BtkWidget *viewport;

  g_return_if_fail (BTK_IS_SCROLLED_WINDOW (scrolled_window));
  g_return_if_fail (BTK_IS_WIDGET (child));
  g_return_if_fail (child->parent == NULL);

  bin = BTK_BIN (scrolled_window);

  if (bin->child != NULL)
    {
      g_return_if_fail (BTK_IS_VIEWPORT (bin->child));
      g_return_if_fail (BTK_BIN (bin->child)->child == NULL);

      viewport = bin->child;
    }
  else
    {
      viewport =
        btk_viewport_new (btk_scrolled_window_get_hadjustment (scrolled_window),
			  btk_scrolled_window_get_vadjustment (scrolled_window));
      btk_container_add (BTK_CONTAINER (scrolled_window), viewport);
    }

  btk_widget_show (viewport);
  btk_container_add (BTK_CONTAINER (viewport), child);
}

/*
 * _btk_scrolled_window_get_spacing:
 * @scrolled_window: a scrolled window
 * 
 * Gets the spacing between the scrolled window's scrollbars and
 * the scrolled widget. Used by BtkCombo
 * 
 * Return value: the spacing, in pixels.
 */
gint
_btk_scrolled_window_get_scrollbar_spacing (BtkScrolledWindow *scrolled_window)
{
  BtkScrolledWindowClass *class;
    
  g_return_val_if_fail (BTK_IS_SCROLLED_WINDOW (scrolled_window), 0);

  class = BTK_SCROLLED_WINDOW_GET_CLASS (scrolled_window);

  if (class->scrollbar_spacing >= 0)
    return class->scrollbar_spacing;
  else
    {
      gint scrollbar_spacing;
      
      btk_widget_style_get (BTK_WIDGET (scrolled_window),
			    "scrollbar-spacing", &scrollbar_spacing,
			    NULL);

      return scrollbar_spacing;
    }
}

#define __BTK_SCROLLED_WINDOW_C__
#include "btkaliasdef.c"
