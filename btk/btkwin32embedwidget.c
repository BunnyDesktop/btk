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
 * Modified by the BTK+ Team and others 1997-2006.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"

#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkwin32embedwidget.h"
#include "btkintl.h"
#include "btkprivate.h"

#include "btkalias.h"

static void            btk_win32_embed_widget_realize               (BtkWidget        *widget);
static void            btk_win32_embed_widget_unrealize             (BtkWidget        *widget);
static void            btk_win32_embed_widget_show                  (BtkWidget        *widget);
static void            btk_win32_embed_widget_hide                  (BtkWidget        *widget);
static void            btk_win32_embed_widget_map                   (BtkWidget        *widget);
static void            btk_win32_embed_widget_unmap                 (BtkWidget        *widget);
static void            btk_win32_embed_widget_size_allocate         (BtkWidget        *widget,
								     BtkAllocation    *allocation);
static void            btk_win32_embed_widget_set_focus             (BtkWindow        *window,
								     BtkWidget        *focus);
static bboolean        btk_win32_embed_widget_focus                 (BtkWidget        *widget,
								     BtkDirectionType  direction);
static void            btk_win32_embed_widget_check_resize          (BtkContainer     *container);

static BtkBinClass *bin_class = NULL;

G_DEFINE_TYPE (BtkWin32EmbedWidget, btk_win32_embed_widget, BTK_TYPE_WINDOW)

static void
btk_win32_embed_widget_class_init (BtkWin32EmbedWidgetClass *class)
{
  BtkWidgetClass *widget_class = (BtkWidgetClass *)class;
  BtkWindowClass *window_class = (BtkWindowClass *)class;
  BtkContainerClass *container_class = (BtkContainerClass *)class;

  bin_class = g_type_class_peek (BTK_TYPE_BIN);

  widget_class->realize = btk_win32_embed_widget_realize;
  widget_class->unrealize = btk_win32_embed_widget_unrealize;

  widget_class->show = btk_win32_embed_widget_show;
  widget_class->hide = btk_win32_embed_widget_hide;
  widget_class->map = btk_win32_embed_widget_map;
  widget_class->unmap = btk_win32_embed_widget_unmap;
  widget_class->size_allocate = btk_win32_embed_widget_size_allocate;

  widget_class->focus = btk_win32_embed_widget_focus;
  
  container_class->check_resize = btk_win32_embed_widget_check_resize;

  window_class->set_focus = btk_win32_embed_widget_set_focus;
}

static void
btk_win32_embed_widget_init (BtkWin32EmbedWidget *embed_widget)
{
  BtkWindow *window;

  window = BTK_WINDOW (embed_widget);

  window->type = BTK_WINDOW_TOPLEVEL;

  _btk_widget_set_is_toplevel (BTK_WIDGET (embed_widget), TRUE);
  btk_container_set_resize_mode (BTK_CONTAINER (embed_widget), BTK_RESIZE_QUEUE);
}

BtkWidget*
_btk_win32_embed_widget_new (BdkNativeWindow parent_id)
{
  BtkWin32EmbedWidget *embed_widget;

  embed_widget = g_object_new (BTK_TYPE_WIN32_EMBED_WIDGET, NULL);
  
  embed_widget->parent_window =
    bdk_window_lookup_for_display (bdk_display_get_default (),
				   parent_id);
  
  if (!embed_widget->parent_window)
    embed_widget->parent_window =
      bdk_window_foreign_new_for_display (bdk_display_get_default (),
					  parent_id);
  
  return BTK_WIDGET (embed_widget);
}

BOOL
_btk_win32_embed_widget_dialog_procedure (BtkWin32EmbedWidget *embed_widget,
					  HWND wnd, UINT message, WPARAM wparam, LPARAM lparam)
{
  BtkWidget *widget = BTK_WIDGET (embed_widget);
  
 if (message == WM_SIZE)
   {
     widget->allocation.width = LOWORD(lparam);
     widget->allocation.height = HIWORD(lparam);
     
     btk_widget_queue_resize (widget);
   }
        
 return 0;
}

