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
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkmenubar.h"
#include "btkmenuitem.h"
#include "btksettings.h"
#include "btkintl.h"
#include "btkwindow.h"
#include "btkprivate.h"
#include "btkalias.h"


#define BORDER_SPACING  0
#define DEFAULT_IPADDING 1

/* Properties */
enum {
  PROP_0,
  PROP_PACK_DIRECTION,
  PROP_CHILD_PACK_DIRECTION
};

typedef struct _BtkMenuBarPrivate BtkMenuBarPrivate;
struct _BtkMenuBarPrivate
{
  BtkPackDirection pack_direction;
  BtkPackDirection child_pack_direction;
};

#define BTK_MENU_BAR_GET_PRIVATE(o)  \
  (B_TYPE_INSTANCE_GET_PRIVATE ((o), BTK_TYPE_MENU_BAR, BtkMenuBarPrivate))


static void btk_menu_bar_set_property      (BObject             *object,
					    buint                prop_id,
					    const BValue        *value,
					    BParamSpec          *pspec);
static void btk_menu_bar_get_property      (BObject             *object,
					    buint                prop_id,
					    BValue              *value,
					    BParamSpec          *pspec);
static void btk_menu_bar_size_request      (BtkWidget       *widget,
					    BtkRequisition  *requisition);
static void btk_menu_bar_size_allocate     (BtkWidget       *widget,
					    BtkAllocation   *allocation);
static void btk_menu_bar_paint             (BtkWidget       *widget,
					    BdkRectangle    *area);
static bint btk_menu_bar_expose            (BtkWidget       *widget,
					    BdkEventExpose  *event);
static void btk_menu_bar_hierarchy_changed (BtkWidget       *widget,
					    BtkWidget       *old_toplevel);
static bint btk_menu_bar_get_popup_delay   (BtkMenuShell    *menu_shell);
static void btk_menu_bar_move_current      (BtkMenuShell     *menu_shell,
                                            BtkMenuDirectionType direction);

static BtkShadowType get_shadow_type   (BtkMenuBar      *menubar);

G_DEFINE_TYPE (BtkMenuBar, btk_menu_bar, BTK_TYPE_MENU_SHELL)

