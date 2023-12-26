/* BTK - The GIMP Toolkit
 *
 * Copyright (C) 2003 Ricardo Fernandez Pascual
 * Copyright (C) 2004 Paolo Borelli
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

#undef BTK_DISABLE_DEPRECATED /* BtkTooltips */

#include "btkmenutoolbutton.h"
#include "btktogglebutton.h"
#include "btkarrow.h"
#include "btkhbox.h"
#include "btkvbox.h"
#include "btkmenu.h"
#include "btkmain.h"
#include "btkbuildable.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"


#define BTK_MENU_TOOL_BUTTON_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), BTK_TYPE_MENU_TOOL_BUTTON, BtkMenuToolButtonPrivate))

struct _BtkMenuToolButtonPrivate
{
  BtkWidget *button;
  BtkWidget *arrow;
  BtkWidget *arrow_button;
  BtkWidget *box;
  BtkMenu   *menu;
};

static void btk_menu_tool_button_destroy    (BtkObject              *object);

static int  menu_deactivate_cb              (BtkMenuShell           *menu_shell,
					     BtkMenuToolButton      *button);

static void btk_menu_tool_button_buildable_interface_init (BtkBuildableIface   *iface);
static void btk_menu_tool_button_buildable_add_child      (BtkBuildable        *buildable,
							   BtkBuilder          *builder,
							   GObject             *child,
							   const gchar         *type);

enum
{
  SHOW_MENU,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_MENU
};

static gint signals[LAST_SIGNAL];

static BtkBuildableIface *parent_buildable_iface;

G_DEFINE_TYPE_WITH_CODE (BtkMenuToolButton, btk_menu_tool_button, BTK_TYPE_TOOL_BUTTON,
                         G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
                                                btk_menu_tool_button_buildable_interface_init))

static void
btk_menu_tool_button_construct_contents (BtkMenuToolButton *button)
{
  BtkMenuToolButtonPrivate *priv = button->priv;
  BtkWidget *box;
  BtkOrientation orientation;

  orientation = btk_tool_item_get_orientation (BTK_TOOL_ITEM (button));

  if (orientation == BTK_ORIENTATION_HORIZONTAL)
    {
      box = btk_hbox_new (FALSE, 0);
      btk_arrow_set (BTK_ARROW (priv->arrow), BTK_ARROW_DOWN, BTK_SHADOW_NONE);
    }
  else
    {
      box = btk_vbox_new (FALSE, 0);
      btk_arrow_set (BTK_ARROW (priv->arrow), BTK_ARROW_RIGHT, BTK_SHADOW_NONE);
    }

  if (priv->button && priv->button->parent)
    {
      g_object_ref (priv->button);
      btk_container_remove (BTK_CONTAINER (priv->button->parent),
                            priv->button);
      btk_container_add (BTK_CONTAINER (box), priv->button);
      g_object_unref (priv->button);
    }

  if (priv->arrow_button && priv->arrow_button->parent)
    {
      g_object_ref (priv->arrow_button);
      btk_container_remove (BTK_CONTAINER (priv->arrow_button->parent),
                            priv->arrow_button);
      btk_box_pack_end (BTK_BOX (box), priv->arrow_button,
                        FALSE, FALSE, 0);
      g_object_unref (priv->arrow_button);
    }

  if (priv->box)
    {
      gchar *tmp;

      /* Transfer a possible tooltip to the new box */
      g_object_get (priv->box, "tooltip-markup", &tmp, NULL);

      if (tmp)
        {
	  g_object_set (box, "tooltip-markup", tmp, NULL);
	  g_free (tmp);
	}

      /* Note: we are not destroying the button and the arrow_button
       * here because they were removed from their container above
       */
      btk_widget_destroy (priv->box);
    }

  priv->box = box;

  btk_container_add (BTK_CONTAINER (button), priv->box);
  btk_widget_show_all (priv->box);

  btk_button_set_relief (BTK_BUTTON (priv->arrow_button),
			 btk_tool_item_get_relief_style (BTK_TOOL_ITEM (button)));
  
  btk_widget_queue_resize (BTK_WIDGET (button));
}