static void
btk_win32_embed_widget_unrealize (BtkWidget *widget)
{
  BtkWin32EmbedWidget *embed_widget = BTK_WIN32_EMBED_WIDGET (widget);

  embed_widget->old_window_procedure = NULL;
  
  if (embed_widget->parent_window != NULL)
    {
      bdk_window_set_user_data (embed_widget->parent_window, NULL);
      g_object_unref (embed_widget->parent_window);
      embed_widget->parent_window = NULL;
    }

  BTK_WIDGET_CLASS (btk_win32_embed_widget_parent_class)->unrealize (widget);
}

static LRESULT CALLBACK
btk_win32_embed_widget_window_process (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  BdkWindow *window;
  BtkWin32EmbedWidget *embed_widget;
  bpointer user_data;

  window = bdk_window_lookup ((BdkNativeWindow)hwnd);
  if (window == NULL) {
    g_warning ("No such window!");
    return 0;
  }
  bdk_window_get_user_data (window, &user_data);
  embed_widget = BTK_WIN32_EMBED_WIDGET (user_data);

  if (msg == WM_GETDLGCODE) {
    return DLGC_WANTALLKEYS;
  }

  if (embed_widget && embed_widget->old_window_procedure)
    return CallWindowProc(embed_widget->old_window_procedure,
			  hwnd, msg, wparam, lparam);
  else
    return 0;
}

static void
btk_win32_embed_widget_realize (BtkWidget *widget)
{
  BtkWindow *window = BTK_WINDOW (widget);
  BtkWin32EmbedWidget *embed_widget = BTK_WIN32_EMBED_WIDGET (widget);
  BdkWindowAttr attributes;
  bint attributes_mask;
  LONG_PTR styles;

  /* ensure widget tree is properly size allocated */
  if (widget->allocation.x == -1 &&
      widget->allocation.y == -1 &&
      widget->allocation.width == 1 &&
      widget->allocation.height == 1)
    {
      BtkRequisition requisition;
      BtkAllocation allocation = { 0, 0, 200, 200 };

      btk_widget_size_request (widget, &requisition);
      if (requisition.width || requisition.height)
	{
	  /* non-empty window */
	  allocation.width = requisition.width;
	  allocation.height = requisition.height;
	}
      btk_widget_size_allocate (widget, &allocation);
      
      _btk_container_queue_resize (BTK_CONTAINER (widget));

      g_return_if_fail (!btk_widget_get_realized (widget));
    }

  btk_widget_set_realized (widget, TRUE);

  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.title = window->title;
  attributes.wmclass_name = window->wmclass_name;
  attributes.wmclass_class = window->wmclass_class;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = BDK_INPUT_OUTPUT;

  /* this isn't right - we should match our parent's visual/colormap.
   * though that will require handling "foreign" colormaps */
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);
  attributes.event_mask = btk_widget_get_events (widget);
  attributes.event_mask |= (BDK_EXPOSURE_MASK |
			    BDK_KEY_PRESS_MASK |
			    BDK_KEY_RELEASE_MASK |
			    BDK_ENTER_NOTIFY_MASK |
			    BDK_LEAVE_NOTIFY_MASK |
			    BDK_STRUCTURE_MASK |
			    BDK_FOCUS_CHANGE_MASK);

  attributes_mask = BDK_WA_VISUAL | BDK_WA_COLORMAP;
  attributes_mask |= (window->title ? BDK_WA_TITLE : 0);
  attributes_mask |= (window->wmclass_name ? BDK_WA_WMCLASS : 0);

  widget->window = bdk_window_new (embed_widget->parent_window, 
				   &attributes, attributes_mask);

  bdk_window_set_user_data (widget->window, window);

  embed_widget->old_window_procedure = (bpointer)
    SetWindowLongPtrW(BDK_WINDOW_HWND (widget->window),
		      GWLP_WNDPROC,
		      (LONG_PTR)btk_win32_embed_widget_window_process);

  /* Enable tab to focus the widget */
  styles = GetWindowLongPtr(BDK_WINDOW_HWND (widget->window), GWL_STYLE);
  SetWindowLongPtrW(BDK_WINDOW_HWND (widget->window), GWL_STYLE, styles | WS_TABSTOP);
  
  widget->style = btk_style_attach (widget->style, widget->window);
  btk_style_set_background (widget->style, widget->window, BTK_STATE_NORMAL);
}

static void
btk_win32_embed_widget_show (BtkWidget *widget)
{
  BTK_WIDGET_SET_FLAGS (widget, BTK_VISIBLE);
  
  btk_widget_realize (widget);
  btk_container_check_resize (BTK_CONTAINER (widget));
  btk_widget_map (widget);
}