static void
btk_menu_bar_class_init (BtkMenuBarClass *class)
{
  BObjectClass *bobject_class;
  BtkWidgetClass *widget_class;
  BtkMenuShellClass *menu_shell_class;

  BtkBindingSet *binding_set;

  bobject_class = (BObjectClass*) class;
  widget_class = (BtkWidgetClass*) class;
  menu_shell_class = (BtkMenuShellClass*) class;

  bobject_class->get_property = btk_menu_bar_get_property;
  bobject_class->set_property = btk_menu_bar_set_property;

  widget_class->size_request = btk_menu_bar_size_request;
  widget_class->size_allocate = btk_menu_bar_size_allocate;
  widget_class->expose_event = btk_menu_bar_expose;
  widget_class->hierarchy_changed = btk_menu_bar_hierarchy_changed;
  
  menu_shell_class->submenu_placement = BTK_TOP_BOTTOM;
  menu_shell_class->get_popup_delay = btk_menu_bar_get_popup_delay;
  menu_shell_class->move_current = btk_menu_bar_move_current;

  binding_set = btk_binding_set_by_class (class);
  btk_binding_entry_add_signal (binding_set,
				BDK_Left, 0,
				"move-current", 1,
				BTK_TYPE_MENU_DIRECTION_TYPE,
				BTK_MENU_DIR_PREV);
  btk_binding_entry_add_signal (binding_set,
				BDK_KP_Left, 0,
				"move-current", 1,
				BTK_TYPE_MENU_DIRECTION_TYPE,
				BTK_MENU_DIR_PREV);
  btk_binding_entry_add_signal (binding_set,
				BDK_Right, 0,
				"move-current", 1,
				BTK_TYPE_MENU_DIRECTION_TYPE,
				BTK_MENU_DIR_NEXT);
  btk_binding_entry_add_signal (binding_set,
				BDK_KP_Right, 0,
				"move-current", 1,
				BTK_TYPE_MENU_DIRECTION_TYPE,
				BTK_MENU_DIR_NEXT);
  btk_binding_entry_add_signal (binding_set,
				BDK_Up, 0,
				"move-current", 1,
				BTK_TYPE_MENU_DIRECTION_TYPE,
				BTK_MENU_DIR_PARENT);
  btk_binding_entry_add_signal (binding_set,
				BDK_KP_Up, 0,
				"move-current", 1,
				BTK_TYPE_MENU_DIRECTION_TYPE,
				BTK_MENU_DIR_PARENT);
  btk_binding_entry_add_signal (binding_set,
				BDK_Down, 0,
				"move-current", 1,
				BTK_TYPE_MENU_DIRECTION_TYPE,
				BTK_MENU_DIR_CHILD);
  btk_binding_entry_add_signal (binding_set,
				BDK_KP_Down, 0,
				"move-current", 1,
				BTK_TYPE_MENU_DIRECTION_TYPE,
				BTK_MENU_DIR_CHILD);

  /**
   * BtkMenuBar:pack-direction:
   *
   * The pack direction of the menubar. It determines how
   * menuitems are arranged in the menubar.
   *
   * Since: 2.8
   */
  g_object_class_install_property (bobject_class,
				   PROP_PACK_DIRECTION,
				   g_param_spec_enum ("pack-direction",
 						      P_("Pack direction"),
 						      P_("The pack direction of the menubar"),
 						      BTK_TYPE_PACK_DIRECTION,
 						      BTK_PACK_DIRECTION_LTR,
 						      BTK_PARAM_READWRITE));
  
  /**
   * BtkMenuBar:child-pack-direction:
   *
   * The child pack direction of the menubar. It determines how
   * the widgets contained in child menuitems are arranged.
   *
   * Since: 2.8
   */
  g_object_class_install_property (bobject_class,
				   PROP_CHILD_PACK_DIRECTION,
				   g_param_spec_enum ("child-pack-direction",
 						      P_("Child Pack direction"),
 						      P_("The child pack direction of the menubar"),
 						      BTK_TYPE_PACK_DIRECTION,
 						      BTK_PACK_DIRECTION_LTR,
 						      BTK_PARAM_READWRITE));
  

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_enum ("shadow-type",
                                                              P_("Shadow type"),
                                                              P_("Style of bevel around the menubar"),
                                                              BTK_TYPE_SHADOW_TYPE,
                                                              BTK_SHADOW_OUT,
                                                              BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("internal-padding",
							     P_("Internal padding"),
							     P_("Amount of border space between the menubar shadow and the menu items"),
							     0,
							     B_MAXINT,
                                                             DEFAULT_IPADDING,
                                                             BTK_PARAM_READABLE));

  g_type_class_add_private (bobject_class, sizeof (BtkMenuBarPrivate));
}

static void
btk_menu_bar_init (BtkMenuBar *object)
{
}

BtkWidget*
btk_menu_bar_new (void)
{
  return g_object_new (BTK_TYPE_MENU_BAR, NULL);
}

