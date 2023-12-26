/* btktrayicon.c
 * Copyright (C) 2002 Anders Carlsson <andersca@gnu.org>
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
 * This is an implementation of the freedesktop.org "system tray" spec,
 * http://www.freedesktop.org/wiki/Standards/systemtray-spec
 */

#include "config.h"
#include <string.h>

#include "btkintl.h"
#include "btkprivate.h"
#include "btktrayicon.h"

#include "btkalias.h"

#include "x11/bdkx.h"
#include <X11/Xatom.h>

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

#define SYSTEM_TRAY_ORIENTATION_HORZ 0
#define SYSTEM_TRAY_ORIENTATION_VERT 1

enum {
  PROP_0,
  PROP_ORIENTATION
};

struct _BtkTrayIconPrivate
{
  buint stamp;
  
  Atom selection_atom;
  Atom manager_atom;
  Atom system_tray_opcode_atom;
  Atom orientation_atom;
  Atom visual_atom;
  Window manager_window;
  BdkVisual *manager_visual;
  bboolean manager_visual_rgba;

  BtkOrientation orientation;
};

static void btk_tray_icon_constructed   (BObject     *object);
static void btk_tray_icon_dispose       (BObject     *object);

static void btk_tray_icon_get_property  (BObject     *object,
				 	 buint        prop_id,
					 BValue      *value,
					 BParamSpec  *pspec);

static void     btk_tray_icon_realize   (BtkWidget   *widget);
static void     btk_tray_icon_style_set (BtkWidget   *widget,
					 BtkStyle    *previous_style);
static bboolean btk_tray_icon_delete    (BtkWidget   *widget,
					 BdkEventAny *event);
static bboolean btk_tray_icon_expose    (BtkWidget      *widget, 
					 BdkEventExpose *event);

static void btk_tray_icon_clear_manager_window     (BtkTrayIcon *icon);
static void btk_tray_icon_update_manager_window    (BtkTrayIcon *icon);
static void btk_tray_icon_manager_window_destroyed (BtkTrayIcon *icon);

static BdkFilterReturn btk_tray_icon_manager_filter (BdkXEvent *xevent,
						     BdkEvent  *event,
						     bpointer   user_data);


G_DEFINE_TYPE (BtkTrayIcon, btk_tray_icon, BTK_TYPE_PLUG)

static void
btk_tray_icon_class_init (BtkTrayIconClass *class)
{
  BObjectClass *bobject_class = (BObjectClass *)class;
  BtkWidgetClass *widget_class = (BtkWidgetClass *)class;

  bobject_class->get_property = btk_tray_icon_get_property;
  bobject_class->constructed = btk_tray_icon_constructed;
  bobject_class->dispose = btk_tray_icon_dispose;

  widget_class->realize = btk_tray_icon_realize;
  widget_class->style_set = btk_tray_icon_style_set;
  widget_class->delete_event = btk_tray_icon_delete;
  widget_class->expose_event = btk_tray_icon_expose;

  g_object_class_install_property (bobject_class,
				   PROP_ORIENTATION,
				   g_param_spec_enum ("orientation",
						      P_("Orientation"),
						      P_("The orientation of the tray"),
						      BTK_TYPE_ORIENTATION,
						      BTK_ORIENTATION_HORIZONTAL,
						      BTK_PARAM_READABLE));

  g_type_class_add_private (class, sizeof (BtkTrayIconPrivate));
}

static void
btk_tray_icon_init (BtkTrayIcon *icon)
{
  icon->priv = B_TYPE_INSTANCE_GET_PRIVATE (icon, BTK_TYPE_TRAY_ICON,
					    BtkTrayIconPrivate);
  
  icon->priv->stamp = 1;
  icon->priv->orientation = BTK_ORIENTATION_HORIZONTAL;

  btk_widget_set_app_paintable (BTK_WIDGET (icon), TRUE);
  btk_widget_add_events (BTK_WIDGET (icon), BDK_PROPERTY_CHANGE_MASK);
}