static void
btk_menu_tool_button_toolbar_reconfigured (BtkToolItem *toolitem)
{
  btk_menu_tool_button_construct_contents (BTK_MENU_TOOL_BUTTON (toolitem));

  /* chain up */
  BTK_TOOL_ITEM_CLASS (btk_menu_tool_button_parent_class)->toolbar_reconfigured (toolitem);
}

static void
btk_menu_tool_button_state_changed (BtkWidget    *widget,
				    BtkStateType  previous_state)
{
  BtkMenuToolButton *button = BTK_MENU_TOOL_BUTTON (widget);
  BtkMenuToolButtonPrivate *priv = button->priv;

  if (!btk_widget_is_sensitive (widget) && priv->menu)
    {
      btk_menu_shell_deactivate (BTK_MENU_SHELL (priv->menu));
    }
}

static void
btk_menu_tool_button_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  BtkMenuToolButton *button = BTK_MENU_TOOL_BUTTON (object);

  switch (prop_id)
    {
    case PROP_MENU:
      btk_menu_tool_button_set_menu (button, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_menu_tool_button_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  BtkMenuToolButton *button = BTK_MENU_TOOL_BUTTON (object);

  switch (prop_id)
    {
    case PROP_MENU:
      g_value_set_object (value, button->priv->menu);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_menu_tool_button_class_init (BtkMenuToolButtonClass *klass)
{
  GObjectClass *object_class;
  BtkObjectClass *btk_object_class;
  BtkWidgetClass *widget_class;
  BtkToolItemClass *toolitem_class;

  object_class = (GObjectClass *)klass;
  btk_object_class = (BtkObjectClass *)klass;
  widget_class = (BtkWidgetClass *)klass;
  toolitem_class = (BtkToolItemClass *)klass;

  object_class->set_property = btk_menu_tool_button_set_property;
  object_class->get_property = btk_menu_tool_button_get_property;
  btk_object_class->destroy = btk_menu_tool_button_destroy;
  widget_class->state_changed = btk_menu_tool_button_state_changed;
  toolitem_class->toolbar_reconfigured = btk_menu_tool_button_toolbar_reconfigured;

  /**
   * BtkMenuToolButton::show-menu:
   * @button: the object on which the signal is emitted
   *
   * The ::show-menu signal is emitted before the menu is shown.
   *
   * It can be used to populate the menu on demand, using 
   * btk_menu_tool_button_get_menu(). 

   * Note that even if you populate the menu dynamically in this way, 
   * you must set an empty menu on the #BtkMenuToolButton beforehand,
   * since the arrow is made insensitive if the menu is not set.
   */
  signals[SHOW_MENU] =
    g_signal_new (I_("show-menu"),
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BtkMenuToolButtonClass, show_menu),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  g_object_class_install_property (object_class,
                                   PROP_MENU,
                                   g_param_spec_object ("menu",
                                                        P_("Menu"),
                                                        P_("The dropdown menu"),
                                                        BTK_TYPE_MENU,
                                                        BTK_PARAM_READWRITE));

  g_type_class_add_private (object_class, sizeof (BtkMenuToolButtonPrivate));
}

static void
menu_position_func (BtkMenu           *menu,
                    int               *x,
                    int               *y,
                    gboolean          *push_in,
                    BtkMenuToolButton *button)
{
  BtkMenuToolButtonPrivate *priv = button->priv;
  BtkWidget *widget = BTK_WIDGET (button);
  BtkRequisition req;
  BtkRequisition menu_req;
  BtkOrientation orientation;
  BtkTextDirection direction;
  BdkRectangle monitor;
  gint monitor_num;
  BdkScreen *screen;

  btk_widget_size_request (BTK_WIDGET (priv->menu), &menu_req);

  orientation = btk_tool_item_get_orientation (BTK_TOOL_ITEM (button));
  direction = btk_widget_get_direction (widget);

  screen = btk_widget_get_screen (BTK_WIDGET (menu));
  monitor_num = bdk_screen_get_monitor_at_window (screen, widget->window);
  if (monitor_num < 0)
    monitor_num = 0;
  bdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

  if (orientation == BTK_ORIENTATION_HORIZONTAL)
    {
      bdk_window_get_origin (widget->window, x, y);
      *x += widget->allocation.x;
      *y += widget->allocation.y;

      if (direction == BTK_TEXT_DIR_LTR)
	*x += MAX (widget->allocation.width - menu_req.width, 0);
      else if (menu_req.width > widget->allocation.width)
        *x -= menu_req.width - widget->allocation.width;

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
      bdk_window_get_origin (BTK_BUTTON (priv->arrow_button)->event_window, x, y);
      btk_widget_size_request (priv->arrow_button, &req);

      if (direction == BTK_TEXT_DIR_LTR)
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
popup_menu_under_arrow (BtkMenuToolButton *button,
                        BdkEventButton    *event)
{
  BtkMenuToolButtonPrivate *priv = button->priv;

  g_signal_emit (button, signals[SHOW_MENU], 0);

  if (!priv->menu)
    return;

  btk_menu_popup (priv->menu, NULL, NULL,
                  (BtkMenuPositionFunc) menu_position_func,
                  button,
                  event ? event->button : 0,
                  event ? event->time : btk_get_current_event_time ());
}

static void
arrow_button_toggled_cb (BtkToggleButton   *togglebutton,
                         BtkMenuToolButton *button)
{
  BtkMenuToolButtonPrivate *priv = button->priv;

  if (!priv->menu)
    return;

  if (btk_toggle_button_get_active (togglebutton) &&
      !btk_widget_get_visible (BTK_WIDGET (priv->menu)))
    {
      /* we get here only when the menu is activated by a key
       * press, so that we can select the first menu item */
      popup_menu_under_arrow (button, NULL);
      btk_menu_shell_select_first (BTK_MENU_SHELL (priv->menu), FALSE);
    }
}

static gboolean
arrow_button_button_press_event_cb (BtkWidget         *widget,
                                    BdkEventButton    *event,
                                    BtkMenuToolButton *button)
{
  if (event->button == 1)
    {
      popup_menu_under_arrow (button, event);
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (widget), TRUE);

      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

static void
btk_menu_tool_button_init (BtkMenuToolButton *button)
{
  BtkWidget *box;
  BtkWidget *arrow;
  BtkWidget *arrow_button;
  BtkWidget *real_button;

  button->priv = BTK_MENU_TOOL_BUTTON_GET_PRIVATE (button);

  btk_tool_item_set_homogeneous (BTK_TOOL_ITEM (button), FALSE);

  box = btk_hbox_new (FALSE, 0);

  real_button = BTK_BIN (button)->child;
  g_object_ref (real_button);
  btk_container_remove (BTK_CONTAINER (button), real_button);
  btk_container_add (BTK_CONTAINER (box), real_button);
  g_object_unref (real_button);

  arrow_button = btk_toggle_button_new ();
  arrow = btk_arrow_new (BTK_ARROW_DOWN, BTK_SHADOW_NONE);
  btk_container_add (BTK_CONTAINER (arrow_button), arrow);
  btk_box_pack_end (BTK_BOX (box), arrow_button,
                    FALSE, FALSE, 0);

  /* the arrow button is insentive until we set a menu */
  btk_widget_set_sensitive (arrow_button, FALSE);

  btk_widget_show_all (box);

  btk_container_add (BTK_CONTAINER (button), box);

  button->priv->button = real_button;
  button->priv->arrow = arrow;
  button->priv->arrow_button = arrow_button;
  button->priv->box = box;

  g_signal_connect (arrow_button, "toggled",
		    G_CALLBACK (arrow_button_toggled_cb), button);
  g_signal_connect (arrow_button, "button-press-event",
		    G_CALLBACK (arrow_button_button_press_event_cb), button);
}

static void
btk_menu_tool_button_destroy (BtkObject *object)
{
  BtkMenuToolButton *button;

  button = BTK_MENU_TOOL_BUTTON (object);

  if (button->priv->menu)
    {
      g_signal_handlers_disconnect_by_func (button->priv->menu, 
					    menu_deactivate_cb, 
					    button);
      btk_menu_detach (button->priv->menu);

      g_signal_handlers_disconnect_by_func (button->priv->arrow_button,
					    arrow_button_toggled_cb, 
					    button);
      g_signal_handlers_disconnect_by_func (button->priv->arrow_button, 
					    arrow_button_button_press_event_cb, 
					    button);
    }

  BTK_OBJECT_CLASS (btk_menu_tool_button_parent_class)->destroy (object);
}

static void
btk_menu_tool_button_buildable_add_child (BtkBuildable *buildable,
					  BtkBuilder   *builder,
					  GObject      *child,
					  const gchar  *type)
{
  if (type && strcmp (type, "menu") == 0)
    btk_menu_tool_button_set_menu (BTK_MENU_TOOL_BUTTON (buildable),
                                   BTK_WIDGET (child));
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
btk_menu_tool_button_buildable_interface_init (BtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = btk_menu_tool_button_buildable_add_child;
}

/**
 * btk_menu_tool_button_new:
 * @icon_widget: (allow-none): a widget that will be used as icon widget, or %NULL
 * @label: (allow-none): a string that will be used as label, or %NULL
 *
 * Creates a new #BtkMenuToolButton using @icon_widget as icon and
 * @label as label.
 *
 * Return value: the new #BtkMenuToolButton
 *
 * Since: 2.6
 **/
BtkToolItem *
btk_menu_tool_button_new (BtkWidget   *icon_widget,
                          const gchar *label)
{
  BtkMenuToolButton *button;

  button = g_object_new (BTK_TYPE_MENU_TOOL_BUTTON, NULL);

  if (label)
    btk_tool_button_set_label (BTK_TOOL_BUTTON (button), label);

  if (icon_widget)
    btk_tool_button_set_icon_widget (BTK_TOOL_BUTTON (button), icon_widget);

  return BTK_TOOL_ITEM (button);
}

/**
 * btk_menu_tool_button_new_from_stock:
 * @stock_id: the name of a stock item
 *
 * Creates a new #BtkMenuToolButton.
 * The new #BtkMenuToolButton will contain an icon and label from
 * the stock item indicated by @stock_id.
 *
 * Return value: the new #BtkMenuToolButton
 *
 * Since: 2.6
 **/
BtkToolItem *
btk_menu_tool_button_new_from_stock (const gchar *stock_id)
{
  BtkMenuToolButton *button;

  g_return_val_if_fail (stock_id != NULL, NULL);

  button = g_object_new (BTK_TYPE_MENU_TOOL_BUTTON,
			 "stock-id", stock_id,
			 NULL);

  return BTK_TOOL_ITEM (button);
}

/* Callback for the "deactivate" signal on the pop-up menu.
 * This is used so that we unset the state of the toggle button
 * when the pop-up menu disappears. 
 */
static int
menu_deactivate_cb (BtkMenuShell      *menu_shell,
		    BtkMenuToolButton *button)
{
  BtkMenuToolButtonPrivate *priv = button->priv;

  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (priv->arrow_button), FALSE);

  return TRUE;
}

static void
menu_detacher (BtkWidget *widget,
               BtkMenu   *menu)
{
  BtkMenuToolButtonPrivate *priv = BTK_MENU_TOOL_BUTTON (widget)->priv;

  g_return_if_fail (priv->menu == menu);

  priv->menu = NULL;
}

/**
 * btk_menu_tool_button_set_menu:
 * @button: a #BtkMenuToolButton
 * @menu: the #BtkMenu associated with #BtkMenuToolButton
 *
 * Sets the #BtkMenu that is popped up when the user clicks on the arrow.
 * If @menu is NULL, the arrow button becomes insensitive.
 *
 * Since: 2.6
 **/
void
btk_menu_tool_button_set_menu (BtkMenuToolButton *button,
                               BtkWidget         *menu)
{
  BtkMenuToolButtonPrivate *priv;

  g_return_if_fail (BTK_IS_MENU_TOOL_BUTTON (button));
  g_return_if_fail (BTK_IS_MENU (menu) || menu == NULL);

  priv = button->priv;

  if (priv->menu != BTK_MENU (menu))
    {
      if (priv->menu && btk_widget_get_visible (BTK_WIDGET (priv->menu)))
        btk_menu_shell_deactivate (BTK_MENU_SHELL (priv->menu));

      if (priv->menu)
	{
          g_signal_handlers_disconnect_by_func (priv->menu, 
						menu_deactivate_cb, 
						button);
	  btk_menu_detach (priv->menu);
	}

      priv->menu = BTK_MENU (menu);

      if (priv->menu)
        {
          btk_menu_attach_to_widget (priv->menu, BTK_WIDGET (button),
                                     menu_detacher);

          btk_widget_set_sensitive (priv->arrow_button, TRUE);

          g_signal_connect (priv->menu, "deactivate",
                            G_CALLBACK (menu_deactivate_cb), button);
        }
      else
       btk_widget_set_sensitive (priv->arrow_button, FALSE);
    }

  g_object_notify (G_OBJECT (button), "menu");
}

/**
 * btk_menu_tool_button_get_menu:
 * @button: a #BtkMenuToolButton
 *
 * Gets the #BtkMenu associated with #BtkMenuToolButton.
 *
 * Return value: (transfer none): the #BtkMenu associated
 *     with #BtkMenuToolButton
 *
 * Since: 2.6
 **/
BtkWidget *
btk_menu_tool_button_get_menu (BtkMenuToolButton *button)
{
  g_return_val_if_fail (BTK_IS_MENU_TOOL_BUTTON (button), NULL);

  return BTK_WIDGET (button->priv->menu);
}

/**
 * btk_menu_tool_button_set_arrow_tooltip:
 * @button: a #BtkMenuToolButton
 * @tooltips: the #BtkTooltips object to be used
 * @tip_text: (allow-none): text to be used as tooltip text for tool_item
 * @tip_private: (allow-none): text to be used as private tooltip text
 *
 * Sets the #BtkTooltips object to be used for arrow button which
 * pops up the menu. See btk_tool_item_set_tooltip() for setting
 * a tooltip on the whole #BtkMenuToolButton.
 *
 * Since: 2.6
 *
 * Deprecated: 2.12: Use btk_menu_tool_button_set_arrow_tooltip_text()
 * instead.
 **/
void
btk_menu_tool_button_set_arrow_tooltip (BtkMenuToolButton *button,
                                        BtkTooltips       *tooltips,
                                        const gchar       *tip_text,
                                        const gchar       *tip_private)
{
  g_return_if_fail (BTK_IS_MENU_TOOL_BUTTON (button));

  btk_tooltips_set_tip (tooltips, button->priv->arrow_button, tip_text, tip_private);
}

/**
 * btk_menu_tool_button_set_arrow_tooltip_text:
 * @button: a #BtkMenuToolButton
 * @text: text to be used as tooltip text for button's arrow button
 *
 * Sets the tooltip text to be used as tooltip for the arrow button which
 * pops up the menu.  See btk_tool_item_set_tooltip() for setting a tooltip
 * on the whole #BtkMenuToolButton.
 *
 * Since: 2.12
 **/
void
btk_menu_tool_button_set_arrow_tooltip_text (BtkMenuToolButton *button,
					     const gchar       *text)
{
  g_return_if_fail (BTK_IS_MENU_TOOL_BUTTON (button));

  btk_widget_set_tooltip_text (button->priv->arrow_button, text);
}

/**
 * btk_menu_tool_button_set_arrow_tooltip_markup:
 * @button: a #BtkMenuToolButton
 * @markup: markup text to be used as tooltip text for button's arrow button
 *
 * Sets the tooltip markup text to be used as tooltip for the arrow button
 * which pops up the menu.  See btk_tool_item_set_tooltip() for setting a
 * tooltip on the whole #BtkMenuToolButton.
 *
 * Since: 2.12
 **/
void
btk_menu_tool_button_set_arrow_tooltip_markup (BtkMenuToolButton *button,
					       const gchar       *markup)
{
  g_return_if_fail (BTK_IS_MENU_TOOL_BUTTON (button));

  btk_widget_set_tooltip_markup (button->priv->arrow_button, markup);
}

#define __BTK_MENU_TOOL_BUTTON_C__
#include "btkaliasdef.c"