static void
btk_menu_bar_set_property (BObject      *object,
			   buint         prop_id,
			   const BValue *value,
			   BParamSpec   *pspec)
{
  BtkMenuBar *menubar = BTK_MENU_BAR (object);
  
  switch (prop_id)
    {
    case PROP_PACK_DIRECTION:
      btk_menu_bar_set_pack_direction (menubar, b_value_get_enum (value));
      break;
    case PROP_CHILD_PACK_DIRECTION:
      btk_menu_bar_set_child_pack_direction (menubar, b_value_get_enum (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_menu_bar_get_property (BObject    *object,
			   buint       prop_id,
			   BValue     *value,
			   BParamSpec *pspec)
{
  BtkMenuBar *menubar = BTK_MENU_BAR (object);
  
  switch (prop_id)
    {
    case PROP_PACK_DIRECTION:
      b_value_set_enum (value, btk_menu_bar_get_pack_direction (menubar));
      break;
    case PROP_CHILD_PACK_DIRECTION:
      b_value_set_enum (value, btk_menu_bar_get_child_pack_direction (menubar));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_menu_bar_size_request (BtkWidget      *widget,
			   BtkRequisition *requisition)
{
  BtkMenuBar *menu_bar;
  BtkMenuBarPrivate *priv;
  BtkMenuShell *menu_shell;
  BtkWidget *child;
  GList *children;
  bint nchildren;
  BtkRequisition child_requisition;
  bint ipadding;

  g_return_if_fail (BTK_IS_MENU_BAR (widget));
  g_return_if_fail (requisition != NULL);

  requisition->width = 0;
  requisition->height = 0;
  
  if (btk_widget_get_visible (widget))
    {
      menu_bar = BTK_MENU_BAR (widget);
      menu_shell = BTK_MENU_SHELL (widget);
      priv = BTK_MENU_BAR_GET_PRIVATE (menu_bar);

      nchildren = 0;
      children = menu_shell->children;

      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if (btk_widget_get_visible (child))
	    {
              bint toggle_size;

	      BTK_MENU_ITEM (child)->show_submenu_indicator = FALSE;
	      btk_widget_size_request (child, &child_requisition);
              btk_menu_item_toggle_size_request (BTK_MENU_ITEM (child),
                                                 &toggle_size);

	      if (priv->child_pack_direction == BTK_PACK_DIRECTION_LTR ||
		  priv->child_pack_direction == BTK_PACK_DIRECTION_RTL)
		child_requisition.width += toggle_size;
	      else
		child_requisition.height += toggle_size;

              if (priv->pack_direction == BTK_PACK_DIRECTION_LTR ||
		  priv->pack_direction == BTK_PACK_DIRECTION_RTL)
		{
		  requisition->width += child_requisition.width;
		  requisition->height = MAX (requisition->height, child_requisition.height);
		}
	      else
		{
		  requisition->width = MAX (requisition->width, child_requisition.width);
		  requisition->height += child_requisition.height;
		}
	      nchildren += 1;
	    }
	}

      btk_widget_style_get (widget, "internal-padding", &ipadding, NULL);
      
      requisition->width += (BTK_CONTAINER (menu_bar)->border_width +
                             ipadding + 
			     BORDER_SPACING) * 2;
      requisition->height += (BTK_CONTAINER (menu_bar)->border_width +
                              ipadding +
			      BORDER_SPACING) * 2;

      if (get_shadow_type (menu_bar) != BTK_SHADOW_NONE)
	{
	  requisition->width += widget->style->xthickness * 2;
	  requisition->height += widget->style->ythickness * 2;
	}
    }
}

static void
btk_menu_bar_size_allocate (BtkWidget     *widget,
			    BtkAllocation *allocation)
{
  BtkMenuBar *menu_bar;
  BtkMenuShell *menu_shell;
  BtkMenuBarPrivate *priv;
  BtkWidget *child;
  GList *children;
  BtkAllocation child_allocation;
  BtkRequisition child_requisition;
  buint offset;
  BtkTextDirection direction;
  bint ltr_x, ltr_y;
  bint ipadding;

  g_return_if_fail (BTK_IS_MENU_BAR (widget));
  g_return_if_fail (allocation != NULL);

  menu_bar = BTK_MENU_BAR (widget);
  menu_shell = BTK_MENU_SHELL (widget);
  priv = BTK_MENU_BAR_GET_PRIVATE (menu_bar);

  direction = btk_widget_get_direction (widget);

  widget->allocation = *allocation;
  if (btk_widget_get_realized (widget))
    bdk_window_move_resize (widget->window,
			    allocation->x, allocation->y,
			    allocation->width, allocation->height);

  btk_widget_style_get (widget, "internal-padding", &ipadding, NULL);
  
  if (menu_shell->children)
    {
      child_allocation.x = (BTK_CONTAINER (menu_bar)->border_width +
			    ipadding + 
			    BORDER_SPACING);
      child_allocation.y = (BTK_CONTAINER (menu_bar)->border_width +
			    BORDER_SPACING);
      
      if (get_shadow_type (menu_bar) != BTK_SHADOW_NONE)
	{
	  child_allocation.x += widget->style->xthickness;
	  child_allocation.y += widget->style->ythickness;
	}
      
      if (priv->pack_direction == BTK_PACK_DIRECTION_LTR ||
	  priv->pack_direction == BTK_PACK_DIRECTION_RTL)
	{
	  child_allocation.height = MAX (1, (bint)allocation->height - child_allocation.y * 2);
	  
	  offset = child_allocation.x; 	/* Window edge to menubar start */
	  ltr_x = child_allocation.x;
	  
	  children = menu_shell->children;
	  while (children)
	    {
	      bint toggle_size;          
	      
	      child = children->data;
	      children = children->next;
	      
	      btk_menu_item_toggle_size_request (BTK_MENU_ITEM (child),
						 &toggle_size);
	      btk_widget_get_child_requisition (child, &child_requisition);
	    
	      if (priv->child_pack_direction == BTK_PACK_DIRECTION_LTR ||
		  priv->child_pack_direction == BTK_PACK_DIRECTION_RTL)
		child_requisition.width += toggle_size;
	      else
		child_requisition.height += toggle_size;
	      
	      /* Support for the right justified help menu */
	      if ((children == NULL) && (BTK_IS_MENU_ITEM(child))
		  && (BTK_MENU_ITEM(child)->right_justify)) 
		{
		  ltr_x = allocation->width -
		    child_requisition.width - offset;
		}
	      if (btk_widget_get_visible (child))
		{
		  if ((direction == BTK_TEXT_DIR_LTR) == (priv->pack_direction == BTK_PACK_DIRECTION_LTR))
		    child_allocation.x = ltr_x;
		  else
		    child_allocation.x = allocation->width -
		      child_requisition.width - ltr_x; 
		  
		  child_allocation.width = child_requisition.width;
		  
		  btk_menu_item_toggle_size_allocate (BTK_MENU_ITEM (child),
						      toggle_size);
		  btk_widget_size_allocate (child, &child_allocation);
		  
		  ltr_x += child_allocation.width;
		}
	    }
	}
      else
	{
	  child_allocation.width = MAX (1, (bint)allocation->width - child_allocation.x * 2);
	  
	  offset = child_allocation.y; 	/* Window edge to menubar start */
	  ltr_y = child_allocation.y;
	  
	  children = menu_shell->children;
	  while (children)
	    {
	      bint toggle_size;          
	      
	      child = children->data;
	      children = children->next;
	      
	      btk_menu_item_toggle_size_request (BTK_MENU_ITEM (child),
						 &toggle_size);
	      btk_widget_get_child_requisition (child, &child_requisition);
	      
	      if (priv->child_pack_direction == BTK_PACK_DIRECTION_LTR ||
		  priv->child_pack_direction == BTK_PACK_DIRECTION_RTL)
		child_requisition.width += toggle_size;
	      else
		child_requisition.height += toggle_size;
	      
	      /* Support for the right justified help menu */
	      if ((children == NULL) && (BTK_IS_MENU_ITEM(child))
		  && (BTK_MENU_ITEM(child)->right_justify)) 
		{
		  ltr_y = allocation->height -
		    child_requisition.height - offset;
		}
	      if (btk_widget_get_visible (child))
		{
		  if ((direction == BTK_TEXT_DIR_LTR) ==
		      (priv->pack_direction == BTK_PACK_DIRECTION_TTB))
		    child_allocation.y = ltr_y;
		  else
		    child_allocation.y = allocation->height -
		      child_requisition.height - ltr_y; 
		  child_allocation.height = child_requisition.height;
		  
		  btk_menu_item_toggle_size_allocate (BTK_MENU_ITEM (child),
						      toggle_size);
		  btk_widget_size_allocate (child, &child_allocation);
		  
		  ltr_y += child_allocation.height;
		}
	    }
	}
    }
}

static void
btk_menu_bar_paint (BtkWidget    *widget,
                    BdkRectangle *area)
{
  g_return_if_fail (BTK_IS_MENU_BAR (widget));

  if (btk_widget_is_drawable (widget))
    {
      bint border;

      border = BTK_CONTAINER (widget)->border_width;
      
      btk_paint_box (widget->style,
		     widget->window,
                     btk_widget_get_state (widget),
                     get_shadow_type (BTK_MENU_BAR (widget)),
		     area, widget, "menubar",
		     border, border,
		     widget->allocation.width - border * 2,
                     widget->allocation.height - border * 2);
    }
}

static bint
btk_menu_bar_expose (BtkWidget      *widget,
		     BdkEventExpose *event)
{
  g_return_val_if_fail (BTK_IS_MENU_BAR (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (btk_widget_is_drawable (widget))
    {
      btk_menu_bar_paint (widget, &event->area);

      BTK_WIDGET_CLASS (btk_menu_bar_parent_class)->expose_event (widget, event);
    }

  return FALSE;
}

static GList *
get_menu_bars (BtkWindow *window)
{
  return g_object_get_data (B_OBJECT (window), "btk-menu-bar-list");
}

static GList *
get_viewable_menu_bars (BtkWindow *window)
{
  GList *menu_bars;
  GList *viewable_menu_bars = NULL;

  for (menu_bars = get_menu_bars (window);
       menu_bars;
       menu_bars = menu_bars->next)
    {
      BtkWidget *widget = menu_bars->data;
      bboolean viewable = TRUE;
      
      while (widget)
	{
	  if (!btk_widget_get_mapped (widget))
	    viewable = FALSE;
	  
	  widget = widget->parent;
	}

      if (viewable)
	viewable_menu_bars = g_list_prepend (viewable_menu_bars, menu_bars->data);
    }

  return g_list_reverse (viewable_menu_bars);
}

static void
set_menu_bars (BtkWindow *window,
	       GList     *menubars)
{
  g_object_set_data (B_OBJECT (window), I_("btk-menu-bar-list"), menubars);
}

static bboolean
window_key_press_handler (BtkWidget   *widget,
                          BdkEventKey *event,
                          bpointer     data)
{
  bchar *accel = NULL;
  bboolean retval = FALSE;
  
  g_object_get (btk_widget_get_settings (widget),
                "btk-menu-bar-accel", &accel,
                NULL);

  if (accel && *accel)
    {
      buint keyval = 0;
      BdkModifierType mods = 0;

      btk_accelerator_parse (accel, &keyval, &mods);

      if (keyval == 0)
        g_warning ("Failed to parse menu bar accelerator '%s'\n", accel);

      /* FIXME this is wrong, needs to be in the global accel resolution
       * thing, to properly consider i18n etc., but that probably requires
       * AccelGroup changes etc.
       */
      if (event->keyval == keyval &&
          ((event->state & btk_accelerator_get_default_mod_mask ()) ==
	   (mods & btk_accelerator_get_default_mod_mask ())))
        {
	  GList *tmp_menubars = get_viewable_menu_bars (BTK_WINDOW (widget));
	  GList *menubars;

	  menubars = _btk_container_focus_sort (BTK_CONTAINER (widget), tmp_menubars,
						BTK_DIR_TAB_FORWARD, NULL);
	  g_list_free (tmp_menubars);
	  
	  if (menubars)
	    {
	      BtkMenuShell *menu_shell = BTK_MENU_SHELL (menubars->data);

              _btk_menu_shell_set_keyboard_mode (menu_shell, TRUE);
	      btk_menu_shell_select_first (menu_shell, FALSE);
	      
	      g_list_free (menubars);
	      
	      retval = TRUE;	      
	    }
        }
    }

  g_free (accel);

  return retval;
}

static void
add_to_window (BtkWindow  *window,
               BtkMenuBar *menubar)
{
  GList *menubars = get_menu_bars (window);

  if (!menubars)
    {
      g_signal_connect (window,
			"key-press-event",
			G_CALLBACK (window_key_press_handler),
			NULL);
    }

  set_menu_bars (window, g_list_prepend (menubars, menubar));
}

static void
remove_from_window (BtkWindow  *window,
                    BtkMenuBar *menubar)
{
  GList *menubars = get_menu_bars (window);

  menubars = g_list_remove (menubars, menubar);

  if (!menubars)
    {
      g_signal_handlers_disconnect_by_func (window,
					    window_key_press_handler,
					    NULL);
    }

  set_menu_bars (window, menubars);
}

static void
btk_menu_bar_hierarchy_changed (BtkWidget *widget,
				BtkWidget *old_toplevel)
{
  BtkWidget *toplevel;  
  BtkMenuBar *menubar;

  menubar = BTK_MENU_BAR (widget);

  toplevel = btk_widget_get_toplevel (widget);

  if (old_toplevel)
    remove_from_window (BTK_WINDOW (old_toplevel), menubar);
  
  if (btk_widget_is_toplevel (toplevel))
    add_to_window (BTK_WINDOW (toplevel), menubar);
}

/**
 * _btk_menu_bar_cycle_focus:
 * @menubar: a #BtkMenuBar
 * @dir: direction in which to cycle the focus
 * 
 * Move the focus between menubars in the toplevel.
 **/
void
_btk_menu_bar_cycle_focus (BtkMenuBar       *menubar,
			   BtkDirectionType  dir)
{
  BtkWidget *toplevel = btk_widget_get_toplevel (BTK_WIDGET (menubar));
  BtkMenuItem *to_activate = NULL;

  if (btk_widget_is_toplevel (toplevel))
    {
      GList *tmp_menubars = get_viewable_menu_bars (BTK_WINDOW (toplevel));
      GList *menubars;
      GList *current;

      menubars = _btk_container_focus_sort (BTK_CONTAINER (toplevel), tmp_menubars,
					    dir, BTK_WIDGET (menubar));
      g_list_free (tmp_menubars);

      if (menubars)
	{
	  current = g_list_find (menubars, menubar);

	  if (current && current->next)
	    {
	      BtkMenuShell *new_menushell = BTK_MENU_SHELL (current->next->data);
	      if (new_menushell->children)
		to_activate = new_menushell->children->data;
	    }
	}
	  
      g_list_free (menubars);
    }

  btk_menu_shell_cancel (BTK_MENU_SHELL (menubar));

  if (to_activate)
    g_signal_emit_by_name (to_activate, "activate_item");
}

static BtkShadowType
get_shadow_type (BtkMenuBar *menubar)
{
  BtkShadowType shadow_type = BTK_SHADOW_OUT;
  
  btk_widget_style_get (BTK_WIDGET (menubar),
			"shadow-type", &shadow_type,
			NULL);

  return shadow_type;
}

static bint
btk_menu_bar_get_popup_delay (BtkMenuShell *menu_shell)
{
  bint popup_delay;
  
  g_object_get (btk_widget_get_settings (BTK_WIDGET (menu_shell)),
		"btk-menu-bar-popup-delay", &popup_delay,
		NULL);

  return popup_delay;
}

static void
btk_menu_bar_move_current (BtkMenuShell         *menu_shell,
			   BtkMenuDirectionType  direction)
{
  BtkMenuBar *menubar = BTK_MENU_BAR (menu_shell);
  BtkTextDirection text_dir;
  BtkPackDirection pack_dir;

  text_dir = btk_widget_get_direction (BTK_WIDGET (menubar));
  pack_dir = btk_menu_bar_get_pack_direction (menubar);
  
  if (pack_dir == BTK_PACK_DIRECTION_LTR || pack_dir == BTK_PACK_DIRECTION_RTL)
     {
      if ((text_dir == BTK_TEXT_DIR_RTL) == (pack_dir == BTK_PACK_DIRECTION_LTR))
	{
	  switch (direction) 
	    {      
	    case BTK_MENU_DIR_PREV:
	      direction = BTK_MENU_DIR_NEXT;
	      break;
	    case BTK_MENU_DIR_NEXT:
	      direction = BTK_MENU_DIR_PREV;
	      break;
	    default: ;
	    }
	}
    }
  else
    {
      switch (direction) 
	{
	case BTK_MENU_DIR_PARENT:
	  if ((text_dir == BTK_TEXT_DIR_LTR) == (pack_dir == BTK_PACK_DIRECTION_TTB))
	    direction = BTK_MENU_DIR_PREV;
	  else
	    direction = BTK_MENU_DIR_NEXT;
	  break;
	case BTK_MENU_DIR_CHILD:
	  if ((text_dir == BTK_TEXT_DIR_LTR) == (pack_dir == BTK_PACK_DIRECTION_TTB))
	    direction = BTK_MENU_DIR_NEXT;
	  else
	    direction = BTK_MENU_DIR_PREV;
	  break;
	case BTK_MENU_DIR_PREV:
	  if (text_dir == BTK_TEXT_DIR_RTL)	  
	    direction = BTK_MENU_DIR_CHILD;
	  else
	    direction = BTK_MENU_DIR_PARENT;
	  break;
	case BTK_MENU_DIR_NEXT:
	  if (text_dir == BTK_TEXT_DIR_RTL)	  
	    direction = BTK_MENU_DIR_PARENT;
	  else
	    direction = BTK_MENU_DIR_CHILD;
	  break;
	default: ;
	}
    }
  
  BTK_MENU_SHELL_CLASS (btk_menu_bar_parent_class)->move_current (menu_shell, direction);
}

/**
 * btk_menu_bar_get_pack_direction:
 * @menubar: a #BtkMenuBar
 * 
 * Retrieves the current pack direction of the menubar. 
 * See btk_menu_bar_set_pack_direction().
 *
 * Return value: the pack direction
 *
 * Since: 2.8
 */
BtkPackDirection
btk_menu_bar_get_pack_direction (BtkMenuBar *menubar)
{
  BtkMenuBarPrivate *priv;

  g_return_val_if_fail (BTK_IS_MENU_BAR (menubar), 
			BTK_PACK_DIRECTION_LTR);
  
  priv = BTK_MENU_BAR_GET_PRIVATE (menubar);

  return priv->pack_direction;
}

/**
 * btk_menu_bar_set_pack_direction:
 * @menubar: a #BtkMenuBar
 * @pack_dir: a new #BtkPackDirection
 * 
 * Sets how items should be packed inside a menubar.
 * 
 * Since: 2.8
 */
void
btk_menu_bar_set_pack_direction (BtkMenuBar       *menubar,
                                 BtkPackDirection  pack_dir)
{
  BtkMenuBarPrivate *priv;
  GList *l;

  g_return_if_fail (BTK_IS_MENU_BAR (menubar));

  priv = BTK_MENU_BAR_GET_PRIVATE (menubar);

  if (priv->pack_direction != pack_dir)
    {
      priv->pack_direction = pack_dir;

      btk_widget_queue_resize (BTK_WIDGET (menubar));

      for (l = BTK_MENU_SHELL (menubar)->children; l; l = l->next)
	btk_widget_queue_resize (BTK_WIDGET (l->data));

      g_object_notify (B_OBJECT (menubar), "pack-direction");
    }
}

/**
 * btk_menu_bar_get_child_pack_direction:
 * @menubar: a #BtkMenuBar
 * 
 * Retrieves the current child pack direction of the menubar.
 * See btk_menu_bar_set_child_pack_direction().
 *
 * Return value: the child pack direction
 *
 * Since: 2.8
 */
BtkPackDirection
btk_menu_bar_get_child_pack_direction (BtkMenuBar *menubar)
{
  BtkMenuBarPrivate *priv;

  g_return_val_if_fail (BTK_IS_MENU_BAR (menubar), 
			BTK_PACK_DIRECTION_LTR);
  
  priv = BTK_MENU_BAR_GET_PRIVATE (menubar);

  return priv->child_pack_direction;
}

/**
 * btk_menu_bar_set_child_pack_direction:
 * @menubar: a #BtkMenuBar
 * @child_pack_dir: a new #BtkPackDirection
 * 
 * Sets how widgets should be packed inside the children of a menubar.
 * 
 * Since: 2.8
 */
void
btk_menu_bar_set_child_pack_direction (BtkMenuBar       *menubar,
                                       BtkPackDirection  child_pack_dir)
{
  BtkMenuBarPrivate *priv;
  GList *l;

  g_return_if_fail (BTK_IS_MENU_BAR (menubar));

  priv = BTK_MENU_BAR_GET_PRIVATE (menubar);

  if (priv->child_pack_direction != child_pack_dir)
    {
      priv->child_pack_direction = child_pack_dir;

      btk_widget_queue_resize (BTK_WIDGET (menubar));

      for (l = BTK_MENU_SHELL (menubar)->children; l; l = l->next)
	btk_widget_queue_resize (BTK_WIDGET (l->data));

      g_object_notify (B_OBJECT (menubar), "child-pack-direction");
    }
}

#define __BTK_MENU_BAR_C__
#include "btkaliasdef.c"