static void
btk_win32_embed_widget_hide (BtkWidget *widget)
{
  BTK_WIDGET_UNSET_FLAGS (widget, BTK_VISIBLE);
  btk_widget_unmap (widget);
}

static void
btk_win32_embed_widget_map (BtkWidget *widget)
{
  BtkBin    *bin = BTK_BIN (widget);
  BtkWidget *child;

  btk_widget_set_mapped (widget, TRUE);

  child = btk_bin_get_child (bin);
  if (child &&
      btk_widget_get_visible (child) &&
      !btk_widget_get_mapped (child))
    btk_widget_map (child);

  bdk_window_show (widget->window);
}

static void
btk_win32_embed_widget_unmap (BtkWidget *widget)
{
  btk_widget_set_mapped (widget, FALSE);
  bdk_window_hide (widget->window);
}

static void
btk_win32_embed_widget_size_allocate (BtkWidget     *widget,
				      BtkAllocation *allocation)
{
  BtkBin    *bin = BTK_BIN (widget);
  BtkWidget *child;
  
  widget->allocation = *allocation;
  
  if (btk_widget_get_realized (widget))
    bdk_window_move_resize (widget->window,
			    allocation->x, allocation->y,
			    allocation->width, allocation->height);

  child = btk_bin_get_child (bin);
  if (child && btk_widget_get_visible (child))
    {
      BtkAllocation child_allocation;
      
      child_allocation.x = btk_container_get_border_width (BTK_CONTAINER (widget));
      child_allocation.y = child_allocation.x;
      child_allocation.width =
	MAX (1, (bint)allocation->width - child_allocation.x * 2);
      child_allocation.height =
	MAX (1, (bint)allocation->height - child_allocation.y * 2);
      
      btk_widget_size_allocate (child, &child_allocation);
    }
}

static void
btk_win32_embed_widget_check_resize (BtkContainer *container)
{
  BTK_CONTAINER_CLASS (bin_class)->check_resize (container);
}

static bboolean
btk_win32_embed_widget_focus (BtkWidget        *widget,
			      BtkDirectionType  direction)
{
  BtkBin *bin = BTK_BIN (widget);
  BtkWin32EmbedWidget *embed_widget = BTK_WIN32_EMBED_WIDGET (widget);
  BtkWindow *window = BTK_WINDOW (widget);
  BtkContainer *container = BTK_CONTAINER (widget);
  BtkWidget *old_focus_child = btk_container_get_focus_child (container);
  BtkWidget *parent;
  BtkWidget *child;

  /* We override BtkWindow's behavior, since we don't want wrapping here.
   */
  if (old_focus_child)
    {
      if (btk_widget_child_focus (old_focus_child, direction))
	return TRUE;

      if (window->focus_widget)
	{
	  /* Wrapped off the end, clear the focus setting for the toplevel */
	  parent = window->focus_widget->parent;
	  while (parent)
	    {
	      btk_container_set_focus_child (BTK_CONTAINER (parent), NULL);
	      parent = BTK_WIDGET (parent)->parent;
	    }
	  
	  btk_window_set_focus (BTK_WINDOW (container), NULL);
	}
    }
  else
    {
      /* Try to focus the first widget in the window */
      child = btk_bin_get_child (bin);
      if (child && btk_widget_child_focus (child, direction))
        return TRUE;
    }

  if (!btk_container_get_focus_child (BTK_CONTAINER (window)))
    {
      int backwards = FALSE;

      if (direction == BTK_DIR_TAB_BACKWARD ||
	  direction == BTK_DIR_LEFT)
	backwards = TRUE;
      
      PostMessage(BDK_WINDOW_HWND (embed_widget->parent_window),
				   WM_NEXTDLGCTL,
				   backwards, 0);
    }

  return FALSE;
}

static void
btk_win32_embed_widget_set_focus (BtkWindow *window,
				  BtkWidget *focus)
{
  BTK_WINDOW_CLASS (btk_win32_embed_widget_parent_class)->set_focus (window, focus);

  bdk_window_focus (BTK_WIDGET(window)->window, 0);
}

#define __BTK_WIN32_EMBED_WIDGET_C__
#include "btkaliasdef.c"