static void
btk_tray_icon_constructed (BObject *object)
{
  /* Do setup that depends on the screen; screen has been set at this point */

  BtkTrayIcon *icon = BTK_TRAY_ICON (object);
  BdkScreen *screen = btk_widget_get_screen (BTK_WIDGET (object));
  BdkWindow *root_window = bdk_screen_get_root_window (screen);
  BdkDisplay *display = btk_widget_get_display (BTK_WIDGET (object));
  Display *xdisplay = bdk_x11_display_get_xdisplay (display);
  char buffer[256];
  
  g_snprintf (buffer, sizeof (buffer),
	      "_NET_SYSTEM_TRAY_S%d",
	      bdk_screen_get_number (screen));

  icon->priv->selection_atom = XInternAtom (xdisplay, buffer, False);
  
  icon->priv->manager_atom = XInternAtom (xdisplay, "MANAGER", False);
  
  icon->priv->system_tray_opcode_atom = XInternAtom (xdisplay,
						     "_NET_SYSTEM_TRAY_OPCODE",
						     False);

  icon->priv->orientation_atom = XInternAtom (xdisplay,
					      "_NET_SYSTEM_TRAY_ORIENTATION",
					      False);

  icon->priv->visual_atom = XInternAtom (xdisplay,
					 "_NET_SYSTEM_TRAY_VISUAL",
					 False);

  /* Add a root window filter so that we get changes on MANAGER */
  bdk_window_add_filter (root_window,
			 btk_tray_icon_manager_filter, icon);

  btk_tray_icon_update_manager_window (icon);
}

static void
btk_tray_icon_clear_manager_window (BtkTrayIcon *icon)
{
  BdkDisplay *display = btk_widget_get_display (BTK_WIDGET (icon));

  if (icon->priv->manager_window != None)
    {
      BdkWindow *bdkwin;

      bdkwin = bdk_window_lookup_for_display (display,
                                              icon->priv->manager_window);

      bdk_window_remove_filter (bdkwin, btk_tray_icon_manager_filter, icon);

      icon->priv->manager_window = None;
      icon->priv->manager_visual = NULL;
    }
}

static void
btk_tray_icon_dispose (BObject *object)
{
  BtkTrayIcon *icon = BTK_TRAY_ICON (object);
  BtkWidget *widget = BTK_WIDGET (object);
  BdkWindow *root_window = bdk_screen_get_root_window (btk_widget_get_screen (widget));

  btk_tray_icon_clear_manager_window (icon);

  bdk_window_remove_filter (root_window, btk_tray_icon_manager_filter, icon);

  B_OBJECT_CLASS (btk_tray_icon_parent_class)->dispose (object);
}

