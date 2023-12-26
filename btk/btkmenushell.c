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

#define BTK_MENU_INTERNALS

#include "config.h"
#include "bdk/bdkkeysyms.h"
#include "btkbindings.h"
#include "btkkeyhash.h"
#include "btklabel.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkmenu.h"
#include "btkmenubar.h"
#include "btkmenuitem.h"
#include "btkmenushell.h"
#include "btkmnemonichash.h"
#include "btktearoffmenuitem.h"
#include "btkwindow.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

#define MENU_SHELL_TIMEOUT   500

#define PACK_DIRECTION(m)                                 \
   (BTK_IS_MENU_BAR (m)                                   \
     ? btk_menu_bar_get_pack_direction (BTK_MENU_BAR (m)) \
     : BTK_PACK_DIRECTION_LTR)

enum {
  DEACTIVATE,
  SELECTION_DONE,
  MOVE_CURRENT,
  ACTIVATE_CURRENT,
  CANCEL,
  CYCLE_FOCUS,
  MOVE_SELECTED,
  INSERT,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_TAKE_FOCUS
};

/* Terminology:
 * 
 * A menu item can be "selected", this means that it is displayed
 * in the prelight state, and if it has a submenu, that submenu
 * will be popped up. 
 * 
 * A menu is "active" when it is visible onscreen and the user
 * is selecting from it. A menubar is not active until the user
 * clicks on one of its menuitems. When a menu is active,
 * passing the mouse over a submenu will pop it up.
 *
 * menu_shell->active_menu_item, is however, not an "active"
 * menu item (there is no such thing) but rather, the selected
 * menu item in that MenuShell, if there is one.
 *
 * There is also is a concept of the current menu and a current
 * menu item. The current menu item is the selected menu item
 * that is furthest down in the hierarchy. (Every active menu_shell
 * does not necessarily contain a selected menu item, but if
 * it does, then menu_shell->parent_menu_shell must also contain
 * a selected menu item. The current menu is the menu that 
 * contains the current menu_item. It will always have a BTK
 * grab and receive all key presses.
 *
 *
 * Action signals:
 *
 *  ::move_current (BtkMenuDirection *dir)
 *     Moves the current menu item in direction 'dir':
 *
 *       BTK_MENU_DIR_PARENT: To the parent menu shell
 *       BTK_MENU_DIR_CHILD: To the child menu shell (if this item has
 *          a submenu.
 *       BTK_MENU_DIR_NEXT/PREV: To the next or previous item
 *          in this menu.
 * 
 *     As a a bit of a hack to get movement between menus and
 *     menubars working, if submenu_placement is different for
 *     the menu and its MenuShell then the following apply:
 * 
 *       - For 'parent' the current menu is not just moved to
 *         the parent, but moved to the previous entry in the parent
 *       - For 'child', if there is no child, then current is
 *         moved to the next item in the parent.
 *
 *    Note that the above explanation of ::move_current was written
 *    before menus and menubars had support for RTL flipping and
 *    different packing directions, and therefore only applies for
 *    when text direction and packing direction are both left-to-right.
 * 
 *  ::activate_current (GBoolean *force_hide)
 *     Activate the current item. If 'force_hide' is true, hide
 *     the current menu item always. Otherwise, only hide
 *     it if menu_item->klass->hide_on_activate is true.
 *
 *  ::cancel ()
 *     Cancels the current selection
 */

#define BTK_MENU_SHELL_GET_PRIVATE(obj) (B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_MENU_SHELL, BtkMenuShellPrivate))

typedef struct _BtkMenuShellPrivate BtkMenuShellPrivate;

struct _BtkMenuShellPrivate
{
  BtkMnemonicHash *mnemonic_hash;
  BtkKeyHash *key_hash;

  guint take_focus : 1;
  guint activated_submenu : 1;
  /* This flag is a crutch to keep mnemonics in the same menu
   * if the user moves the mouse over an unselectable menuitem.
   */
  guint in_unselectable_item : 1;
};

static void btk_menu_shell_set_property      (BObject           *object,
                                              guint              prop_id,
                                              const BValue      *value,
                                              BParamSpec        *pspec);
static void btk_menu_shell_get_property      (BObject           *object,
                                              guint              prop_id,
                                              BValue            *value,
                                              BParamSpec        *pspec);
static void btk_menu_shell_realize           (BtkWidget         *widget);
static void btk_menu_shell_finalize          (BObject           *object);
static gint btk_menu_shell_button_press      (BtkWidget         *widget,
					      BdkEventButton    *event);
static gint btk_menu_shell_button_release    (BtkWidget         *widget,
					      BdkEventButton    *event);
static gint btk_menu_shell_key_press         (BtkWidget	        *widget,
					      BdkEventKey       *event);
static gint btk_menu_shell_enter_notify      (BtkWidget         *widget,
					      BdkEventCrossing  *event);
static gint btk_menu_shell_leave_notify      (BtkWidget         *widget,
					      BdkEventCrossing  *event);
static void btk_menu_shell_screen_changed    (BtkWidget         *widget,
					      BdkScreen         *previous_screen);
static gboolean btk_menu_shell_grab_broken       (BtkWidget         *widget,
					      BdkEventGrabBroken *event);
static void btk_menu_shell_add               (BtkContainer      *container,
					      BtkWidget         *widget);
static void btk_menu_shell_remove            (BtkContainer      *container,
					      BtkWidget         *widget);
static void btk_menu_shell_forall            (BtkContainer      *container,
					      gboolean		 include_internals,
					      BtkCallback        callback,
					      gpointer           callback_data);
static void btk_menu_shell_real_insert       (BtkMenuShell *menu_shell,
					      BtkWidget    *child,
					      gint          position);
static void btk_real_menu_shell_deactivate   (BtkMenuShell      *menu_shell);
static gint btk_menu_shell_is_item           (BtkMenuShell      *menu_shell,
					      BtkWidget         *child);
static BtkWidget *btk_menu_shell_get_item    (BtkMenuShell      *menu_shell,
					      BdkEvent          *event);
static GType    btk_menu_shell_child_type  (BtkContainer      *container);
static void btk_menu_shell_real_select_item  (BtkMenuShell      *menu_shell,
					      BtkWidget         *menu_item);
static gboolean btk_menu_shell_select_submenu_first (BtkMenuShell   *menu_shell); 

static void btk_real_menu_shell_move_current (BtkMenuShell      *menu_shell,
					      BtkMenuDirectionType direction);
static void btk_real_menu_shell_activate_current (BtkMenuShell      *menu_shell,
						  gboolean           force_hide);
static void btk_real_menu_shell_cancel           (BtkMenuShell      *menu_shell);
static void btk_real_menu_shell_cycle_focus      (BtkMenuShell      *menu_shell,
						  BtkDirectionType   dir);

static void     btk_menu_shell_reset_key_hash    (BtkMenuShell *menu_shell);
static gboolean btk_menu_shell_activate_mnemonic (BtkMenuShell *menu_shell,
						  BdkEventKey  *event);
static gboolean btk_menu_shell_real_move_selected (BtkMenuShell  *menu_shell, 
						   gint           distance);

static guint menu_shell_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_ABSTRACT_TYPE (BtkMenuShell, btk_menu_shell, BTK_TYPE_CONTAINER)