static void
btk_tray_icon_get_property (BObject    *object,
			    buint       prop_id,
			    BValue     *value,
			    BParamSpec *pspec)
{
  BtkTrayIcon *icon = BTK_TRAY_ICON (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      b_value_set_enum (value, icon->priv->orientation);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static bboolean
btk_tray_icon_expose (BtkWidget      *widget, 
		      BdkEventExpose *event)
{
  BtkTrayIcon *icon = BTK_TRAY_ICON (widget);
  BtkWidget *focus_child;
  bint border_width, x, y, width, height;
  bboolean retval = FALSE;

  if (icon->priv->manager_visual_rgba)
    {
      /* Clear to transparent */
      bairo_t *cr = bdk_bairo_create (widget->window);
      bairo_set_source_rgba (cr, 0, 0, 0, 0);
      bairo_set_operator (cr, BAIRO_OPERATOR_SOURCE);
      bdk_bairo_rebunnyion (cr, event->rebunnyion);
      bairo_fill (cr);
      bairo_destroy (cr);
    }
  else
    {
      /* Clear to parent-relative pixmap */
      bdk_window_clear_area (widget->window, event->area.x, event->area.y,
			     event->area.width, event->area.height);
    }

  if (BTK_WIDGET_CLASS (btk_tray_icon_parent_class)->expose_event)
    retval = BTK_WIDGET_CLASS (btk_tray_icon_parent_class)->expose_event (widget, event);

  focus_child = BTK_CONTAINER (widget)->focus_child;
  if (focus_child && btk_widget_has_focus (focus_child))
    {
      border_width = BTK_CONTAINER (widget)->border_width;

      x = widget->allocation.x + border_width;
      y = widget->allocation.y + border_width;

      width  = widget->allocation.width  - 2 * border_width;
      height = widget->allocation.height - 2 * border_width;

      btk_paint_focus (widget->style, widget->window,
                       btk_widget_get_state (widget),
                       &event->area, widget, "tray_icon",
                       x, y, width, height);
    }

  return retval;
}

static void
btk_tray_icon_get_orientation_property (BtkTrayIcon *icon)
{
  BdkScreen *screen = btk_widget_get_screen (BTK_WIDGET (icon));
  BdkDisplay *display = bdk_screen_get_display (screen);
  Display *xdisplay = BDK_DISPLAY_XDISPLAY (display);

  Atom type;
  int format;
  union {
	bulong *prop;
	buchar *prop_ch;
  } prop = { NULL };
  bulong nitems;
  bulong bytes_after;
  int error, result;

  g_assert (icon->priv->manager_window != None);
  
  bdk_error_trap_push ();
  type = None;
  result = XGetWindowProperty (xdisplay,
			       icon->priv->manager_window,
			       icon->priv->orientation_atom,
			       0, B_MAXLONG, FALSE,
			       XA_CARDINAL,
			       &type, &format, &nitems,
			       &bytes_after, &(prop.prop_ch));
  error = bdk_error_trap_pop ();

  if (error || result != Success)
    return;

  if (type == XA_CARDINAL && nitems == 1 && format == 32)
    {
      BtkOrientation orientation;

      orientation = (prop.prop [0] == SYSTEM_TRAY_ORIENTATION_HORZ) ?
					BTK_ORIENTATION_HORIZONTAL :
					BTK_ORIENTATION_VERTICAL;

      if (icon->priv->orientation != orientation)
	{
	  icon->priv->orientation = orientation;

	  g_object_notify (B_OBJECT (icon), "orientation");
	}
    }

  if (type != None)
    XFree (prop.prop);
}

static void
btk_tray_icon_get_visual_property (BtkTrayIcon *icon)
{
  BdkScreen *screen = btk_widget_get_screen (BTK_WIDGET (icon));
  BdkDisplay *display = bdk_screen_get_display (screen);
  Display *xdisplay = BDK_DISPLAY_XDISPLAY (display);

  Atom type;
  int format;
  union {
	bulong *prop;
	buchar *prop_ch;
  } prop = { NULL };
  bulong nitems;
  bulong bytes_after;
  int error, result;

  g_assert (icon->priv->manager_window != None);

  bdk_error_trap_push ();
  type = None;
  result = XGetWindowProperty (xdisplay,
			       icon->priv->manager_window,
			       icon->priv->visual_atom,
			       0, B_MAXLONG, FALSE,
			       XA_VISUALID,
			       &type, &format, &nitems,
			       &bytes_after, &(prop.prop_ch));
  error = bdk_error_trap_pop ();

  if (!error && result == Success &&
      type == XA_VISUALID && nitems == 1 && format == 32)
    {
      VisualID visual_id;
      BdkVisual *visual;

      visual_id = prop.prop[0];
      visual = bdk_x11_screen_lookup_visual (screen, visual_id);

      icon->priv->manager_visual = visual;
      icon->priv->manager_visual_rgba = visual != NULL &&
        (visual->red_prec + visual->blue_prec + visual->green_prec < visual->depth);
    }
  else
    {
      icon->priv->manager_visual = NULL;
      icon->priv->manager_visual_rgba = FALSE;
    }


  /* For the background-relative hack we use when we aren't using a real RGBA
   * visual, we can't be double-buffered */
  btk_widget_set_double_buffered (BTK_WIDGET (icon), icon->priv->manager_visual_rgba);

  if (type != None)
    XFree (prop.prop);
}

static BdkFilterReturn
btk_tray_icon_manager_filter (BdkXEvent *xevent, 
			      BdkEvent  *event, 
			      bpointer   user_data)
{
  BtkTrayIcon *icon = user_data;
  XEvent *xev = (XEvent *)xevent;

  if (xev->xany.type == ClientMessage &&
      xev->xclient.message_type == icon->priv->manager_atom &&
      xev->xclient.data.l[1] == icon->priv->selection_atom)
    {
      BTK_NOTE (PLUGSOCKET,
		g_print ("BtkStatusIcon %p: tray manager appeared\n", icon));

      btk_tray_icon_update_manager_window (icon);
    }
  else if (xev->xany.window == icon->priv->manager_window)
    {
      if (xev->xany.type == PropertyNotify &&
	  xev->xproperty.atom == icon->priv->orientation_atom)
	{
          BTK_NOTE (PLUGSOCKET,
		    g_print ("BtkStatusIcon %p: got PropertyNotify on manager window for orientation atom\n", icon));

	  btk_tray_icon_get_orientation_property (icon);
	}
      else if (xev->xany.type == DestroyNotify)
	{
          BTK_NOTE (PLUGSOCKET,
		    g_print ("BtkStatusIcon %p: got DestroyNotify for manager window\n", icon));

	  btk_tray_icon_manager_window_destroyed (icon);
	}
      else
        BTK_NOTE (PLUGSOCKET,
		  g_print ("BtkStatusIcon %p: got other message on manager window\n", icon));
    }
  
  return BDK_FILTER_CONTINUE;
}

static void
btk_tray_icon_send_manager_message (BtkTrayIcon *icon,
				    long         message,
				    Window       window,
				    long         data1,
				    long         data2,
				    long         data3)
{
  XClientMessageEvent ev;
  Display *display;
  
  memset (&ev, 0, sizeof (ev));
  ev.type = ClientMessage;
  ev.window = window;
  ev.message_type = icon->priv->system_tray_opcode_atom;
  ev.format = 32;
  ev.data.l[0] = bdk_x11_get_server_time (BTK_WIDGET (icon)->window);
  ev.data.l[1] = message;
  ev.data.l[2] = data1;
  ev.data.l[3] = data2;
  ev.data.l[4] = data3;

  display = BDK_DISPLAY_XDISPLAY (btk_widget_get_display (BTK_WIDGET (icon)));
  
  bdk_error_trap_push ();
  XSendEvent (display,
	      icon->priv->manager_window, False, NoEventMask, (XEvent *)&ev);
  bdk_display_sync (btk_widget_get_display (BTK_WIDGET (icon)));
  bdk_error_trap_pop ();
}

static void
btk_tray_icon_send_dock_request (BtkTrayIcon *icon)
{
  BTK_NOTE (PLUGSOCKET,
	    g_print ("BtkStatusIcon %p: sending dock request to manager window %lx\n",
	    	     icon, (bulong) icon->priv->manager_window));

  btk_tray_icon_send_manager_message (icon,
				      SYSTEM_TRAY_REQUEST_DOCK,
				      icon->priv->manager_window,
				      btk_plug_get_id (BTK_PLUG (icon)),
				      0, 0);
}

static void
btk_tray_icon_update_manager_window (BtkTrayIcon *icon)
{
  BtkWidget *widget = BTK_WIDGET (icon);
  BdkScreen *screen = btk_widget_get_screen (widget);
  BdkDisplay *display = bdk_screen_get_display (screen);
  Display *xdisplay = BDK_DISPLAY_XDISPLAY (display);

  BTK_NOTE (PLUGSOCKET,
	    g_print ("BtkStatusIcon %p: updating tray icon manager window, current manager window: %lx\n",
		     icon, (bulong) icon->priv->manager_window));

  if (icon->priv->manager_window != None)
    return;

  BTK_NOTE (PLUGSOCKET,
	    g_print ("BtkStatusIcon %p: trying to find manager window\n", icon));

  XGrabServer (xdisplay);
  
  icon->priv->manager_window = XGetSelectionOwner (xdisplay,
						   icon->priv->selection_atom);

  if (icon->priv->manager_window != None)
    XSelectInput (xdisplay,
		  icon->priv->manager_window, StructureNotifyMask|PropertyChangeMask);

  XUngrabServer (xdisplay);
  XFlush (xdisplay);
  
  if (icon->priv->manager_window != None)
    {
      BdkWindow *bdkwin;

      BTK_NOTE (PLUGSOCKET,
		g_print ("BtkStatusIcon %p: is being managed by window %lx\n",
				icon, (bulong) icon->priv->manager_window));

      bdkwin = bdk_window_lookup_for_display (display,
					      icon->priv->manager_window);
      
      bdk_window_add_filter (bdkwin, btk_tray_icon_manager_filter, icon);

      btk_tray_icon_get_orientation_property (icon);
      btk_tray_icon_get_visual_property (icon);

      if (btk_widget_get_realized (BTK_WIDGET (icon)))
	{
	  if ((icon->priv->manager_visual == NULL &&
	       btk_widget_get_visual (widget) == bdk_screen_get_system_visual (screen)) ||
	      (icon->priv->manager_visual == btk_widget_get_visual (widget)))
	    {
	      /* Already have the right visual, can just dock
	       */
	      btk_tray_icon_send_dock_request (icon);
	    }
	  else
	    {
	      /* Need to re-realize the widget to get the right visual
	       */
	      btk_widget_hide (widget);
	      btk_widget_unrealize (widget);
	      btk_widget_show (widget);
	    }
	}
    }
  else
    BTK_NOTE (PLUGSOCKET,
	      g_print ("BtkStatusIcon %p: no tray manager found\n", icon));
}

static void
btk_tray_icon_manager_window_destroyed (BtkTrayIcon *icon)
{
  g_return_if_fail (icon->priv->manager_window != None);

  BTK_NOTE (PLUGSOCKET,
	    g_print ("BtkStatusIcon %p: tray manager window destroyed\n", icon));

  btk_tray_icon_clear_manager_window (icon);
}

static bboolean
btk_tray_icon_delete (BtkWidget   *widget,
		      BdkEventAny *event)
{
#ifdef G_ENABLE_DEBUG
  BtkTrayIcon *icon = BTK_TRAY_ICON (widget);
#endif

  BTK_NOTE (PLUGSOCKET,
	    g_print ("BtkStatusIcon %p: delete notify, tray manager window %lx\n",
		     icon, (bulong) icon->priv->manager_window));

  /* A bug in X server versions up to x.org 1.5.0 means that:
   * XFixesChangeSaveSet(...., SaveSetRoot, SaveSetUnmap) doesn't work properly
   * and we'll left mapped in a separate toplevel window if the tray is destroyed.
   * For simplicity just get rid of our X window and start over.
   */
  btk_widget_hide (widget);
  btk_widget_unrealize (widget);
  btk_widget_show (widget);

  /* Handled it, don't destroy the tray icon */
  return TRUE;
}

static void
btk_tray_icon_set_colormap (BtkTrayIcon *icon)
{
  BdkScreen *screen = btk_widget_get_screen (BTK_WIDGET (icon));
  BdkColormap *colormap;
  BdkVisual *visual = icon->priv->manager_visual;
  bboolean new_colormap = FALSE;

  /* To avoid uncertainty about colormaps, _NET_SYSTEM_TRAY_VISUAL is supposed
   * to be either the screen default visual or a TrueColor visual; ignore it
   * if it is something else
   */
  if (visual && visual->type != BDK_VISUAL_TRUE_COLOR)
    visual = NULL;

  if (visual == NULL || visual == bdk_screen_get_system_visual (screen))
    colormap = bdk_screen_get_system_colormap (screen);
  else if (visual == bdk_screen_get_rgb_visual (screen))
    colormap = bdk_screen_get_rgb_colormap (screen);
  else if (visual == bdk_screen_get_rgba_visual (screen))
    colormap = bdk_screen_get_rgba_colormap (screen);
  else
    {
      colormap = bdk_colormap_new (visual, FALSE);
      new_colormap = TRUE;
    }

  btk_widget_set_colormap (BTK_WIDGET (icon), colormap);

  if (new_colormap)
    g_object_unref (colormap);
}

static void
btk_tray_icon_realize (BtkWidget *widget)
{
  BtkTrayIcon *icon = BTK_TRAY_ICON (widget);

  /* Set our colormap before realizing */
  btk_tray_icon_set_colormap (icon);

  BTK_WIDGET_CLASS (btk_tray_icon_parent_class)->realize (widget);
  if (icon->priv->manager_visual_rgba)
    {
      /* Set a transparent background */
      BdkColor transparent = { 0, 0, 0, 0 }; /* Only pixel=0 matters */
      bdk_window_set_background (widget->window, &transparent);
    }
  else
    {
      /* Set a parent-relative background pixmap */
      bdk_window_set_back_pixmap (widget->window, NULL, TRUE);
    }

  BTK_NOTE (PLUGSOCKET,
	    g_print ("BtkStatusIcon %p: realized, window: %lx, socket window: %lx\n",
		     widget,
		     (bulong) BDK_WINDOW_XWINDOW (widget->window),
		     BTK_PLUG (icon)->socket_window ?
			     (bulong) BDK_WINDOW_XWINDOW (BTK_PLUG (icon)->socket_window) : 0UL));

  if (icon->priv->manager_window != None)
    btk_tray_icon_send_dock_request (icon);
}

static void
btk_tray_icon_style_set (BtkWidget   *widget,
			 BtkStyle    *previous_style)
{
  /* The default handler resets the background according to the style. We either
   * use a transparent background or a parent-relative background and ignore the
   * style background. So, just don't chain up.
   */
}

buint
_btk_tray_icon_send_message (BtkTrayIcon *icon,
			     bint         timeout,
			     const bchar *message,
			     bint         len)
{
  buint stamp;
  Display *xdisplay;
 
  g_return_val_if_fail (BTK_IS_TRAY_ICON (icon), 0);
  g_return_val_if_fail (timeout >= 0, 0);
  g_return_val_if_fail (message != NULL, 0);

  if (icon->priv->manager_window == None)
    return 0;

  if (len < 0)
    len = strlen (message);

  stamp = icon->priv->stamp++;
  
  /* Get ready to send the message */
  btk_tray_icon_send_manager_message (icon, SYSTEM_TRAY_BEGIN_MESSAGE,
				      (Window)btk_plug_get_id (BTK_PLUG (icon)),
				      timeout, len, stamp);

  /* Now to send the actual message */
  xdisplay = BDK_DISPLAY_XDISPLAY (btk_widget_get_display (BTK_WIDGET (icon)));
  bdk_error_trap_push ();
  while (len > 0)
    {
      XClientMessageEvent ev;

      memset (&ev, 0, sizeof (ev));
      ev.type = ClientMessage;
      ev.window = (Window)btk_plug_get_id (BTK_PLUG (icon));
      ev.format = 8;
      ev.message_type = XInternAtom (xdisplay,
				     "_NET_SYSTEM_TRAY_MESSAGE_DATA", False);
      if (len > 20)
	{
	  memcpy (&ev.data, message, 20);
	  len -= 20;
	  message += 20;
	}
      else
	{
	  memcpy (&ev.data, message, len);
	  len = 0;
	}

      XSendEvent (xdisplay,
		  icon->priv->manager_window, False,
		  StructureNotifyMask, (XEvent *)&ev);
    }
  bdk_display_sync (btk_widget_get_display (BTK_WIDGET (icon)));
  bdk_error_trap_pop ();

  return stamp;
}

void
_btk_tray_icon_cancel_message (BtkTrayIcon *icon,
			       buint        id)
{
  g_return_if_fail (BTK_IS_TRAY_ICON (icon));
  g_return_if_fail (id > 0);
  
  btk_tray_icon_send_manager_message (icon, SYSTEM_TRAY_CANCEL_MESSAGE,
				      (Window)btk_plug_get_id (BTK_PLUG (icon)),
				      id, 0, 0);
}

BtkTrayIcon *
_btk_tray_icon_new_for_screen (BdkScreen  *screen, 
			       const bchar *name)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);

  return g_object_new (BTK_TYPE_TRAY_ICON, 
		       "screen", screen, 
		       "title", name, 
		       NULL);
}

BtkTrayIcon*
_btk_tray_icon_new (const bchar *name)
{
  return g_object_new (BTK_TYPE_TRAY_ICON, 
		       "title", name, 
		       NULL);
}

BtkOrientation
_btk_tray_icon_get_orientation (BtkTrayIcon *icon)
{
  g_return_val_if_fail (BTK_IS_TRAY_ICON (icon), BTK_ORIENTATION_HORIZONTAL);

  return icon->priv->orientation;
}


#define __BTK_TRAY_ICON_X11_C__
#include "btkaliasdef.c"