static void
btk_menu_shell_class_init (BtkMenuShellClass *klass)
{
  BObjectClass *object_class;
  BtkWidgetClass *widget_class;
  BtkContainerClass *container_class;

  BtkBindingSet *binding_set;

  object_class = (BObjectClass*) klass;
  widget_class = (BtkWidgetClass*) klass;
  container_class = (BtkContainerClass*) klass;

  object_class->set_property = btk_menu_shell_set_property;
  object_class->get_property = btk_menu_shell_get_property;
  object_class->finalize = btk_menu_shell_finalize;

  widget_class->realize = btk_menu_shell_realize;
  widget_class->button_press_event = btk_menu_shell_button_press;
  widget_class->button_release_event = btk_menu_shell_button_release;
  widget_class->grab_broken_event = btk_menu_shell_grab_broken;
  widget_class->key_press_event = btk_menu_shell_key_press;
  widget_class->enter_notify_event = btk_menu_shell_enter_notify;
  widget_class->leave_notify_event = btk_menu_shell_leave_notify;
  widget_class->screen_changed = btk_menu_shell_screen_changed;

  container_class->add = btk_menu_shell_add;
  container_class->remove = btk_menu_shell_remove;
  container_class->forall = btk_menu_shell_forall;
  container_class->child_type = btk_menu_shell_child_type;

  klass->submenu_placement = BTK_TOP_BOTTOM;
  klass->deactivate = btk_real_menu_shell_deactivate;
  klass->selection_done = NULL;
  klass->move_current = btk_real_menu_shell_move_current;
  klass->activate_current = btk_real_menu_shell_activate_current;
  klass->cancel = btk_real_menu_shell_cancel;
  klass->select_item = btk_menu_shell_real_select_item;
  klass->insert = btk_menu_shell_real_insert;
  klass->move_selected = btk_menu_shell_real_move_selected;

  menu_shell_signals[DEACTIVATE] =
    g_signal_new (I_("deactivate"),
		  B_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkMenuShellClass, deactivate),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);

  menu_shell_signals[SELECTION_DONE] =
    g_signal_new (I_("selection-done"),
		  B_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkMenuShellClass, selection_done),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);

  menu_shell_signals[MOVE_CURRENT] =
    g_signal_new (I_("move-current"),
		  B_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkMenuShellClass, move_current),
		  NULL, NULL,
		  _btk_marshal_VOID__ENUM,
		  B_TYPE_NONE, 1,
		  BTK_TYPE_MENU_DIRECTION_TYPE);

  menu_shell_signals[ACTIVATE_CURRENT] =
    g_signal_new (I_("activate-current"),
		  B_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkMenuShellClass, activate_current),
		  NULL, NULL,
		  _btk_marshal_VOID__BOOLEAN,
		  B_TYPE_NONE, 1,
		  B_TYPE_BOOLEAN);

  menu_shell_signals[CANCEL] =
    g_signal_new (I_("cancel"),
		  B_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkMenuShellClass, cancel),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);

  menu_shell_signals[CYCLE_FOCUS] =
    g_signal_new_class_handler (I_("cycle-focus"),
                                B_OBJECT_CLASS_TYPE (object_class),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (btk_real_menu_shell_cycle_focus),
                                NULL, NULL,
                                _btk_marshal_VOID__ENUM,
                                B_TYPE_NONE, 1,
                                BTK_TYPE_DIRECTION_TYPE);

  /**
   * BtkMenuShell::move-selected:
   * @menu_shell: the object on which the signal is emitted
   * @distance: +1 to move to the next item, -1 to move to the previous
   *
   * The ::move-selected signal is emitted to move the selection to
   * another item.
   *
   * Returns: %TRUE to stop the signal emission, %FALSE to continue
   *
   * Since: 2.12
   */
  menu_shell_signals[MOVE_SELECTED] =
    g_signal_new (I_("move-selected"),
		  B_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkMenuShellClass, move_selected),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__INT,
		  B_TYPE_BOOLEAN, 1,
		  B_TYPE_INT);

  /**
   * BtkMenuShell::insert:
   * @menu_shell: the object on which the signal is emitted
   * @child: the #BtkMenuItem that is being inserted
   * @position: the position at which the insert occurs
   *
   * The ::insert signal is emitted when a new #BtkMenuItem is added to
   * a #BtkMenuShell.  A separate signal is used instead of
   * BtkContainer::add because of the need for an additional position
   * parameter.
   *
   * The inverse of this signal is the BtkContainer::remove signal.
   *
   * Since: 2.24.15
   **/
  menu_shell_signals[INSERT] =
    g_signal_new (I_("insert"),
                  B_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BtkMenuShellClass, insert),
                  NULL, NULL,
                  _btk_marshal_VOID__OBJECT_INT,
                  B_TYPE_NONE, 2, BTK_TYPE_WIDGET, B_TYPE_INT);

  binding_set = btk_binding_set_by_class (klass);
  btk_binding_entry_add_signal (binding_set,
				BDK_Escape, 0,
				"cancel", 0);
  btk_binding_entry_add_signal (binding_set,
				BDK_Return, 0,
				"activate-current", 1,
				B_TYPE_BOOLEAN,
				TRUE);
  btk_binding_entry_add_signal (binding_set,
				BDK_ISO_Enter, 0,
				"activate-current", 1,
				B_TYPE_BOOLEAN,
				TRUE);
  btk_binding_entry_add_signal (binding_set,
				BDK_KP_Enter, 0,
				"activate-current", 1,
				B_TYPE_BOOLEAN,
				TRUE);
  btk_binding_entry_add_signal (binding_set,
				BDK_space, 0,
				"activate-current", 1,
				B_TYPE_BOOLEAN,
				FALSE);
  btk_binding_entry_add_signal (binding_set,
				BDK_KP_Space, 0,
				"activate-current", 1,
				B_TYPE_BOOLEAN,
				FALSE);
  btk_binding_entry_add_signal (binding_set,
				BDK_F10, 0,
				"cycle-focus", 1,
                                BTK_TYPE_DIRECTION_TYPE, BTK_DIR_TAB_FORWARD);
  btk_binding_entry_add_signal (binding_set,
				BDK_F10, BDK_SHIFT_MASK,
				"cycle-focus", 1,
                                BTK_TYPE_DIRECTION_TYPE, BTK_DIR_TAB_BACKWARD);

  /**
   * BtkMenuShell:take-focus:
   *
   * A boolean that determines whether the menu and its submenus grab the
   * keyboard focus. See btk_menu_shell_set_take_focus() and
   * btk_menu_shell_get_take_focus().
   *
   * Since: 2.8
   **/
  g_object_class_install_property (object_class,
                                   PROP_TAKE_FOCUS,
                                   g_param_spec_boolean ("take-focus",
							 P_("Take Focus"),
							 P_("A boolean that determines whether the menu grabs the keyboard focus"),
							 TRUE,
							 BTK_PARAM_READWRITE));

  g_type_class_add_private (object_class, sizeof (BtkMenuShellPrivate));
}

static GType
btk_menu_shell_child_type (BtkContainer     *container)
{
  return BTK_TYPE_MENU_ITEM;
}

static void
btk_menu_shell_init (BtkMenuShell *menu_shell)
{
  BtkMenuShellPrivate *priv = BTK_MENU_SHELL_GET_PRIVATE (menu_shell);

  menu_shell->children = NULL;
  menu_shell->active_menu_item = NULL;
  menu_shell->parent_menu_shell = NULL;
  menu_shell->active = FALSE;
  menu_shell->have_grab = FALSE;
  menu_shell->have_xgrab = FALSE;
  menu_shell->button = 0;
  menu_shell->activate_time = 0;

  priv->mnemonic_hash = NULL;
  priv->key_hash = NULL;
  priv->take_focus = TRUE;
  priv->activated_submenu = FALSE;
}

static void
btk_menu_shell_set_property (BObject      *object,
                             guint         prop_id,
                             const BValue *value,
                             BParamSpec   *pspec)
{
  BtkMenuShell *menu_shell = BTK_MENU_SHELL (object);

  switch (prop_id)
    {
    case PROP_TAKE_FOCUS:
      btk_menu_shell_set_take_focus (menu_shell, b_value_get_boolean (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_menu_shell_get_property (BObject     *object,
                             guint        prop_id,
                             BValue      *value,
                             BParamSpec  *pspec)
{
  BtkMenuShell *menu_shell = BTK_MENU_SHELL (object);

  switch (prop_id)
    {
    case PROP_TAKE_FOCUS:
      b_value_set_boolean (value, btk_menu_shell_get_take_focus (menu_shell));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_menu_shell_finalize (BObject *object)
{
  BtkMenuShell *menu_shell = BTK_MENU_SHELL (object);
  BtkMenuShellPrivate *priv = BTK_MENU_SHELL_GET_PRIVATE (menu_shell);

  if (priv->mnemonic_hash)
    _btk_mnemonic_hash_free (priv->mnemonic_hash);
  if (priv->key_hash)
    _btk_key_hash_free (priv->key_hash);

  B_OBJECT_CLASS (btk_menu_shell_parent_class)->finalize (object);
}


void
btk_menu_shell_append (BtkMenuShell *menu_shell,
		       BtkWidget    *child)
{
  btk_menu_shell_insert (menu_shell, child, -1);
}

void
btk_menu_shell_prepend (BtkMenuShell *menu_shell,
			BtkWidget    *child)
{
  btk_menu_shell_insert (menu_shell, child, 0);
}

void
btk_menu_shell_insert (BtkMenuShell *menu_shell,
		       BtkWidget    *child,
		       gint          position)
{
  g_return_if_fail (BTK_IS_MENU_SHELL (menu_shell));
  g_return_if_fail (BTK_IS_MENU_ITEM (child));

  g_signal_emit (menu_shell, menu_shell_signals[INSERT], 0, child, position);
}

static void
btk_menu_shell_real_insert (BtkMenuShell *menu_shell,
			    BtkWidget    *child,
			    gint          position)
{
  menu_shell->children = g_list_insert (menu_shell->children, child, position);

  btk_widget_set_parent (child, BTK_WIDGET (menu_shell));
}

void
btk_menu_shell_deactivate (BtkMenuShell *menu_shell)
{
  g_return_if_fail (BTK_IS_MENU_SHELL (menu_shell));

  g_signal_emit (menu_shell, menu_shell_signals[DEACTIVATE], 0);
}

static void
btk_menu_shell_realize (BtkWidget *widget)
{
  BdkWindowAttr attributes;
  gint attributes_mask;

  btk_widget_set_realized (widget, TRUE);

  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);
  attributes.event_mask = btk_widget_get_events (widget);
  attributes.event_mask |= (BDK_EXPOSURE_MASK |
			    BDK_BUTTON_PRESS_MASK |
			    BDK_BUTTON_RELEASE_MASK |
			    BDK_KEY_PRESS_MASK |
			    BDK_ENTER_NOTIFY_MASK |
			    BDK_LEAVE_NOTIFY_MASK);

  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;
  widget->window = bdk_window_new (btk_widget_get_parent_window (widget), &attributes, attributes_mask);
  bdk_window_set_user_data (widget->window, widget);

  widget->style = btk_style_attach (widget->style, widget->window);
  btk_style_set_background (widget->style, widget->window, BTK_STATE_NORMAL);
}

static void
btk_menu_shell_activate (BtkMenuShell *menu_shell)
{
  if (!menu_shell->active)
    {
      btk_grab_add (BTK_WIDGET (menu_shell));
      menu_shell->have_grab = TRUE;
      menu_shell->active = TRUE;
    }
}

static gint
btk_menu_shell_button_press (BtkWidget      *widget,
			     BdkEventButton *event)
{
  BtkMenuShell *menu_shell;
  BtkWidget *menu_item;

  if (event->type != BDK_BUTTON_PRESS)
    return FALSE;

  menu_shell = BTK_MENU_SHELL (widget);

  if (menu_shell->parent_menu_shell)
    return btk_widget_event (menu_shell->parent_menu_shell, (BdkEvent*) event);

  menu_item = btk_menu_shell_get_item (menu_shell, (BdkEvent *)event);

  if (menu_item && _btk_menu_item_is_selectable (menu_item) &&
      menu_item != BTK_MENU_SHELL (menu_item->parent)->active_menu_item)
    {
      /*  select the menu item *before* activating the shell, so submenus
       *  which might be open are closed the friendly way. If we activate
       *  (and thus grab) this menu shell first, we might get grab_broken
       *  events which will close the entire menu hierarchy. Selecting the
       *  menu item also fixes up the state as if enter_notify() would
       *  have run before (which normally selects the item).
       */
      if (BTK_MENU_SHELL_GET_CLASS (menu_item->parent)->submenu_placement != BTK_TOP_BOTTOM)
        {
          btk_menu_shell_select_item (BTK_MENU_SHELL (menu_item->parent), menu_item);
        }
    }

  if (!menu_shell->active || !menu_shell->button)
    {
      btk_menu_shell_activate (menu_shell);

      menu_shell->button = event->button;

      if (menu_item && _btk_menu_item_is_selectable (menu_item) &&
	  menu_item->parent == widget &&
          menu_item != menu_shell->active_menu_item)
        {
          if (BTK_MENU_SHELL_GET_CLASS (menu_shell)->submenu_placement == BTK_TOP_BOTTOM)
            {
              menu_shell->activate_time = event->time;
              btk_menu_shell_select_item (menu_shell, menu_item);
            }
        }
    }
  else
    {
      widget = btk_get_event_widget ((BdkEvent*) event);
      if (widget == BTK_WIDGET (menu_shell))
	{
	  btk_menu_shell_deactivate (menu_shell);
	  g_signal_emit (menu_shell, menu_shell_signals[SELECTION_DONE], 0);
	}
    }

  if (menu_item && _btk_menu_item_is_selectable (menu_item) &&
      BTK_MENU_ITEM (menu_item)->submenu != NULL &&
      !btk_widget_get_visible (BTK_MENU_ITEM (menu_item)->submenu))
    {
      BtkMenuShellPrivate *priv;

      _btk_menu_item_popup_submenu (menu_item, FALSE);

      priv = BTK_MENU_SHELL_GET_PRIVATE (menu_item->parent);
      priv->activated_submenu = TRUE;
    }

  return TRUE;
}

static gboolean
btk_menu_shell_grab_broken (BtkWidget          *widget,
			    BdkEventGrabBroken *event)
{
  BtkMenuShell *menu_shell = BTK_MENU_SHELL (widget);

  if (menu_shell->have_xgrab && event->grab_window == NULL)
    {
      /* Unset the active menu item so btk_menu_popdown() doesn't see it.
       */
      
      btk_menu_shell_deselect (menu_shell);
      
      btk_menu_shell_deactivate (menu_shell);
      g_signal_emit (menu_shell, menu_shell_signals[SELECTION_DONE], 0);
    }

  return TRUE;
}

static gint
btk_menu_shell_button_release (BtkWidget      *widget,
			       BdkEventButton *event)
{
  BtkMenuShell *menu_shell = BTK_MENU_SHELL (widget);
  BtkMenuShellPrivate *priv = BTK_MENU_SHELL_GET_PRIVATE (widget);

  if (menu_shell->active)
    {
      BtkWidget *menu_item;
      gboolean   deactivate = TRUE;

      if (menu_shell->button && (event->button != menu_shell->button))
	{
	  menu_shell->button = 0;
	  if (menu_shell->parent_menu_shell)
	    return btk_widget_event (menu_shell->parent_menu_shell, (BdkEvent*) event);
	}

      menu_shell->button = 0;
      menu_item = btk_menu_shell_get_item (menu_shell, (BdkEvent*) event);

      if ((event->time - menu_shell->activate_time) > MENU_SHELL_TIMEOUT)
        {
          if (menu_item && (menu_shell->active_menu_item == menu_item) &&
              _btk_menu_item_is_selectable (menu_item))
            {
              BtkWidget *submenu = BTK_MENU_ITEM (menu_item)->submenu;

              if (submenu == NULL)
                {
                  btk_menu_shell_activate_item (menu_shell, menu_item, TRUE);

                  deactivate = FALSE;
                }
              else if (BTK_MENU_SHELL_GET_CLASS (menu_shell)->submenu_placement != BTK_TOP_BOTTOM ||
                       priv->activated_submenu)
                {
                  gint popdown_delay;
                  GTimeVal *popup_time;
                  gint64 usec_since_popup = 0;

                  g_object_get (btk_widget_get_settings (widget),
                                "btk-menu-popdown-delay", &popdown_delay,
                                NULL);

                  popup_time = g_object_get_data (B_OBJECT (submenu),
                                                  "btk-menu-exact-popup-time");

                  if (popup_time)
                    {
                      GTimeVal current_time;

                      g_get_current_time (&current_time);

                      usec_since_popup = ((gint64) current_time.tv_sec * 1000 * 1000 +
                                          (gint64) current_time.tv_usec -
                                          (gint64) popup_time->tv_sec * 1000 * 1000 -
                                          (gint64) popup_time->tv_usec);

                      g_object_set_data (B_OBJECT (submenu),
                                         "btk-menu-exact-popup-time", NULL);
                    }

                  /*  only close the submenu on click if we opened the
                   *  menu explicitely (usec_since_popup == 0) or
                   *  enough time has passed since it was opened by
                   *  BtkMenuItem's timeout (usec_since_popup > delay).
                   */
                  if (!priv->activated_submenu &&
                      (usec_since_popup == 0 ||
                       usec_since_popup > popdown_delay * 1000))
                    {
                      _btk_menu_item_popdown_submenu (menu_item);
                    }
                  else
                    {
                      btk_menu_item_select (BTK_MENU_ITEM (menu_item));
                    }

                  deactivate = FALSE;
                }
            }
          else if (menu_item &&
                   !_btk_menu_item_is_selectable (menu_item) &&
                   BTK_MENU_SHELL_GET_CLASS (menu_shell)->submenu_placement != BTK_TOP_BOTTOM)
            {
              deactivate = FALSE;
            }
          else if (menu_shell->parent_menu_shell)
            {
              menu_shell->active = TRUE;
              btk_widget_event (menu_shell->parent_menu_shell, (BdkEvent*) event);
              deactivate = FALSE;
            }

          /* If we ended up on an item with a submenu, leave the menu up.
           */
          if (menu_item && (menu_shell->active_menu_item == menu_item) &&
              BTK_MENU_SHELL_GET_CLASS (menu_shell)->submenu_placement != BTK_TOP_BOTTOM)
            {
              deactivate = FALSE;
            }
        }
      else /* a very fast press-release */
        {
          /* We only ever want to prevent deactivation on the first
           * press/release. Setting the time to zero is a bit of a
           * hack, since we could be being triggered in the first
           * few fractions of a second after a server time wraparound.
           * the chances of that happening are ~1/10^6, without
           * serious harm if we lose.
           */
          menu_shell->activate_time = 0;
          deactivate = FALSE;
        }

      if (deactivate)
        {
          btk_menu_shell_deactivate (menu_shell);
          g_signal_emit (menu_shell, menu_shell_signals[SELECTION_DONE], 0);
        }

      priv->activated_submenu = FALSE;
    }

  return TRUE;
}

void
_btk_menu_shell_set_keyboard_mode (BtkMenuShell *menu_shell,
                                   gboolean      keyboard_mode)
{
  menu_shell->keyboard_mode = keyboard_mode;
}

gboolean
_btk_menu_shell_get_keyboard_mode (BtkMenuShell *menu_shell)
{
  return menu_shell->keyboard_mode;
}

void
_btk_menu_shell_update_mnemonics (BtkMenuShell *menu_shell)
{
  BtkMenuShell *target;
  gboolean auto_mnemonics;
  gboolean found;
  gboolean mnemonics_visible;

  g_object_get (btk_widget_get_settings (BTK_WIDGET (menu_shell)),
                "btk-auto-mnemonics", &auto_mnemonics, NULL);

  if (!auto_mnemonics)
    return;

  target = menu_shell;
  found = FALSE;
  while (target)
    {
      BtkMenuShellPrivate *priv = BTK_MENU_SHELL_GET_PRIVATE (target);
      BtkWidget *toplevel = btk_widget_get_toplevel (BTK_WIDGET (target));

      /* The idea with keyboard mode is that once you start using
       * the keyboard to navigate the menus, we show mnemonics
       * until the menu navigation is over. To that end, we spread
       * the keyboard mode upwards in the menu hierarchy here.
       * Also see btk_menu_popup, where we inherit it downwards.
       */
      if (menu_shell->keyboard_mode)
        target->keyboard_mode = TRUE;

      /* While navigating menus, the first parent menu with an active
       * item is the one where mnemonics are effective, as can be seen
       * in btk_menu_shell_key_press below.
       * We also show mnemonics in context menus. The grab condition is
       * necessary to ensure we remove underlines from menu bars when
       * dismissing menus.
       */
      mnemonics_visible = target->keyboard_mode &&
                          (((target->active_menu_item || priv->in_unselectable_item) && !found) ||
                           (target == menu_shell &&
                            !target->parent_menu_shell &&
                            btk_widget_has_grab (BTK_WIDGET (target))));

      /* While menus are up, only show underlines inside the menubar,
       * not in the entire window.
       */
      if (BTK_IS_MENU_BAR (target))
        {
          btk_window_set_mnemonics_visible (BTK_WINDOW (toplevel), FALSE);
          _btk_label_mnemonics_visible_apply_recursively (BTK_WIDGET (target),
                                                          mnemonics_visible);
        }
      else
        btk_window_set_mnemonics_visible (BTK_WINDOW (toplevel), mnemonics_visible);

      if (target->active_menu_item || priv->in_unselectable_item)
        found = TRUE;

      target = BTK_MENU_SHELL (target->parent_menu_shell);
    }
}

static gint
btk_menu_shell_key_press (BtkWidget   *widget,
			  BdkEventKey *event)
{
  BtkMenuShell *menu_shell = BTK_MENU_SHELL (widget);
  BtkMenuShellPrivate *priv = BTK_MENU_SHELL_GET_PRIVATE (menu_shell);
  gboolean enable_mnemonics;

  menu_shell->keyboard_mode = TRUE;

  if (!(menu_shell->active_menu_item || priv->in_unselectable_item) && menu_shell->parent_menu_shell)
    return btk_widget_event (menu_shell->parent_menu_shell, (BdkEvent *)event);

  if (btk_bindings_activate_event (BTK_OBJECT (widget), event))
    return TRUE;

  g_object_get (btk_widget_get_settings (widget),
		"btk-enable-mnemonics", &enable_mnemonics,
		NULL);

  if (enable_mnemonics)
    return btk_menu_shell_activate_mnemonic (menu_shell, event);

  return FALSE;
}

static gint
btk_menu_shell_enter_notify (BtkWidget        *widget,
			     BdkEventCrossing *event)
{
  BtkMenuShell *menu_shell = BTK_MENU_SHELL (widget);

  if (event->mode == BDK_CROSSING_BTK_GRAB ||
      event->mode == BDK_CROSSING_BTK_UNGRAB ||
      event->mode == BDK_CROSSING_STATE_CHANGED)
    return TRUE;

  if (menu_shell->active)
    {
      BtkWidget *menu_item;

      menu_item = btk_get_event_widget ((BdkEvent*) event);

      if (!menu_item)
        return TRUE;

      if (BTK_IS_MENU_ITEM (menu_item) &&
          !_btk_menu_item_is_selectable (menu_item))
        {
          BtkMenuShellPrivate *priv;

          priv = BTK_MENU_SHELL_GET_PRIVATE (menu_shell);
          priv->in_unselectable_item = TRUE;

          return TRUE;
        }

      if (menu_item->parent == widget &&
	  BTK_IS_MENU_ITEM (menu_item))
	{
	  if (menu_shell->ignore_enter)
	    return TRUE;

	  if (event->detail != BDK_NOTIFY_INFERIOR)
            {
	      if (btk_widget_get_state (menu_item) != BTK_STATE_PRELIGHT)
                btk_menu_shell_select_item (menu_shell, menu_item);

              /* If any mouse button is down, and there is a submenu
               * that is not yet visible, activate it. It's sufficient
               * to check for any button's mask (not only the one
               * matching menu_shell->button), because there is no
               * situation a mouse button could be pressed while
               * entering a menu item where we wouldn't want to show
               * its submenu.
               */
              if ((event->state & (BDK_BUTTON1_MASK | BDK_BUTTON2_MASK | BDK_BUTTON3_MASK)) &&
                  BTK_MENU_ITEM (menu_item)->submenu != NULL)
                {
                  BtkMenuShellPrivate *priv;

                  priv = BTK_MENU_SHELL_GET_PRIVATE (menu_item->parent);
                  priv->activated_submenu = TRUE;

                  if (!btk_widget_get_visible (BTK_MENU_ITEM (menu_item)->submenu))
                    {
                      gboolean touchscreen_mode;

                      g_object_get (btk_widget_get_settings (widget),
                                    "btk-touchscreen-mode", &touchscreen_mode,
                                    NULL);

                      if (touchscreen_mode)
                        _btk_menu_item_popup_submenu (menu_item, TRUE);
                    }
                }
	    }
	}
      else if (menu_shell->parent_menu_shell)
	{
	  btk_widget_event (menu_shell->parent_menu_shell, (BdkEvent*) event);
	}
    }

  return TRUE;
}

static gint
btk_menu_shell_leave_notify (BtkWidget        *widget,
			     BdkEventCrossing *event)
{
  if (event->mode == BDK_CROSSING_BTK_GRAB ||
      event->mode == BDK_CROSSING_BTK_GRAB ||
      event->mode == BDK_CROSSING_STATE_CHANGED)
    return TRUE;

  if (btk_widget_get_visible (widget))
    {
      BtkMenuShell *menu_shell = BTK_MENU_SHELL (widget);
      BtkWidget *event_widget = btk_get_event_widget ((BdkEvent*) event);
      BtkMenuItem *menu_item;

      if (!event_widget || !BTK_IS_MENU_ITEM (event_widget))
	return TRUE;

      menu_item = BTK_MENU_ITEM (event_widget);

      if (!_btk_menu_item_is_selectable (event_widget))
        {
          BtkMenuShellPrivate *priv;

          priv = BTK_MENU_SHELL_GET_PRIVATE (menu_shell);
          priv->in_unselectable_item = TRUE;

          return TRUE;
        }

      if ((menu_shell->active_menu_item == event_widget) &&
	  (menu_item->submenu == NULL))
	{
	  if ((event->detail != BDK_NOTIFY_INFERIOR) &&
	      (btk_widget_get_state (BTK_WIDGET (menu_item)) != BTK_STATE_NORMAL))
	    {
	      btk_menu_shell_deselect (menu_shell);
	    }
	}
      else if (menu_shell->parent_menu_shell)
	{
	  btk_widget_event (menu_shell->parent_menu_shell, (BdkEvent*) event);
	}
    }

  return TRUE;
}

static void
btk_menu_shell_screen_changed (BtkWidget *widget,
			       BdkScreen *previous_screen)
{
  btk_menu_shell_reset_key_hash (BTK_MENU_SHELL (widget));
}

static void
btk_menu_shell_add (BtkContainer *container,
		    BtkWidget    *widget)
{
  btk_menu_shell_append (BTK_MENU_SHELL (container), widget);
}

static void
btk_menu_shell_remove (BtkContainer *container,
		       BtkWidget    *widget)
{
  BtkMenuShell *menu_shell = BTK_MENU_SHELL (container);
  gint was_visible;

  was_visible = btk_widget_get_visible (widget);
  menu_shell->children = g_list_remove (menu_shell->children, widget);
  
  if (widget == menu_shell->active_menu_item)
    {
      btk_item_deselect (BTK_ITEM (menu_shell->active_menu_item));
      menu_shell->active_menu_item = NULL;
    }

  btk_widget_unparent (widget);
  
  /* queue resize regardless of btk_widget_get_visible (container),
   * since that's what is needed by toplevels.
   */
  if (was_visible)
    btk_widget_queue_resize (BTK_WIDGET (container));
}

static void
btk_menu_shell_forall (BtkContainer *container,
		       gboolean      include_internals,
		       BtkCallback   callback,
		       gpointer      callback_data)
{
  BtkMenuShell *menu_shell = BTK_MENU_SHELL (container);
  BtkWidget *child;
  GList *children;

  children = menu_shell->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      (* callback) (child, callback_data);
    }
}


static void
btk_real_menu_shell_deactivate (BtkMenuShell *menu_shell)
{
  if (menu_shell->active)
    {
      menu_shell->button = 0;
      menu_shell->active = FALSE;
      menu_shell->activate_time = 0;

      if (menu_shell->active_menu_item)
	{
	  btk_menu_item_deselect (BTK_MENU_ITEM (menu_shell->active_menu_item));
	  menu_shell->active_menu_item = NULL;
	}

      if (menu_shell->have_grab)
	{
	  menu_shell->have_grab = FALSE;
	  btk_grab_remove (BTK_WIDGET (menu_shell));
	}
      if (menu_shell->have_xgrab)
	{
	  BdkDisplay *display = btk_widget_get_display (BTK_WIDGET (menu_shell));

	  menu_shell->have_xgrab = FALSE;
	  bdk_display_pointer_ungrab (display, BDK_CURRENT_TIME);
	  bdk_display_keyboard_ungrab (display, BDK_CURRENT_TIME);
	}

      menu_shell->keyboard_mode = FALSE;

      _btk_menu_shell_update_mnemonics (menu_shell);
    }
}

static gint
btk_menu_shell_is_item (BtkMenuShell *menu_shell,
			BtkWidget    *child)
{
  BtkWidget *parent;

  g_return_val_if_fail (BTK_IS_MENU_SHELL (menu_shell), FALSE);
  g_return_val_if_fail (child != NULL, FALSE);

  parent = child->parent;
  while (BTK_IS_MENU_SHELL (parent))
    {
      if (parent == (BtkWidget*) menu_shell)
	return TRUE;
      parent = BTK_MENU_SHELL (parent)->parent_menu_shell;
    }

  return FALSE;
}

static BtkWidget*
btk_menu_shell_get_item (BtkMenuShell *menu_shell,
			 BdkEvent     *event)
{
  BtkWidget *menu_item;

  menu_item = btk_get_event_widget ((BdkEvent*) event);
  
  while (menu_item && !BTK_IS_MENU_ITEM (menu_item))
    menu_item = menu_item->parent;

  if (menu_item && btk_menu_shell_is_item (menu_shell, menu_item))
    return menu_item;
  else
    return NULL;
}

/* Handlers for action signals */

void
btk_menu_shell_select_item (BtkMenuShell *menu_shell,
			    BtkWidget    *menu_item)
{
  BtkMenuShellClass *class;

  g_return_if_fail (BTK_IS_MENU_SHELL (menu_shell));
  g_return_if_fail (BTK_IS_MENU_ITEM (menu_item));

  class = BTK_MENU_SHELL_GET_CLASS (menu_shell);

  if (class->select_item &&
      !(menu_shell->active &&
	menu_shell->active_menu_item == menu_item))
    class->select_item (menu_shell, menu_item);
}

void _btk_menu_item_set_placement (BtkMenuItem         *menu_item,
				   BtkSubmenuPlacement  placement);

static void
btk_menu_shell_real_select_item (BtkMenuShell *menu_shell,
				 BtkWidget    *menu_item)
{
  BtkPackDirection pack_dir = PACK_DIRECTION (menu_shell);

  if (menu_shell->active_menu_item)
    {
      btk_menu_item_deselect (BTK_MENU_ITEM (menu_shell->active_menu_item));
      menu_shell->active_menu_item = NULL;
    }

  if (!_btk_menu_item_is_selectable (menu_item))
    {
      BtkMenuShellPrivate *priv = BTK_MENU_SHELL_GET_PRIVATE (menu_shell);

      priv->in_unselectable_item = TRUE;
      _btk_menu_shell_update_mnemonics (menu_shell);

      return;
    }

  btk_menu_shell_activate (menu_shell);

  menu_shell->active_menu_item = menu_item;
  if (pack_dir == BTK_PACK_DIRECTION_TTB || pack_dir == BTK_PACK_DIRECTION_BTT)
    _btk_menu_item_set_placement (BTK_MENU_ITEM (menu_shell->active_menu_item),
				  BTK_LEFT_RIGHT);
  else
    _btk_menu_item_set_placement (BTK_MENU_ITEM (menu_shell->active_menu_item),
				  BTK_MENU_SHELL_GET_CLASS (menu_shell)->submenu_placement);
  btk_menu_item_select (BTK_MENU_ITEM (menu_shell->active_menu_item));

  _btk_menu_shell_update_mnemonics (menu_shell);

  /* This allows the bizarre radio buttons-with-submenus-display-history
   * behavior
   */
  if (BTK_MENU_ITEM (menu_shell->active_menu_item)->submenu)
    btk_widget_activate (menu_shell->active_menu_item);
}

void
btk_menu_shell_deselect (BtkMenuShell *menu_shell)
{
  g_return_if_fail (BTK_IS_MENU_SHELL (menu_shell));

  if (menu_shell->active_menu_item)
    {
      btk_menu_item_deselect (BTK_MENU_ITEM (menu_shell->active_menu_item));
      menu_shell->active_menu_item = NULL;
      _btk_menu_shell_update_mnemonics (menu_shell);
    }
}

void
btk_menu_shell_activate_item (BtkMenuShell      *menu_shell,
			      BtkWidget         *menu_item,
			      gboolean           force_deactivate)
{
  GSList *slist, *shells = NULL;
  gboolean deactivate = force_deactivate;

  g_return_if_fail (BTK_IS_MENU_SHELL (menu_shell));
  g_return_if_fail (BTK_IS_MENU_ITEM (menu_item));

  if (!deactivate)
    deactivate = BTK_MENU_ITEM_GET_CLASS (menu_item)->hide_on_activate;

  g_object_ref (menu_shell);
  g_object_ref (menu_item);

  if (deactivate)
    {
      BtkMenuShell *parent_menu_shell = menu_shell;

      do
	{
	  g_object_ref (parent_menu_shell);
	  shells = b_slist_prepend (shells, parent_menu_shell);
	  parent_menu_shell = (BtkMenuShell*) parent_menu_shell->parent_menu_shell;
	}
      while (parent_menu_shell);
      shells = b_slist_reverse (shells);

      btk_menu_shell_deactivate (menu_shell);
  
      /* flush the x-queue, so any grabs are removed and
       * the menu is actually taken down
       */
      bdk_display_sync (btk_widget_get_display (menu_item));
    }

  btk_widget_activate (menu_item);

  for (slist = shells; slist; slist = slist->next)
    {
      g_signal_emit (slist->data, menu_shell_signals[SELECTION_DONE], 0);
      g_object_unref (slist->data);
    }
  b_slist_free (shells);

  g_object_unref (menu_shell);
  g_object_unref (menu_item);
}

/* Distance should be +/- 1 */
static gboolean
btk_menu_shell_real_move_selected (BtkMenuShell  *menu_shell, 
				   gint           distance)
{
  if (menu_shell->active_menu_item)
    {
      GList *node = g_list_find (menu_shell->children,
				 menu_shell->active_menu_item);
      GList *start_node = node;
      gboolean wrap_around;

      g_object_get (btk_widget_get_settings (BTK_WIDGET (menu_shell)),
                    "btk-keynav-wrap-around", &wrap_around,
                    NULL);

      if (distance > 0)
	{
	  node = node->next;
	  while (node != start_node && 
		 (!node || !_btk_menu_item_is_selectable (node->data)))
	    {
	      if (node)
		node = node->next;
              else if (wrap_around)
		node = menu_shell->children;
              else
                {
                  btk_widget_error_bell (BTK_WIDGET (menu_shell));
                  break;
                }
	    }
	}
      else
	{
	  node = node->prev;
	  while (node != start_node &&
		 (!node || !_btk_menu_item_is_selectable (node->data)))
	    {
	      if (node)
		node = node->prev;
              else if (wrap_around)
		node = g_list_last (menu_shell->children);
              else
                {
                  btk_widget_error_bell (BTK_WIDGET (menu_shell));
                  break;
                }
	    }
	}
      
      if (node)
	btk_menu_shell_select_item (menu_shell, node->data);
    }

  return TRUE;
}

/* Distance should be +/- 1 */
static void
btk_menu_shell_move_selected (BtkMenuShell  *menu_shell, 
			      gint           distance)
{
  gboolean handled = FALSE;

  g_signal_emit (menu_shell, menu_shell_signals[MOVE_SELECTED], 0,
		 distance, &handled);
}

/**
 * btk_menu_shell_select_first:
 * @menu_shell: a #BtkMenuShell
 * @search_sensitive: if %TRUE, search for the first selectable
 *                    menu item, otherwise select nothing if
 *                    the first item isn't sensitive. This
 *                    should be %FALSE if the menu is being
 *                    popped up initially.
 * 
 * Select the first visible or selectable child of the menu shell;
 * don't select tearoff items unless the only item is a tearoff
 * item.
 *
 * Since: 2.2
 **/
void
btk_menu_shell_select_first (BtkMenuShell *menu_shell,
			     gboolean      search_sensitive)
{
  BtkWidget *to_select = NULL;
  GList *tmp_list;

  tmp_list = menu_shell->children;
  while (tmp_list)
    {
      BtkWidget *child = tmp_list->data;
      
      if ((!search_sensitive && btk_widget_get_visible (child)) ||
	  _btk_menu_item_is_selectable (child))
	{
	  to_select = child;
	  if (!BTK_IS_TEAROFF_MENU_ITEM (child))
	    break;
	}
      
      tmp_list = tmp_list->next;
    }

  if (to_select)
    btk_menu_shell_select_item (menu_shell, to_select);
}

void
_btk_menu_shell_select_last (BtkMenuShell *menu_shell,
			     gboolean      search_sensitive)
{
  BtkWidget *to_select = NULL;
  GList *tmp_list;

  tmp_list = g_list_last (menu_shell->children);
  while (tmp_list)
    {
      BtkWidget *child = tmp_list->data;
      
      if ((!search_sensitive && btk_widget_get_visible (child)) ||
	  _btk_menu_item_is_selectable (child))
	{
	  to_select = child;
	  if (!BTK_IS_TEAROFF_MENU_ITEM (child))
	    break;
	}
      
      tmp_list = tmp_list->prev;
    }

  if (to_select)
    btk_menu_shell_select_item (menu_shell, to_select);
}

static gboolean
btk_menu_shell_select_submenu_first (BtkMenuShell     *menu_shell)
{
  BtkMenuItem *menu_item;

  if (menu_shell->active_menu_item == NULL)
    return FALSE;

  menu_item = BTK_MENU_ITEM (menu_shell->active_menu_item); 
  
  if (menu_item->submenu)
    {
      _btk_menu_item_popup_submenu (BTK_WIDGET (menu_item), FALSE);
      btk_menu_shell_select_first (BTK_MENU_SHELL (menu_item->submenu), TRUE);
      if (BTK_MENU_SHELL (menu_item->submenu)->active_menu_item)
	return TRUE;
    }

  return FALSE;
}

static void
btk_real_menu_shell_move_current (BtkMenuShell         *menu_shell,
				  BtkMenuDirectionType  direction)
{
  BtkMenuShellPrivate *priv = BTK_MENU_SHELL_GET_PRIVATE (menu_shell);
  BtkMenuShell *parent_menu_shell = NULL;
  gboolean had_selection;
  gboolean touchscreen_mode;

  priv->in_unselectable_item = FALSE;

  had_selection = menu_shell->active_menu_item != NULL;

  g_object_get (btk_widget_get_settings (BTK_WIDGET (menu_shell)),
                "btk-touchscreen-mode", &touchscreen_mode,
                NULL);

  if (menu_shell->parent_menu_shell)
    parent_menu_shell = BTK_MENU_SHELL (menu_shell->parent_menu_shell);

  switch (direction)
    {
    case BTK_MENU_DIR_PARENT:
      if (touchscreen_mode &&
          menu_shell->active_menu_item &&
          BTK_MENU_ITEM (menu_shell->active_menu_item)->submenu &&
          btk_widget_get_visible (BTK_MENU_ITEM (menu_shell->active_menu_item)->submenu))
        {
          /* if we are on a menu item that has an open submenu but the
           * focus is not in that submenu (e.g. because it's empty or
           * has only insensitive items), close that submenu instead
           * of running into the code below which would close *this*
           * menu.
           */
          _btk_menu_item_popdown_submenu (menu_shell->active_menu_item);
          _btk_menu_shell_update_mnemonics (menu_shell);
        }
      else if (parent_menu_shell)
	{
          if (touchscreen_mode)
            {
              /* close menu when returning from submenu. */
              _btk_menu_item_popdown_submenu (BTK_MENU (menu_shell)->parent_menu_item);
              _btk_menu_shell_update_mnemonics (parent_menu_shell);
              break;
            }

	  if (BTK_MENU_SHELL_GET_CLASS (parent_menu_shell)->submenu_placement ==
              BTK_MENU_SHELL_GET_CLASS (menu_shell)->submenu_placement)
	    btk_menu_shell_deselect (menu_shell);
	  else
	    {
	      if (PACK_DIRECTION (parent_menu_shell) == BTK_PACK_DIRECTION_LTR)
		btk_menu_shell_move_selected (parent_menu_shell, -1);
	      else
		btk_menu_shell_move_selected (parent_menu_shell, 1);
	      btk_menu_shell_select_submenu_first (parent_menu_shell);
	    }
	}
      /* If there is no parent and the submenu is in the opposite direction
       * to the menu, then make the PARENT direction wrap around to
       * the bottom of the submenu.
       */
      else if (menu_shell->active_menu_item &&
	       _btk_menu_item_is_selectable (menu_shell->active_menu_item) &&
	       BTK_MENU_ITEM (menu_shell->active_menu_item)->submenu)
	{
	  BtkMenuShell *submenu = BTK_MENU_SHELL (BTK_MENU_ITEM (menu_shell->active_menu_item)->submenu);

	  if (BTK_MENU_SHELL_GET_CLASS (menu_shell)->submenu_placement !=
	      BTK_MENU_SHELL_GET_CLASS (submenu)->submenu_placement)
	    _btk_menu_shell_select_last (submenu, TRUE);
	}
      break;

    case BTK_MENU_DIR_CHILD:
      if (menu_shell->active_menu_item &&
	  _btk_menu_item_is_selectable (menu_shell->active_menu_item) &&
	  BTK_MENU_ITEM (menu_shell->active_menu_item)->submenu)
	{
	  if (btk_menu_shell_select_submenu_first (menu_shell))
	    break;
	}

      /* Try to find a menu running the opposite direction */
      while (parent_menu_shell &&
	     (BTK_MENU_SHELL_GET_CLASS (parent_menu_shell)->submenu_placement ==
	      BTK_MENU_SHELL_GET_CLASS (menu_shell)->submenu_placement))
	{
	  parent_menu_shell = BTK_MENU_SHELL (parent_menu_shell->parent_menu_shell);
	}

      if (parent_menu_shell)
	{
	  if (PACK_DIRECTION (parent_menu_shell) == BTK_PACK_DIRECTION_LTR)
	    btk_menu_shell_move_selected (parent_menu_shell, 1);
	  else
	    btk_menu_shell_move_selected (parent_menu_shell, -1);

	  btk_menu_shell_select_submenu_first (parent_menu_shell);
	}
      break;

    case BTK_MENU_DIR_PREV:
      btk_menu_shell_move_selected (menu_shell, -1);
      if (!had_selection &&
	  !menu_shell->active_menu_item &&
	  menu_shell->children)
	_btk_menu_shell_select_last (menu_shell, TRUE);
      break;

    case BTK_MENU_DIR_NEXT:
      btk_menu_shell_move_selected (menu_shell, 1);
      if (!had_selection &&
	  !menu_shell->active_menu_item &&
	  menu_shell->children)
	btk_menu_shell_select_first (menu_shell, TRUE);
      break;
    }
}

static void
btk_real_menu_shell_activate_current (BtkMenuShell      *menu_shell,
				      gboolean           force_hide)
{
  if (menu_shell->active_menu_item &&
      _btk_menu_item_is_selectable (menu_shell->active_menu_item))
  {
    if (BTK_MENU_ITEM (menu_shell->active_menu_item)->submenu == NULL)
      btk_menu_shell_activate_item (menu_shell,
				    menu_shell->active_menu_item,
				    force_hide);
    else
      btk_menu_shell_select_submenu_first (menu_shell);
  }
}

static void
btk_real_menu_shell_cancel (BtkMenuShell      *menu_shell)
{
  /* Unset the active menu item so btk_menu_popdown() doesn't see it.
   */
  btk_menu_shell_deselect (menu_shell);
  
  btk_menu_shell_deactivate (menu_shell);
  g_signal_emit (menu_shell, menu_shell_signals[SELECTION_DONE], 0);
}

static void
btk_real_menu_shell_cycle_focus (BtkMenuShell      *menu_shell,
				 BtkDirectionType   dir)
{
  while (menu_shell && !BTK_IS_MENU_BAR (menu_shell))
    {
      if (menu_shell->parent_menu_shell)
	menu_shell = BTK_MENU_SHELL (menu_shell->parent_menu_shell);
      else
	menu_shell = NULL;
    }

  if (menu_shell)
    _btk_menu_bar_cycle_focus (BTK_MENU_BAR (menu_shell), dir);
}

gint
_btk_menu_shell_get_popup_delay (BtkMenuShell *menu_shell)
{
  BtkMenuShellClass *klass = BTK_MENU_SHELL_GET_CLASS (menu_shell);
  
  if (klass->get_popup_delay)
    {
      return klass->get_popup_delay (menu_shell);
    }
  else
    {
      gint popup_delay;
      BtkWidget *widget = BTK_WIDGET (menu_shell);
      
      g_object_get (btk_widget_get_settings (widget),
		    "btk-menu-popup-delay", &popup_delay,
		    NULL);
      
      return popup_delay;
    }
}

/**
 * btk_menu_shell_cancel:
 * @menu_shell: a #BtkMenuShell
 * 
 * Cancels the selection within the menu shell.  
 * 
 * Since: 2.4
 */
void
btk_menu_shell_cancel (BtkMenuShell *menu_shell)
{
  g_return_if_fail (BTK_IS_MENU_SHELL (menu_shell));

  g_signal_emit (menu_shell, menu_shell_signals[CANCEL], 0);
}

static BtkMnemonicHash *
btk_menu_shell_get_mnemonic_hash (BtkMenuShell *menu_shell,
				  gboolean      create)
{
  BtkMenuShellPrivate *private = BTK_MENU_SHELL_GET_PRIVATE (menu_shell);

  if (!private->mnemonic_hash && create)
    private->mnemonic_hash = _btk_mnemonic_hash_new ();
  
  return private->mnemonic_hash;
}

static void
menu_shell_add_mnemonic_foreach (guint    keyval,
				 GSList  *targets,
				 gpointer data)
{
  BtkKeyHash *key_hash = data;

  _btk_key_hash_add_entry (key_hash, keyval, 0, GUINT_TO_POINTER (keyval));
}

static BtkKeyHash *
btk_menu_shell_get_key_hash (BtkMenuShell *menu_shell,
			     gboolean      create)
{
  BtkMenuShellPrivate *private = BTK_MENU_SHELL_GET_PRIVATE (menu_shell);
  BtkWidget *widget = BTK_WIDGET (menu_shell);

  if (!private->key_hash && create && btk_widget_has_screen (widget))
    {
      BtkMnemonicHash *mnemonic_hash = btk_menu_shell_get_mnemonic_hash (menu_shell, FALSE);
      BdkScreen *screen = btk_widget_get_screen (widget);
      BdkKeymap *keymap = bdk_keymap_get_for_display (bdk_screen_get_display (screen));

      if (!mnemonic_hash)
	return NULL;
      
      private->key_hash = _btk_key_hash_new (keymap, NULL);

      _btk_mnemonic_hash_foreach (mnemonic_hash,
				  menu_shell_add_mnemonic_foreach,
				  private->key_hash);
    }
  
  return private->key_hash;
}

static void
btk_menu_shell_reset_key_hash (BtkMenuShell *menu_shell)
{
  BtkMenuShellPrivate *private = BTK_MENU_SHELL_GET_PRIVATE (menu_shell);

  if (private->key_hash)
    {
      _btk_key_hash_free (private->key_hash);
      private->key_hash = NULL;
    }
}

static gboolean
btk_menu_shell_activate_mnemonic (BtkMenuShell *menu_shell,
				  BdkEventKey  *event)
{
  BtkMnemonicHash *mnemonic_hash;
  BtkKeyHash *key_hash;
  GSList *entries;
  gboolean result = FALSE;

  mnemonic_hash = btk_menu_shell_get_mnemonic_hash (menu_shell, FALSE);
  if (!mnemonic_hash)
    return FALSE;

  key_hash = btk_menu_shell_get_key_hash (menu_shell, TRUE);
  if (!key_hash)
    return FALSE;
  
  entries = _btk_key_hash_lookup (key_hash,
				  event->hardware_keycode,
				  event->state,
				  btk_accelerator_get_default_mod_mask (),
				  event->group);

  if (entries)
    result = _btk_mnemonic_hash_activate (mnemonic_hash,
					  GPOINTER_TO_UINT (entries->data));

  return result;
}

void
_btk_menu_shell_add_mnemonic (BtkMenuShell *menu_shell,
			      guint      keyval,
			      BtkWidget *target)
{
  g_return_if_fail (BTK_IS_MENU_SHELL (menu_shell));
  g_return_if_fail (BTK_IS_WIDGET (target));

  _btk_mnemonic_hash_add (btk_menu_shell_get_mnemonic_hash (menu_shell, TRUE),
			  keyval, target);
  btk_menu_shell_reset_key_hash (menu_shell);
}

void
_btk_menu_shell_remove_mnemonic (BtkMenuShell *menu_shell,
				 guint      keyval,
				 BtkWidget *target)
{
  g_return_if_fail (BTK_IS_MENU_SHELL (menu_shell));
  g_return_if_fail (BTK_IS_WIDGET (target));
  
  _btk_mnemonic_hash_remove (btk_menu_shell_get_mnemonic_hash (menu_shell, TRUE),
			     keyval, target);
  btk_menu_shell_reset_key_hash (menu_shell);
}

/**
 * btk_menu_shell_get_take_focus:
 * @menu_shell: a #BtkMenuShell
 *
 * Returns %TRUE if the menu shell will take the keyboard focus on popup.
 *
 * Returns: %TRUE if the menu shell will take the keyboard focus on popup.
 *
 * Since: 2.8
 **/
gboolean
btk_menu_shell_get_take_focus (BtkMenuShell *menu_shell)
{
  BtkMenuShellPrivate *priv;

  g_return_val_if_fail (BTK_IS_MENU_SHELL (menu_shell), FALSE);

  priv = BTK_MENU_SHELL_GET_PRIVATE (menu_shell);

  return priv->take_focus;
}

/**
 * btk_menu_shell_set_take_focus:
 * @menu_shell: a #BtkMenuShell
 * @take_focus: %TRUE if the menu shell should take the keyboard focus on popup.
 *
 * If @take_focus is %TRUE (the default) the menu shell will take the keyboard 
 * focus so that it will receive all keyboard events which is needed to enable
 * keyboard navigation in menus.
 *
 * Setting @take_focus to %FALSE is useful only for special applications
 * like virtual keyboard implementations which should not take keyboard
 * focus.
 *
 * The @take_focus state of a menu or menu bar is automatically propagated
 * to submenus whenever a submenu is popped up, so you don't have to worry
 * about recursively setting it for your entire menu hierarchy. Only when
 * programmatically picking a submenu and popping it up manually, the
 * @take_focus property of the submenu needs to be set explicitely.
 *
 * Note that setting it to %FALSE has side-effects:
 *
 * If the focus is in some other app, it keeps the focus and keynav in
 * the menu doesn't work. Consequently, keynav on the menu will only
 * work if the focus is on some toplevel owned by the onscreen keyboard.
 *
 * To avoid confusing the user, menus with @take_focus set to %FALSE
 * should not display mnemonics or accelerators, since it cannot be
 * guaranteed that they will work.
 *
 * See also bdk_keyboard_grab()
 *
 * Since: 2.8
 **/
void
btk_menu_shell_set_take_focus (BtkMenuShell *menu_shell,
                               gboolean      take_focus)
{
  BtkMenuShellPrivate *priv;

  g_return_if_fail (BTK_IS_MENU_SHELL (menu_shell));

  priv = BTK_MENU_SHELL_GET_PRIVATE (menu_shell);

  if (priv->take_focus != take_focus)
    {
      priv->take_focus = take_focus;
      g_object_notify (B_OBJECT (menu_shell), "take-focus");
    }
}

#define __BTK_MENU_SHELL_C__
#include "btkaliasdef.c"
