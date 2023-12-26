/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1999 Peter Mattis, Spencer Kimball and Josh MacDonald
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

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "bdkconfig.h"

#include "bdk/bdkkeysyms.h"

#ifdef BDK_WINDOWING_X11
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "bdk/x11/bdkx.h"
#endif

#include "btkdnd.h"
#include "btkiconfactory.h"
#include "btkicontheme.h"
#include "btkimage.h"
#include "btkinvisible.h"
#include "btkmain.h"
#include "btkplug.h"
#include "btkstock.h"
#include "btktooltip.h"
#include "btkwindow.h"
#include "btkintl.h"
#include "btkdndcursors.h"
#include "btkalias.h"

static GSList *source_widgets = NULL;

typedef struct _BtkDragSourceSite BtkDragSourceSite;
typedef struct _BtkDragSourceInfo BtkDragSourceInfo;
typedef struct _BtkDragDestSite BtkDragDestSite;
typedef struct _BtkDragDestInfo BtkDragDestInfo;
typedef struct _BtkDragAnim BtkDragAnim;


typedef enum 
{
  BTK_DRAG_STATUS_DRAG,
  BTK_DRAG_STATUS_WAIT,
  BTK_DRAG_STATUS_DROP
} BtkDragStatus;

struct _BtkDragSourceSite 
{
  BdkModifierType    start_button_mask;
  BtkTargetList     *target_list;        /* Targets for drag data */
  BdkDragAction      actions;            /* Possible actions */

  /* Drag icon */
  BtkImageType icon_type;
  union
  {
    BtkImagePixmapData pixmap;
    BtkImagePixbufData pixbuf;
    BtkImageStockData stock;
    BtkImageIconNameData name;
  } icon_data;
  BdkBitmap *icon_mask;

  BdkColormap       *colormap;	         /* Colormap for drag icon */

  /* Stored button press information to detect drag beginning */
  gint               state;
  gint               x, y;
};
  
struct _BtkDragSourceInfo 
{
  BtkWidget         *widget;
  BtkTargetList     *target_list; /* Targets for drag data */
  BdkDragAction      possible_actions; /* Actions allowed by source */
  BdkDragContext    *context;	  /* drag context */
  BtkWidget         *icon_window; /* Window for drag */
  BtkWidget         *fallback_icon; /* Window for drag used on other screens */
  BtkWidget         *ipc_widget;  /* BtkInvisible for grab, message passing */
  BdkCursor         *cursor;	  /* Cursor for drag */
  gint hot_x, hot_y;		  /* Hot spot for drag */
  gint button;			  /* mouse button starting drag */

  BtkDragStatus      status;	  /* drag status */
  BdkEvent          *last_event;  /* pending event */

  gint               start_x, start_y; /* Initial position */
  gint               cur_x, cur_y;     /* Current Position */
  BdkScreen         *cur_screen;       /* Current screen for pointer */

  guint32            grab_time;   /* timestamp for initial grab */
  GList             *selections;  /* selections we've claimed */
  
  BtkDragDestInfo   *proxy_dest;  /* Set if this is a proxy drag */

  guint              update_idle;      /* Idle function to update the drag */
  guint              drop_timeout;     /* Timeout for aborting drop */
  guint              destroy_icon : 1; /* If true, destroy icon_window
      				        */
  guint              have_grab : 1;    /* Do we still have the pointer grab
				        */
  BdkPixbuf         *icon_pixbuf;
  BdkCursor         *drag_cursors[6];
};

struct _BtkDragDestSite 
{
  BtkDestDefaults    flags;
  BtkTargetList     *target_list;
  BdkDragAction      actions;
  BdkWindow         *proxy_window;
  BdkDragProtocol    proxy_protocol;
  guint              do_proxy : 1;
  guint              proxy_coords : 1;
  guint              have_drag : 1;
  guint              track_motion : 1;
};
  
struct _BtkDragDestInfo 
{
  BtkWidget         *widget;	   /* Widget in which drag is in */
  BdkDragContext    *context;	   /* Drag context */
  BtkDragSourceInfo *proxy_source; /* Set if this is a proxy drag */
  BtkSelectionData  *proxy_data;   /* Set while retrieving proxied data */
  guint              dropped : 1;     /* Set after we receive a drop */
  guint32            proxy_drop_time; /* Timestamp for proxied drop */
  guint              proxy_drop_wait : 1; /* Set if we are waiting for a
					   * status reply before sending
					   * a proxied drop on.
					   */
  gint               drop_x, drop_y; /* Position of drop */
};

#define DROP_ABORT_TIME 300000

#define ANIM_STEP_TIME 50
#define ANIM_STEP_LENGTH 50
#define ANIM_MIN_STEPS 5
#define ANIM_MAX_STEPS 10

struct _BtkDragAnim 
{
  BtkDragSourceInfo *info;
  gint step;
  gint n_steps;
};

typedef gboolean (* BtkDragDestCallback) (BtkWidget      *widget,
                                          BdkDragContext *context,
                                          gint            x,
                                          gint            y,
                                          guint32         time);

/* Enumeration for some targets we handle internally */

enum {
  TARGET_MOTIF_SUCCESS = 0x40000000,
  TARGET_MOTIF_FAILURE,
  TARGET_DELETE
};

/* Drag icons */

static BdkPixmap   *default_icon_pixmap = NULL;
static BdkPixmap   *default_icon_mask = NULL;
static BdkColormap *default_icon_colormap = NULL;
static gint         default_icon_hot_x;
static gint         default_icon_hot_y;

/* Forward declarations */
static void          btk_drag_get_event_actions (BdkEvent        *event, 
					         gint             button,
					         BdkDragAction    actions,
					         BdkDragAction   *suggested_action,
					         BdkDragAction   *possible_actions);
static BdkCursor *   btk_drag_get_cursor         (BdkDisplay     *display,
						  BdkDragAction   action,
						  BtkDragSourceInfo *info);
static void          btk_drag_update_cursor      (BtkDragSourceInfo *info);
static BtkWidget    *btk_drag_get_ipc_widget            (BtkWidget *widget);
static BtkWidget    *btk_drag_get_ipc_widget_for_screen (BdkScreen *screen);
static void          btk_drag_release_ipc_widget (BtkWidget      *widget);

static gboolean      btk_drag_highlight_expose   (BtkWidget      *widget,
					  	  BdkEventExpose *event,
						  gpointer        data);

static void     btk_drag_selection_received     (BtkWidget        *widget,
						 BtkSelectionData *selection_data,
						 guint             time,
						 gpointer          data);
static gboolean btk_drag_find_widget            (BtkWidget        *widget,
                                                 BdkDragContext   *context,
                                                 BtkDragDestInfo  *info,
                                                 gint              x,
                                                 gint              y,
                                                 guint32           time,
                                                 BtkDragDestCallback callback);
static void     btk_drag_proxy_begin            (BtkWidget        *widget,
						 BtkDragDestInfo  *dest_info,
						 guint32           time);
static void     btk_drag_dest_realized          (BtkWidget        *widget);
static void     btk_drag_dest_hierarchy_changed (BtkWidget        *widget,
						 BtkWidget        *previous_toplevel);
static void     btk_drag_dest_site_destroy      (gpointer          data);
static void     btk_drag_dest_leave             (BtkWidget        *widget,
						 BdkDragContext   *context,
						 guint             time);
static gboolean btk_drag_dest_motion            (BtkWidget        *widget,
						 BdkDragContext   *context,
						 gint              x,
						 gint              y,
						 guint             time);
static gboolean btk_drag_dest_drop              (BtkWidget        *widget,
						 BdkDragContext   *context,
						 gint              x,
						 gint              y,
						 guint             time);

static BtkDragDestInfo *  btk_drag_get_dest_info     (BdkDragContext *context,
						      gboolean        create);
static BtkDragSourceInfo *btk_drag_get_source_info   (BdkDragContext *context,
						      gboolean        create);
static void               btk_drag_clear_source_info (BdkDragContext *context);

static void btk_drag_source_check_selection    (BtkDragSourceInfo *info, 
					        BdkAtom            selection,
					        guint32            time);
static void btk_drag_source_release_selections (BtkDragSourceInfo *info,
						guint32            time);
static void btk_drag_drop                      (BtkDragSourceInfo *info,
						guint32            time);
static void btk_drag_drop_finished             (BtkDragSourceInfo *info,
						BtkDragResult      result,
						guint              time);
static void btk_drag_cancel                    (BtkDragSourceInfo *info,
						BtkDragResult      result,
						guint32            time);

static gboolean btk_drag_source_event_cb       (BtkWidget         *widget,
						BdkEvent          *event,
						gpointer           data);
static void btk_drag_source_site_destroy       (gpointer           data);
static void btk_drag_selection_get             (BtkWidget         *widget, 
						BtkSelectionData  *selection_data,
						guint              sel_info,
						guint32            time,
						gpointer           data);
static gboolean btk_drag_anim_timeout          (gpointer           data);
static void btk_drag_remove_icon               (BtkDragSourceInfo *info);
static void btk_drag_source_info_destroy       (BtkDragSourceInfo *info);
static void btk_drag_add_update_idle           (BtkDragSourceInfo *info);

static void btk_drag_update                    (BtkDragSourceInfo *info,
						BdkScreen         *screen,
						gint               x_root,
						gint               y_root,
						BdkEvent          *event);
static gboolean btk_drag_motion_cb             (BtkWidget         *widget, 
					        BdkEventMotion    *event, 
					        gpointer           data);
static gboolean btk_drag_key_cb                (BtkWidget         *widget, 
					        BdkEventKey       *event, 
					        gpointer           data);
static gboolean btk_drag_grab_broken_event_cb  (BtkWidget          *widget,
						BdkEventGrabBroken *event,
						gpointer            data);
static void     btk_drag_grab_notify_cb        (BtkWidget         *widget,
						gboolean           was_grabbed,
						gpointer           data);
static gboolean btk_drag_button_release_cb     (BtkWidget         *widget, 
					        BdkEventButton    *event, 
					        gpointer           data);
static gboolean btk_drag_abort_timeout         (gpointer           data);

static void     set_icon_stock_pixbuf          (BdkDragContext    *context,
						const gchar       *stock_id,
						BdkPixbuf         *pixbuf,
						gint               hot_x,
						gint               hot_y,
						gboolean           force_window);

/************************
 * Cursor and Icon data *
 ************************/

static struct {
  BdkDragAction action;
  const gchar  *name;
  const guint8 *data;
  BdkPixbuf    *pixbuf;
  BdkCursor    *cursor;
} drag_cursors[] = {
  { BDK_ACTION_DEFAULT, NULL },
  { BDK_ACTION_ASK,   "dnd-ask",  dnd_cursor_ask,  NULL, NULL },
  { BDK_ACTION_COPY,  "dnd-copy", dnd_cursor_copy, NULL, NULL },
  { BDK_ACTION_MOVE,  "dnd-move", dnd_cursor_move, NULL, NULL },
  { BDK_ACTION_LINK,  "dnd-link", dnd_cursor_link, NULL, NULL },
  { 0              ,  "dnd-none", dnd_cursor_none, NULL, NULL },
};

static const gint n_drag_cursors = sizeof (drag_cursors) / sizeof (drag_cursors[0]);

/*********************
 * Utility functions *
 *********************/

static void
set_can_change_screen (BtkWidget *widget,
		       gboolean   can_change_screen)
{
  can_change_screen = can_change_screen != FALSE;
  
  g_object_set_data (G_OBJECT (widget), I_("btk-dnd-can-change-screen"),
		     GUINT_TO_POINTER (can_change_screen));
}

static gboolean
get_can_change_screen (BtkWidget *widget)
{
  return g_object_get_data (G_OBJECT (widget), "btk-dnd-can-change-screen") != NULL;

}

static BtkWidget *
btk_drag_get_ipc_widget_for_screen (BdkScreen *screen)
{
  BtkWidget *result;
  GSList *drag_widgets = g_object_get_data (G_OBJECT (screen), 
					    "btk-dnd-ipc-widgets");
  
  if (drag_widgets)
    {
      GSList *tmp = drag_widgets;
      result = drag_widgets->data;
      drag_widgets = drag_widgets->next;
      g_object_set_data (G_OBJECT (screen),
			 I_("btk-dnd-ipc-widgets"),
			 drag_widgets);
      g_slist_free_1 (tmp);
    }
  else
    {
      result = btk_window_new (BTK_WINDOW_POPUP);
      btk_window_set_screen (BTK_WINDOW (result), screen);
      btk_window_resize (BTK_WINDOW (result), 1, 1);
      btk_window_move (BTK_WINDOW (result), -100, -100);
      btk_widget_show (result);
    }  

  return result;
}

static BtkWidget *
btk_drag_get_ipc_widget (BtkWidget *widget)
{
  BtkWidget *result;
  BtkWidget *toplevel;

  result = btk_drag_get_ipc_widget_for_screen (btk_widget_get_screen (widget));
  
  toplevel = btk_widget_get_toplevel (widget);
  
  if (BTK_IS_WINDOW (toplevel))
    {
      if (BTK_WINDOW (toplevel)->group)
	btk_window_group_add_window (BTK_WINDOW (toplevel)->group, 
                                     BTK_WINDOW (result));
    }

  return result;
}


#ifdef BDK_WINDOWING_X11

/*
 * We want to handle a handful of keys during DND, e.g. Escape to abort.
 * Grabbing the keyboard has the unfortunate side-effect of preventing
 * useful things such as using Alt-Tab to cycle between windows or
 * switching workspaces. Therefore, we just grab the few keys we are
 * interested in. Note that we need to put the grabs on the root window
 * in order for them to still work when the focus is moved to another
 * app/workspace.
 *
 * BDK needs a little help to successfully deliver root key events...
 */

static BdkFilterReturn
root_key_filter (BdkXEvent *xevent,
                 BdkEvent  *event,
                 gpointer   data)
{
  XEvent *ev = (XEvent *)xevent;

  if ((ev->type == KeyPress || ev->type == KeyRelease) &&
      ev->xkey.root == ev->xkey.window)
    ev->xkey.window = (Window)data;

  return BDK_FILTER_CONTINUE;
}

typedef struct {
  gint keysym;
  gint modifiers;
} GrabKey;

static GrabKey grab_keys[] = {
  { XK_Escape, 0 },
  { XK_space, 0 },
  { XK_KP_Space, 0 },
  { XK_Return, 0 },
  { XK_KP_Enter, 0 },
  { XK_Up, 0 },
  { XK_Up, Mod1Mask },
  { XK_Down, 0 },
  { XK_Down, Mod1Mask },
  { XK_Left, 0 },
  { XK_Left, Mod1Mask },
  { XK_Right, 0 },
  { XK_Right, Mod1Mask },
  { XK_KP_Up, 0 },
  { XK_KP_Up, Mod1Mask },
  { XK_KP_Down, 0 },
  { XK_KP_Down, Mod1Mask },
  { XK_KP_Left, 0 },
  { XK_KP_Left, Mod1Mask },
  { XK_KP_Right, 0 },
  { XK_KP_Right, Mod1Mask }
};

static void
grab_dnd_keys (BtkWidget *widget,
               guint32    time)
{
  guint i;
  BdkWindow *window, *root;
  gint keycode;

  window = widget->window;
  root = bdk_screen_get_root_window (btk_widget_get_screen (widget));

  bdk_error_trap_push ();

  for (i = 0; i < G_N_ELEMENTS (grab_keys); ++i)
    {
      keycode = XKeysymToKeycode (BDK_WINDOW_XDISPLAY (window), grab_keys[i].keysym);
      if (keycode == NoSymbol)
        continue;
      XGrabKey (BDK_WINDOW_XDISPLAY (window),
   	        keycode, grab_keys[i].modifiers,
	        BDK_WINDOW_XID (root),
	        FALSE,
	        GrabModeAsync,
	        GrabModeAsync);
    }

  bdk_flush ();
  bdk_error_trap_pop ();

  bdk_window_add_filter (NULL, root_key_filter, (gpointer) BDK_WINDOW_XID (window));
}

static void
ungrab_dnd_keys (BtkWidget *widget,
                 guint32    time)
{
  guint i;
  BdkWindow *window, *root;
  gint keycode;

  window = widget->window;
  root = bdk_screen_get_root_window (btk_widget_get_screen (widget));

  bdk_window_remove_filter (NULL, root_key_filter, (gpointer) BDK_WINDOW_XID (window));

  bdk_error_trap_push ();

  for (i = 0; i < G_N_ELEMENTS (grab_keys); ++i)
    {
      keycode = XKeysymToKeycode (BDK_WINDOW_XDISPLAY (window), grab_keys[i].keysym);
      if (keycode == NoSymbol)
        continue;
      XUngrabKey (BDK_WINDOW_XDISPLAY (window),
      	          keycode, grab_keys[i].modifiers,
                  BDK_WINDOW_XID (root));
    }

  bdk_flush ();
  bdk_error_trap_pop ();
}

#else

static void
grab_dnd_keys (BtkWidget *widget,
               guint32    time)
{
  bdk_keyboard_grab (widget->window, FALSE, time);
}

static void
ungrab_dnd_keys (BtkWidget *widget,
                 guint32    time)
{
  bdk_display_keyboard_ungrab (btk_widget_get_display (widget), time);
}

#endif


/***************************************************************
 * btk_drag_release_ipc_widget:
 *     Releases widget retrieved with btk_drag_get_ipc_widget ()
 *   arguments:
 *     widget: the widget to release.
 *   results:
 ***************************************************************/

static void
btk_drag_release_ipc_widget (BtkWidget *widget)
{
  BtkWindow *window = BTK_WINDOW (widget);
  BdkScreen *screen = btk_widget_get_screen (widget);
  GSList *drag_widgets = g_object_get_data (G_OBJECT (screen),
					    "btk-dnd-ipc-widgets");
  ungrab_dnd_keys (widget, BDK_CURRENT_TIME);
  if (window->group)
    btk_window_group_remove_window (window->group, window);
  drag_widgets = g_slist_prepend (drag_widgets, widget);
  g_object_set_data (G_OBJECT (screen),
		     I_("btk-dnd-ipc-widgets"),
		     drag_widgets);
}

static guint32
btk_drag_get_event_time (BdkEvent *event)
{
  guint32 tm = BDK_CURRENT_TIME;
  
  if (event)
    switch (event->type)
      {
      case BDK_MOTION_NOTIFY:
	tm = event->motion.time; break;
      case BDK_BUTTON_PRESS:
      case BDK_2BUTTON_PRESS:
      case BDK_3BUTTON_PRESS:
      case BDK_BUTTON_RELEASE:
	tm = event->button.time; break;
      case BDK_KEY_PRESS:
      case BDK_KEY_RELEASE:
	tm = event->key.time; break;
      case BDK_ENTER_NOTIFY:
      case BDK_LEAVE_NOTIFY:
	tm = event->crossing.time; break;
      case BDK_PROPERTY_NOTIFY:
	tm = event->property.time; break;
      case BDK_SELECTION_CLEAR:
      case BDK_SELECTION_REQUEST:
      case BDK_SELECTION_NOTIFY:
	tm = event->selection.time; break;
      case BDK_PROXIMITY_IN:
      case BDK_PROXIMITY_OUT:
	tm = event->proximity.time; break;
      default:			/* use current time */
	break;
      }
  
  return tm;
}

static void
btk_drag_get_event_actions (BdkEvent *event, 
			    gint button, 
			    BdkDragAction  actions,
			    BdkDragAction *suggested_action,
			    BdkDragAction *possible_actions)
{
  *suggested_action = 0;
  *possible_actions = 0;

  if (event)
    {
      BdkModifierType state = 0;
      
      switch (event->type)
	{
	case BDK_MOTION_NOTIFY:
	  state = event->motion.state;
	  break;
	case BDK_BUTTON_PRESS:
	case BDK_2BUTTON_PRESS:
	case BDK_3BUTTON_PRESS:
	case BDK_BUTTON_RELEASE:
	  state = event->button.state;
	  break;
	case BDK_KEY_PRESS:
	case BDK_KEY_RELEASE:
	  state = event->key.state;
	  break;
	case BDK_ENTER_NOTIFY:
	case BDK_LEAVE_NOTIFY:
	  state = event->crossing.state;
	  break;
	default:
	  break;
	}

      if ((button == 2 || button == 3) && (actions & BDK_ACTION_ASK))
	{
	  *suggested_action = BDK_ACTION_ASK;
	  *possible_actions = actions;
	}
      else if (state & (BDK_SHIFT_MASK | BDK_CONTROL_MASK))
	{
	  if ((state & BDK_SHIFT_MASK) && (state & BDK_CONTROL_MASK))
	    {
	      if (actions & BDK_ACTION_LINK)
		{
		  *suggested_action = BDK_ACTION_LINK;
		  *possible_actions = BDK_ACTION_LINK;
		}
	    }
	  else if (state & BDK_CONTROL_MASK)
	    {
	      if (actions & BDK_ACTION_COPY)
		{
		  *suggested_action = BDK_ACTION_COPY;
		  *possible_actions = BDK_ACTION_COPY;
		}
	      return;
	    }
	  else
	    {
	      if (actions & BDK_ACTION_MOVE)
		{
		  *suggested_action = BDK_ACTION_MOVE;
		  *possible_actions = BDK_ACTION_MOVE;
		}
	      return;
	    }
	}
      else
	{
	  *possible_actions = actions;

	  if ((state & (BDK_MOD1_MASK)) && (actions & BDK_ACTION_ASK))
	    *suggested_action = BDK_ACTION_ASK;
	  else if (actions & BDK_ACTION_COPY)
	    *suggested_action =  BDK_ACTION_COPY;
	  else if (actions & BDK_ACTION_MOVE)
	    *suggested_action = BDK_ACTION_MOVE;
	  else if (actions & BDK_ACTION_LINK)
	    *suggested_action = BDK_ACTION_LINK;
	}
    }
  else
    {
      *possible_actions = actions;
      
      if (actions & BDK_ACTION_COPY)
	*suggested_action =  BDK_ACTION_COPY;
      else if (actions & BDK_ACTION_MOVE)
	*suggested_action = BDK_ACTION_MOVE;
      else if (actions & BDK_ACTION_LINK)
	*suggested_action = BDK_ACTION_LINK;
    }
}

static gboolean
btk_drag_can_use_rgba_cursor (BdkDisplay *display, 
			      gint        width,
			      gint        height)
{
  guint max_width, max_height;
  
  if (!bdk_display_supports_cursor_color (display))
    return FALSE;

  if (!bdk_display_supports_cursor_alpha (display))
    return FALSE;

  bdk_display_get_maximal_cursor_size (display, 
                                       &max_width,
                                       &max_height);
  if (width > max_width || height > max_height)
    {
       /* can't use rgba cursor (too large) */
      return FALSE;
    }

  return TRUE;
}

static BdkCursor *
btk_drag_get_cursor (BdkDisplay        *display,
		     BdkDragAction      action,
		     BtkDragSourceInfo *info)
{
  gint i;

  /* reconstruct the cursors for each new drag (thus !info),
   * to catch cursor theme changes 
   */ 
  if (!info)
    {
      for (i = 0 ; i < n_drag_cursors - 1; i++)
	if (drag_cursors[i].cursor != NULL)
	  {
	    bdk_cursor_unref (drag_cursors[i].cursor);
	    drag_cursors[i].cursor = NULL;
	  }
    }
 
  for (i = 0 ; i < n_drag_cursors - 1; i++)
    if (drag_cursors[i].action == action)
      break;

  if (drag_cursors[i].pixbuf == NULL)
    drag_cursors[i].pixbuf = 
      bdk_pixbuf_new_from_inline (-1, drag_cursors[i].data, FALSE, NULL);

  if (drag_cursors[i].cursor != NULL)
    {
      if (display != bdk_cursor_get_display (drag_cursors[i].cursor))
	{
	  bdk_cursor_unref (drag_cursors[i].cursor);
	  drag_cursors[i].cursor = NULL;
	}
    }
  
  if (drag_cursors[i].cursor == NULL)
    drag_cursors[i].cursor = bdk_cursor_new_from_name (display, drag_cursors[i].name);
  
  if (drag_cursors[i].cursor == NULL)
    drag_cursors[i].cursor = bdk_cursor_new_from_pixbuf (display, drag_cursors[i].pixbuf, 0, 0);

  if (info && info->icon_pixbuf) 
    {
      gint cursor_width, cursor_height;
      gint icon_width, icon_height;
      gint width, height;
      BdkPixbuf *cursor_pixbuf, *pixbuf;
      gint hot_x, hot_y;
      gint icon_x, icon_y, ref_x, ref_y;

      if (info->drag_cursors[i] != NULL)
        {
          if (display == bdk_cursor_get_display (info->drag_cursors[i]))
	    return info->drag_cursors[i];
	  
	  bdk_cursor_unref (info->drag_cursors[i]);
	  info->drag_cursors[i] = NULL;
        }

      icon_x = info->hot_x;
      icon_y = info->hot_y;
      icon_width = bdk_pixbuf_get_width (info->icon_pixbuf);
      icon_height = bdk_pixbuf_get_height (info->icon_pixbuf);

      hot_x = hot_y = 0;
      cursor_pixbuf = bdk_cursor_get_image (drag_cursors[i].cursor);
      if (!cursor_pixbuf)
	cursor_pixbuf = g_object_ref (drag_cursors[i].pixbuf);
      else
	{
	  if (bdk_pixbuf_get_option (cursor_pixbuf, "x_hot"))
	    hot_x = atoi (bdk_pixbuf_get_option (cursor_pixbuf, "x_hot"));
	  
	  if (bdk_pixbuf_get_option (cursor_pixbuf, "y_hot"))
	    hot_y = atoi (bdk_pixbuf_get_option (cursor_pixbuf, "y_hot"));

#if 0	  
	  /* The code below is an attempt to let cursor themes
	   * determine the attachment of the icon to enable things
	   * like the following:
	   *
	   *    +-----+
           *    |     |
           *    |     ||
           *    +-----+|
           *        ---+
           * 
           * It does not work since Xcursor doesn't allow to attach
           * any additional information to cursors in a retrievable
           * way  (there are comments, but no way to get at them
           * short of searching for the actual cursor file).
           * If this code ever gets used, the icon_window placement
           * must be changed to recognize these placement options
           * as well. Note that this code ignores info->hot_x/y.
           */ 
	  for (j = 0; j < 10; j++)
	    {
	      const gchar *opt;
	      gchar key[32];
	      gchar **toks;
	      BtkAnchorType icon_anchor;

	      g_snprintf (key, 32, "comment%d", j);
	      opt = bdk_pixbuf_get_option (cursor_pixbuf, key);
	      if (opt && g_str_has_prefix ("icon-attach:", opt))
		{
		  toks = g_strsplit (opt + strlen ("icon-attach:"), "'", -1);
		  if (g_strv_length (toks) != 3)
		    {
		      g_strfreev (toks);
		      break;
		    }
		  icon_anchor = atoi (toks[0]);
		  icon_x = atoi (toks[1]);
		  icon_y = atoi (toks[2]);
		  
		  switch (icon_anchor)
		    {
		    case BTK_ANCHOR_NORTH:
		    case BTK_ANCHOR_CENTER:
		    case BTK_ANCHOR_SOUTH:
		      icon_x += icon_width / 2;
		      break;
		    case BTK_ANCHOR_NORTH_EAST:
		    case BTK_ANCHOR_EAST:
		    case BTK_ANCHOR_SOUTH_EAST:
		      icon_x += icon_width;
		      break;
		    default: ;
		    }
		  
		  switch (icon_anchor)
		    {
		    case BTK_ANCHOR_WEST:
		    case BTK_ANCHOR_CENTER:
		    case BTK_ANCHOR_EAST:
		      icon_y += icon_height / 2;
		      break;
		    case BTK_ANCHOR_SOUTH_WEST:
		    case BTK_ANCHOR_SOUTH:
		    case BTK_ANCHOR_SOUTH_EAST:
		      icon_x += icon_height;
		      break;
		    default: ;
		    }

		  g_strfreev (toks);
		  break;
		}
	    }
#endif
	}

      cursor_width = bdk_pixbuf_get_width (cursor_pixbuf);
      cursor_height = bdk_pixbuf_get_height (cursor_pixbuf);
      
      ref_x = MAX (hot_x, icon_x);
      ref_y = MAX (hot_y, icon_y);
      width = ref_x + MAX (cursor_width - hot_x, icon_width - icon_x);
      height = ref_y + MAX (cursor_height - hot_y, icon_height - icon_y);
         
      if (btk_drag_can_use_rgba_cursor (display, width, height))
	{
	  /* Composite cursor and icon so that both hotspots
	   * end up at (ref_x, ref_y)
	   */
	  pixbuf = bdk_pixbuf_new (BDK_COLORSPACE_RGB, TRUE, 8,
				   width, height); 
	  
	  bdk_pixbuf_fill (pixbuf, 0xff000000);
	  
	  bdk_pixbuf_composite (info->icon_pixbuf, pixbuf,
				ref_x - icon_x, ref_y - icon_y, 
				icon_width, icon_height,
				ref_x - icon_x, ref_y - icon_y, 
				1.0, 1.0, 
				BDK_INTERP_BILINEAR, 255);
	  
	  bdk_pixbuf_composite (cursor_pixbuf, pixbuf,
				ref_x - hot_x, ref_y - hot_y, 
				cursor_width, cursor_height,
				ref_x - hot_x, ref_y - hot_y,
				1.0, 1.0, 
				BDK_INTERP_BILINEAR, 255);
	  
	  info->drag_cursors[i] = 
	    bdk_cursor_new_from_pixbuf (display, pixbuf, ref_x, ref_y);
	  
	  g_object_unref (pixbuf);
	}
      
      g_object_unref (cursor_pixbuf);
      
      if (info->drag_cursors[i] != NULL)
	return info->drag_cursors[i];
    }
 
  return drag_cursors[i].cursor;
}

static void
btk_drag_update_cursor (BtkDragSourceInfo *info)
{
  BdkCursor *cursor;
  gint i;

  if (!info->have_grab)
    return;

  for (i = 0 ; i < n_drag_cursors - 1; i++)
    if (info->cursor == drag_cursors[i].cursor ||
	info->cursor == info->drag_cursors[i])
      break;
  
  if (i == n_drag_cursors)
    return;

  cursor = btk_drag_get_cursor (bdk_cursor_get_display (info->cursor), 
				drag_cursors[i].action, info);
  
  if (cursor != info->cursor)
    {
      bdk_pointer_grab (info->ipc_widget->window, FALSE,
			BDK_POINTER_MOTION_MASK |
			BDK_BUTTON_RELEASE_MASK,
			NULL,
			cursor, info->grab_time);
      info->cursor = cursor;
    }
}

/********************
 * Destination side *
 ********************/

/*************************************************************
 * btk_drag_get_data:
 *     Get the data for a drag or drop
 *   arguments:
 *     context - drag context
 *     target  - format to retrieve the data in.
 *     time    - timestamp of triggering event.
 *     
 *   results:
 *************************************************************/

void 
btk_drag_get_data (BtkWidget      *widget,
		   BdkDragContext *context,
		   BdkAtom         target,
		   guint32         time)
{
  BtkWidget *selection_widget;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (BDK_IS_DRAG_CONTEXT (context));

  selection_widget = btk_drag_get_ipc_widget (widget);

  g_object_ref (context);
  g_object_ref (widget);
  
  g_signal_connect (selection_widget, "selection-received",
		    G_CALLBACK (btk_drag_selection_received), widget);

  g_object_set_data (G_OBJECT (selection_widget), I_("drag-context"), context);

  btk_selection_convert (selection_widget,
			 bdk_drag_get_selection (context),
			 target,
			 time);
}


/**
 * btk_drag_get_source_widget:
 * @context: a (destination side) drag context
 *
 * Determines the source widget for a drag.
 *
 * Return value: (transfer none): if the drag is occurring
 *     within a single application, a pointer to the source widget.
 *     Otherwise, %NULL.
 */
BtkWidget *
btk_drag_get_source_widget (BdkDragContext *context)
{
  GSList *tmp_list;

  g_return_val_if_fail (BDK_IS_DRAG_CONTEXT (context), NULL);
  
  tmp_list = source_widgets;
  while (tmp_list)
    {
      BtkWidget *ipc_widget = tmp_list->data;

      if (btk_widget_get_window (ipc_widget) == bdk_drag_context_get_source_window (context))
	{
	  BtkDragSourceInfo *info;
	  info = g_object_get_data (G_OBJECT (ipc_widget), "btk-info");

	  return info ? info->widget : NULL;
	}

      tmp_list = tmp_list->next;
    }

  return NULL;
}

/*************************************************************
 * btk_drag_finish:
 *     Notify the drag source that the transfer of data
 *     is complete.
 *   arguments:
 *     context: The drag context for this drag
 *     success: Was the data successfully transferred?
 *     time:    The timestamp to use when notifying the destination.
 *   results:
 *************************************************************/

void 
btk_drag_finish (BdkDragContext *context,
		 gboolean        success,
		 gboolean        del,
		 guint32         time)
{
  BdkAtom target = BDK_NONE;

  g_return_if_fail (BDK_IS_DRAG_CONTEXT (context));

  if (success && del)
    {
      target = bdk_atom_intern_static_string ("DELETE");
    }
  else if (bdk_drag_context_get_protocol (context) == BDK_DRAG_PROTO_MOTIF)
    {
      target = bdk_atom_intern_static_string (success ? 
					      "XmTRANSFER_SUCCESS" : 
					      "XmTRANSFER_FAILURE");
    }

  if (target != BDK_NONE)
    {
      BtkWidget *selection_widget = btk_drag_get_ipc_widget_for_screen (bdk_window_get_screen (bdk_drag_context_get_source_window (context)));

      g_object_ref (context);
      
      g_object_set_data (G_OBJECT (selection_widget), I_("drag-context"), context);
      g_signal_connect (selection_widget, "selection-received",
			G_CALLBACK (btk_drag_selection_received),
			NULL);
      
      btk_selection_convert (selection_widget,
			     bdk_drag_get_selection (context),
			     target,
			     time);
    }
  
  if (!(success && del))
    bdk_drop_finish (context, success, time);
}

/*************************************************************
 * btk_drag_highlight_expose:
 *     Callback for expose_event for highlighted widgets.
 *   arguments:
 *     widget:
 *     event:
 *     data:
 *   results:
 *************************************************************/

static gboolean
btk_drag_highlight_expose (BtkWidget      *widget,
			   BdkEventExpose *event,
			   gpointer        data)
{
  gint x, y, width, height;
  
  if (btk_widget_is_drawable (widget))
    {
      bairo_t *cr;
      
      if (!btk_widget_get_has_window (widget))
	{
	  x = widget->allocation.x;
	  y = widget->allocation.y;
	  width = widget->allocation.width;
	  height = widget->allocation.height;
	}
      else
	{
	  x = 0;
	  y = 0;
          width = bdk_window_get_width (widget->window);
          height = bdk_window_get_height (widget->window);
	}
      
      btk_paint_shadow (widget->style, widget->window,
		        BTK_STATE_NORMAL, BTK_SHADOW_OUT,
		        &event->area, widget, "dnd",
			x, y, width, height);

      cr = bdk_bairo_create (widget->window);
      bairo_set_source_rgb (cr, 0.0, 0.0, 0.0); /* black */
      bairo_set_line_width (cr, 1.0);
      bairo_rectangle (cr,
		       x + 0.5, y + 0.5,
		       width - 1, height - 1);
      bairo_stroke (cr);
      bairo_destroy (cr);
    }

  return FALSE;
}

/*************************************************************
 * btk_drag_highlight:
 *     Highlight the given widget in the default manner.
 *   arguments:
 *     widget:
 *   results:
 *************************************************************/

void 
btk_drag_highlight (BtkWidget  *widget)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  g_signal_connect_after (widget, "expose-event",
			  G_CALLBACK (btk_drag_highlight_expose),
			  NULL);

  btk_widget_queue_draw (widget);
}

/*************************************************************
 * btk_drag_unhighlight:
 *     Refresh the given widget to remove the highlight.
 *   arguments:
 *     widget:
 *   results:
 *************************************************************/

void 
btk_drag_unhighlight (BtkWidget *widget)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  g_signal_handlers_disconnect_by_func (widget,
					btk_drag_highlight_expose,
					NULL);
  
  btk_widget_queue_draw (widget);
}

static void
btk_drag_dest_set_internal (BtkWidget       *widget,
			    BtkDragDestSite *site)
{
  BtkDragDestSite *old_site;
  
  g_return_if_fail (widget != NULL);

  /* HACK, do this in the destroy */
  old_site = g_object_get_data (G_OBJECT (widget), "btk-drag-dest");
  if (old_site)
    {
      g_signal_handlers_disconnect_by_func (widget,
					    btk_drag_dest_realized,
					    old_site);
      g_signal_handlers_disconnect_by_func (widget,
					    btk_drag_dest_hierarchy_changed,
					    old_site);

      site->track_motion = old_site->track_motion;
    }

  if (btk_widget_get_realized (widget))
    btk_drag_dest_realized (widget);

  g_signal_connect (widget, "realize",
		    G_CALLBACK (btk_drag_dest_realized), site);
  g_signal_connect (widget, "hierarchy-changed",
		    G_CALLBACK (btk_drag_dest_hierarchy_changed), site);

  g_object_set_data_full (G_OBJECT (widget), I_("btk-drag-dest"),
			  site, btk_drag_dest_site_destroy);
}

/**
 * btk_drag_dest_set:
 * @widget: a #BtkWidget
 * @flags: which types of default drag behavior to use
 * @targets: (allow-none) (array length=n_targets): a pointer to an array of #BtkTargetEntry<!-- -->s
 *     indicating the drop types that this @widget will accept, or %NULL.
 *     Later you can access the list with btk_drag_dest_get_target_list()
 *     and btk_drag_dest_find_target().
 * @n_targets: the number of entries in @targets
 * @actions: a bitmask of possible actions for a drop onto this @widget.
 *
 * Sets a widget as a potential drop destination, and adds default behaviors.
 *
 * The default behaviors listed in @flags have an effect similar
 * to installing default handlers for the widget's drag-and-drop signals
 * (#BtkWidget:drag-motion, #BtkWidget:drag-drop, ...). They all exist
 * for convenience. When passing #BTK_DEST_DEFAULT_ALL for instance it is
 * sufficient to connect to the widget's #BtkWidget::drag-data-received
 * signal to get primitive, but consistent drag-and-drop support.
 *
 * Things become more complicated when you try to preview the dragged data,
 * as described in the documentation for #BtkWidget:drag-motion. The default
 * behaviors described by @flags make some assumptions, that can conflict
 * with your own signal handlers. For instance #BTK_DEST_DEFAULT_DROP causes
 * invokations of bdk_drag_status() in the context of #BtkWidget:drag-motion,
 * and invokations of btk_drag_finish() in #BtkWidget:drag-data-received.
 * Especially the later is dramatic, when your own #BtkWidget:drag-motion
 * handler calls btk_drag_get_data() to inspect the dragged data.
 *
 * There's no way to set a default action here, you can use the
 * #BtkWidget:drag-motion callback for that. Here's an example which selects
 * the action to use depending on whether the control key is pressed or not:
 * |[
 * static void
 * drag_motion (BtkWidget *widget,
 *              BdkDragContext *context,
 *              gint x,
 *              gint y,
 *              guint time)
 * {
 *   BdkModifierType mask;
 *
 *   bdk_window_get_pointer (btk_widget_get_window (widget),
 *                           NULL, NULL, &mask);
 *   if (mask & BDK_CONTROL_MASK)
 *     bdk_drag_status (context, BDK_ACTION_COPY, time);
 *   else
 *     bdk_drag_status (context, BDK_ACTION_MOVE, time);
 * }
 * ]|
 */
void
btk_drag_dest_set (BtkWidget            *widget,
		   BtkDestDefaults       flags,
		   const BtkTargetEntry *targets,
		   gint                  n_targets,
		   BdkDragAction         actions)
{
  BtkDragDestSite *site;
  
  g_return_if_fail (BTK_IS_WIDGET (widget));

  site = g_new (BtkDragDestSite, 1);

  site->flags = flags;
  site->have_drag = FALSE;
  if (targets)
    site->target_list = btk_target_list_new (targets, n_targets);
  else
    site->target_list = NULL;
  site->actions = actions;
  site->do_proxy = FALSE;
  site->proxy_window = NULL;
  site->track_motion = FALSE;

  btk_drag_dest_set_internal (widget, site);
}

/*************************************************************
 * btk_drag_dest_set_proxy:
 *     Set up this widget to proxy drags elsewhere.
 *   arguments:
 *     widget:          
 *     proxy_window:    window to which forward drag events
 *     protocol:        Drag protocol which the dest widget accepts
 *     use_coordinates: If true, send the same coordinates to the
 *                      destination, because it is a embedded 
 *                      subwindow.
 *   results:
 *************************************************************/

void 
btk_drag_dest_set_proxy (BtkWidget      *widget,
			 BdkWindow      *proxy_window,
			 BdkDragProtocol protocol,
			 gboolean        use_coordinates)
{
  BtkDragDestSite *site;
  
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (!proxy_window || BDK_IS_WINDOW (proxy_window));

  site = g_new (BtkDragDestSite, 1);

  site->flags = 0;
  site->have_drag = FALSE;
  site->target_list = NULL;
  site->actions = 0;
  site->proxy_window = proxy_window;
  if (proxy_window)
    g_object_ref (proxy_window);
  site->do_proxy = TRUE;
  site->proxy_protocol = protocol;
  site->proxy_coords = use_coordinates;
  site->track_motion = FALSE;

  btk_drag_dest_set_internal (widget, site);
}

/*************************************************************
 * btk_drag_dest_unset
 *     Unregister this widget as a drag target.
 *   arguments:
 *     widget:
 *   results:
 *************************************************************/

void 
btk_drag_dest_unset (BtkWidget *widget)
{
  BtkDragDestSite *old_site;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  old_site = g_object_get_data (G_OBJECT (widget),
                                "btk-drag-dest");
  if (old_site)
    {
      g_signal_handlers_disconnect_by_func (widget,
                                            btk_drag_dest_realized,
                                            old_site);
      g_signal_handlers_disconnect_by_func (widget,
                                            btk_drag_dest_hierarchy_changed,
                                            old_site);
    }

  g_object_set_data (G_OBJECT (widget), I_("btk-drag-dest"), NULL);
}

/**
 * btk_drag_dest_get_target_list:
 * @widget: a #BtkWidget
 * 
 * Returns the list of targets this widget can accept from
 * drag-and-drop.
 * 
 * Return value: (transfer none): the #BtkTargetList, or %NULL if none
 **/
BtkTargetList*
btk_drag_dest_get_target_list (BtkWidget *widget)
{
  BtkDragDestSite *site;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);
  
  site = g_object_get_data (G_OBJECT (widget), "btk-drag-dest");

  return site ? site->target_list : NULL;  
}

/**
 * btk_drag_dest_set_target_list:
 * @widget: a #BtkWidget that's a drag destination
 * @target_list: (allow-none): list of droppable targets, or %NULL for none
 * 
 * Sets the target types that this widget can accept from drag-and-drop.
 * The widget must first be made into a drag destination with
 * btk_drag_dest_set().
 **/
void
btk_drag_dest_set_target_list (BtkWidget      *widget,
                               BtkTargetList  *target_list)
{
  BtkDragDestSite *site;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  
  site = g_object_get_data (G_OBJECT (widget), "btk-drag-dest");
  
  if (!site)
    {
      g_warning ("Can't set a target list on a widget until you've called btk_drag_dest_set() "
                 "to make the widget into a drag destination");
      return;
    }

  if (target_list)
    btk_target_list_ref (target_list);
  
  if (site->target_list)
    btk_target_list_unref (site->target_list);

  site->target_list = target_list;
}

/**
 * btk_drag_dest_add_text_targets:
 * @widget: a #BtkWidget that's a drag destination
 *
 * Add the text targets supported by #BtkSelection to
 * the target list of the drag destination. The targets
 * are added with @info = 0. If you need another value, 
 * use btk_target_list_add_text_targets() and
 * btk_drag_dest_set_target_list().
 * 
 * Since: 2.6
 **/
void
btk_drag_dest_add_text_targets (BtkWidget *widget)
{
  BtkTargetList *target_list;

  target_list = btk_drag_dest_get_target_list (widget);
  if (target_list)
    btk_target_list_ref (target_list);
  else
    target_list = btk_target_list_new (NULL, 0);
  btk_target_list_add_text_targets (target_list, 0);
  btk_drag_dest_set_target_list (widget, target_list);
  btk_target_list_unref (target_list);
}

/**
 * btk_drag_dest_add_image_targets:
 * @widget: a #BtkWidget that's a drag destination
 *
 * Add the image targets supported by #BtkSelection to
 * the target list of the drag destination. The targets
 * are added with @info = 0. If you need another value, 
 * use btk_target_list_add_image_targets() and
 * btk_drag_dest_set_target_list().
 * 
 * Since: 2.6
 **/
void
btk_drag_dest_add_image_targets (BtkWidget *widget)
{
  BtkTargetList *target_list;

  target_list = btk_drag_dest_get_target_list (widget);
  if (target_list)
    btk_target_list_ref (target_list);
  else
    target_list = btk_target_list_new (NULL, 0);
  btk_target_list_add_image_targets (target_list, 0, FALSE);
  btk_drag_dest_set_target_list (widget, target_list);
  btk_target_list_unref (target_list);
}

/**
 * btk_drag_dest_add_uri_targets:
 * @widget: a #BtkWidget that's a drag destination
 *
 * Add the URI targets supported by #BtkSelection to
 * the target list of the drag destination. The targets
 * are added with @info = 0. If you need another value, 
 * use btk_target_list_add_uri_targets() and
 * btk_drag_dest_set_target_list().
 * 
 * Since: 2.6
 **/
void
btk_drag_dest_add_uri_targets (BtkWidget *widget)
{
  BtkTargetList *target_list;

  target_list = btk_drag_dest_get_target_list (widget);
  if (target_list)
    btk_target_list_ref (target_list);
  else
    target_list = btk_target_list_new (NULL, 0);
  btk_target_list_add_uri_targets (target_list, 0);
  btk_drag_dest_set_target_list (widget, target_list);
  btk_target_list_unref (target_list);
}

/**
 * btk_drag_dest_set_track_motion:
 * @widget: a #BtkWidget that's a drag destination
 * @track_motion: whether to accept all targets
 * 
 * Tells the widget to emit ::drag-motion and ::drag-leave
 * events regardless of the targets and the %BTK_DEST_DEFAULT_MOTION
 * flag. 
 *
 * This may be used when a widget wants to do generic
 * actions regardless of the targets that the source offers.
 *
 * Since: 2.10
 **/
void
btk_drag_dest_set_track_motion (BtkWidget *widget,
				gboolean   track_motion)
{
  BtkDragDestSite *site;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  site = g_object_get_data (G_OBJECT (widget), "btk-drag-dest");
  
  g_return_if_fail (site != NULL);

  site->track_motion = track_motion != FALSE;
}

/**
 * btk_drag_dest_get_track_motion:
 * @widget: a #BtkWidget that's a drag destination
 * 
 * Returns whether the widget has been configured to always
 * emit ::drag-motion signals.
 * 
 * Return Value: %TRUE if the widget always emits ::drag-motion events
 *
 * Since: 2.10
 **/
gboolean
btk_drag_dest_get_track_motion (BtkWidget *widget)
{
  BtkDragDestSite *site;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  site = g_object_get_data (G_OBJECT (widget), "btk-drag-dest");

  if (site)
    return site->track_motion;

  return FALSE;
}

/*************************************************************
 * _btk_drag_dest_handle_event:
 *     Called from widget event handling code on Drag events
 *     for destinations.
 *
 *   arguments:
 *     toplevel: Toplevel widget that received the event
 *     event:
 *   results:
 *************************************************************/

void
_btk_drag_dest_handle_event (BtkWidget *toplevel,
			    BdkEvent  *event)
{
  BtkDragDestInfo *info;
  BdkDragContext *context;

  g_return_if_fail (toplevel != NULL);
  g_return_if_fail (event != NULL);

  context = event->dnd.context;

  info = btk_drag_get_dest_info (context, TRUE);

  /* Find the widget for the event */
  switch (event->type)
    {
    case BDK_DRAG_ENTER:
      break;
      
    case BDK_DRAG_LEAVE:
      if (info->widget)
	{
	  btk_drag_dest_leave (info->widget, context, event->dnd.time);
	  info->widget = NULL;
	}
      break;
      
    case BDK_DRAG_MOTION:
    case BDK_DROP_START:
      {
	gint tx, ty;
        gboolean found;

	if (event->type == BDK_DROP_START)
	  {
	    info->dropped = TRUE;
	    /* We send a leave here so that the widget unhighlights
	     * properly.
	     */
	    if (info->widget)
	      {
		btk_drag_dest_leave (info->widget, context, event->dnd.time);
		info->widget = NULL;
	      }
	  }

#ifdef BDK_WINDOWING_X11
	/* Hackaround for: http://bugzilla.gnome.org/show_bug.cgi?id=136112
	 *
	 * Currently bdk_window_get_position doesn't provide reliable
	 * information for embedded windows, so we call the much more
	 * expensive bdk_window_get_origin().
	 */
	if (BTK_IS_PLUG (toplevel))
	  bdk_window_get_origin (toplevel->window, &tx, &ty);
	else
#endif /* BDK_WINDOWING_X11 */
	  bdk_window_get_position (toplevel->window, &tx, &ty);

	found = btk_drag_find_widget (toplevel,
                                      context,
                                      info,
                                      event->dnd.x_root - tx,
                                      event->dnd.y_root - ty,
                                      event->dnd.time,
                                      (event->type == BDK_DRAG_MOTION) ?
                                      btk_drag_dest_motion :
                                      btk_drag_dest_drop);

	if (info->widget && !found)
	  {
	    btk_drag_dest_leave (info->widget, context, event->dnd.time);
	    info->widget = NULL;
	  }
	
	/* Send a reply.
	 */
	if (event->type == BDK_DRAG_MOTION)
	  {
	    if (!found)
	      bdk_drag_status (context, 0, event->dnd.time);
	  }
	else if (event->type == BDK_DROP_START && !info->proxy_source)
	  {
	    bdk_drop_reply (context, found, event->dnd.time);
            if ((bdk_drag_context_get_protocol (context) == BDK_DRAG_PROTO_MOTIF) && !found)
	      btk_drag_finish (context, FALSE, FALSE, event->dnd.time);
	  }
      }
      break;

    default:
      g_assert_not_reached ();
    }
}

/**
 * btk_drag_dest_find_target:
 * @widget: drag destination widget
 * @context: drag context
 * @target_list: (allow-none): list of droppable targets, or %NULL to use
 *    btk_drag_dest_get_target_list (@widget).
 * Looks for a match between the supported targets of @context and the
 * @dest_target_list, returning the first matching target, otherwise
 * returning %BDK_NONE. @dest_target_list should usually be the return
 * value from btk_drag_dest_get_target_list(), but some widgets may
 * have different valid targets for different parts of the widget; in
 * that case, they will have to implement a drag_motion handler that
 * passes the correct target list to this function.
 *
 * Return value: (transfer none): first target that the source offers
 *     and the dest can accept, or %BDK_NONE
 **/
BdkAtom
btk_drag_dest_find_target (BtkWidget      *widget,
                           BdkDragContext *context,
                           BtkTargetList  *target_list)
{
  GList *tmp_target;
  GList *tmp_source = NULL;
  BtkWidget *source_widget;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), BDK_NONE);
  g_return_val_if_fail (BDK_IS_DRAG_CONTEXT (context), BDK_NONE);


  source_widget = btk_drag_get_source_widget (context);

  if (target_list == NULL)
    target_list = btk_drag_dest_get_target_list (widget);
  
  if (target_list == NULL)
    return BDK_NONE;
  
  tmp_target = target_list->list;
  while (tmp_target)
    {
      BtkTargetPair *pair = tmp_target->data;
      tmp_source = bdk_drag_context_list_targets (context);
      while (tmp_source)
	{
	  if (tmp_source->data == GUINT_TO_POINTER (pair->target))
	    {
	      if ((!(pair->flags & BTK_TARGET_SAME_APP) || source_widget) &&
		  (!(pair->flags & BTK_TARGET_SAME_WIDGET) || (source_widget == widget)) &&
                  (!(pair->flags & BTK_TARGET_OTHER_APP) || !source_widget) &&
                  (!(pair->flags & BTK_TARGET_OTHER_WIDGET) || (source_widget != widget)))
		return pair->target;
	      else
		break;
	    }
	  tmp_source = tmp_source->next;
	}
      tmp_target = tmp_target->next;
    }

  return BDK_NONE;
}

static void
btk_drag_selection_received (BtkWidget        *widget,
			     BtkSelectionData *selection_data,
			     guint             time,
			     gpointer          data)
{
  BdkDragContext *context;
  BtkDragDestInfo *info;
  BtkWidget *drop_widget;

  drop_widget = data;

  context = g_object_get_data (G_OBJECT (widget), "drag-context");
  info = btk_drag_get_dest_info (context, FALSE);

  if (info->proxy_data && 
      info->proxy_data->target == selection_data->target)
    {
      btk_selection_data_set (info->proxy_data,
			      selection_data->type,
			      selection_data->format,
			      selection_data->data,
			      selection_data->length);
      btk_main_quit ();
      return;
    }

  if (selection_data->target == bdk_atom_intern_static_string ("DELETE"))
    {
      btk_drag_finish (context, TRUE, FALSE, time);
    }
  else if ((selection_data->target == bdk_atom_intern_static_string ("XmTRANSFER_SUCCESS")) ||
	   (selection_data->target == bdk_atom_intern_static_string ("XmTRANSFER_FAILURE")))
    {
      /* Do nothing */
    }
  else
    {
      BtkDragDestSite *site;

      site = g_object_get_data (G_OBJECT (drop_widget), "btk-drag-dest");

      if (site && site->target_list)
	{
	  guint target_info;

	  if (btk_target_list_find (site->target_list, 
				    selection_data->target,
				    &target_info))
	    {
	      if (!(site->flags & BTK_DEST_DEFAULT_DROP) ||
		  selection_data->length >= 0)
		g_signal_emit_by_name (drop_widget,
				       "drag-data-received",
				       context, info->drop_x, info->drop_y,
				       selection_data,
				       target_info, time);
	    }
	}
      else
	{
	  g_signal_emit_by_name (drop_widget,
				 "drag-data-received",
				 context, info->drop_x, info->drop_y,
				 selection_data,
				 0, time);
	}
      
      if (site && site->flags & BTK_DEST_DEFAULT_DROP)
	{

	  btk_drag_finish (context, 
			   (selection_data->length >= 0),
			   (bdk_drag_context_get_selected_action (context) == BDK_ACTION_MOVE),
			   time);
	}
      
      g_object_unref (drop_widget);
    }

  g_signal_handlers_disconnect_by_func (widget,
					btk_drag_selection_received,
					data);
  
  g_object_set_data (G_OBJECT (widget), I_("drag-context"), NULL);
  g_object_unref (context);

  btk_drag_release_ipc_widget (widget);
}

/*************************************************************
 * btk_drag_find_widget:
 *     Function used to locate widgets for
 *     DRAG_MOTION and DROP_START events.
 *************************************************************/

static gboolean
btk_drag_find_widget (BtkWidget           *widget,
                      BdkDragContext      *context,
                      BtkDragDestInfo     *info,
                      gint                 x,
                      gint                 y,
                      guint32              time,
                      BtkDragDestCallback  callback)
{
  if (!btk_widget_get_mapped (widget) ||
      !btk_widget_get_sensitive (widget))
    return FALSE;

  /* Get the widget at the pointer coordinates and travel up
   * the widget hierarchy from there.
   */
  widget = _btk_widget_find_at_coords (btk_widget_get_window (widget),
                                       x, y, &x, &y);
  if (!widget)
    return FALSE;

  while (widget)
    {
      BtkWidget *parent;
      GList *hierarchy = NULL;
      gboolean found = FALSE;

      if (!btk_widget_get_mapped (widget) ||
          !btk_widget_get_sensitive (widget))
        return FALSE;

      /* need to reference the entire hierarchy temporarily in case the
       * ::drag-motion/::drag-drop callbacks change the widget hierarchy.
       */
      for (parent = widget;
           parent;
           parent = btk_widget_get_parent (parent))
        {
          hierarchy = g_list_prepend (hierarchy, g_object_ref (parent));
        }

      /* If the current widget is registered as a drop site, check to
       * emit "drag-motion" to check if we are actually in a drop
       * site.
       */
      if (g_object_get_data (G_OBJECT (widget), "btk-drag-dest"))
	{
	  found = callback (widget, context, x, y, time);

	  /* If so, send a "drag-leave" to the last widget */
	  if (found)
	    {
	      if (info->widget && info->widget != widget)
		{
		  btk_drag_dest_leave (info->widget, context, time);
		}

	      info->widget = widget;
	    }
	}

      if (!found)
        {
          /* Get the parent before unreffing the hierarchy because
           * invoking the callback might have destroyed the widget
           */
          parent = btk_widget_get_parent (widget);

          /* The parent might be going away when unreffing the
           * hierarchy, so also protect againt that
           */
          if (parent)
            g_object_add_weak_pointer (G_OBJECT (parent), (gpointer *) &parent);
        }

      g_list_foreach (hierarchy, (GFunc) g_object_unref, NULL);
      g_list_free (hierarchy);

      if (found)
        return TRUE;

      if (parent)
        g_object_remove_weak_pointer (G_OBJECT (parent), (gpointer *) &parent);
      else
        return FALSE;

      if (!btk_widget_translate_coordinates (widget, parent, x, y, &x, &y))
        return FALSE;

      widget = parent;
    }

  return FALSE;
}

static void
btk_drag_proxy_begin (BtkWidget       *widget,
		      BtkDragDestInfo *dest_info,
		      guint32          time)
{
  BtkDragSourceInfo *source_info;
  GList *tmp_list;
  BdkDragContext *context;
  BtkWidget *ipc_widget;

  if (dest_info->proxy_source)
    {
      bdk_drag_abort (dest_info->proxy_source->context, time);
      btk_drag_source_info_destroy (dest_info->proxy_source);
      dest_info->proxy_source = NULL;
    }
  
  ipc_widget = btk_drag_get_ipc_widget (widget);
  context = bdk_drag_begin (btk_widget_get_window (ipc_widget),
			    bdk_drag_context_list_targets (dest_info->context));

  source_info = btk_drag_get_source_info (context, TRUE);

  source_info->ipc_widget = ipc_widget;
  source_info->widget = g_object_ref (widget);

  source_info->target_list = btk_target_list_new (NULL, 0);
  tmp_list = bdk_drag_context_list_targets (dest_info->context);
  while (tmp_list)
    {
      btk_target_list_add (source_info->target_list,
			   BDK_POINTER_TO_ATOM (tmp_list->data), 0, 0);
      tmp_list = tmp_list->next;
    }

  source_info->proxy_dest = dest_info;
  
  g_signal_connect (ipc_widget,
		    "selection-get",
		    G_CALLBACK (btk_drag_selection_get),
		    source_info);
  
  dest_info->proxy_source = source_info;
}

static void
btk_drag_dest_info_destroy (gpointer data)
{
  BtkDragDestInfo *info = data;

  g_free (info);
}

static BtkDragDestInfo *
btk_drag_get_dest_info (BdkDragContext *context,
			gboolean        create)
{
  BtkDragDestInfo *info;
  static GQuark info_quark = 0;
  if (!info_quark)
    info_quark = g_quark_from_static_string ("btk-dest-info");
  
  info = g_object_get_qdata (G_OBJECT (context), info_quark);
  if (!info && create)
    {
      info = g_new (BtkDragDestInfo, 1);
      info->widget = NULL;
      info->context = context;
      info->proxy_source = NULL;
      info->proxy_data = NULL;
      info->dropped = FALSE;
      info->proxy_drop_wait = FALSE;
      g_object_set_qdata_full (G_OBJECT (context), info_quark,
			       info, btk_drag_dest_info_destroy);
    }

  return info;
}

static GQuark dest_info_quark = 0;

static BtkDragSourceInfo *
btk_drag_get_source_info (BdkDragContext *context,
			  gboolean        create)
{
  BtkDragSourceInfo *info;
  if (!dest_info_quark)
    dest_info_quark = g_quark_from_static_string ("btk-source-info");
  
  info = g_object_get_qdata (G_OBJECT (context), dest_info_quark);
  if (!info && create)
    {
      info = g_new0 (BtkDragSourceInfo, 1);
      info->context = context;
      g_object_set_qdata (G_OBJECT (context), dest_info_quark, info);
    }

  return info;
}

static void
btk_drag_clear_source_info (BdkDragContext *context)
{
  g_object_set_qdata (G_OBJECT (context), dest_info_quark, NULL);
}

static void
btk_drag_dest_realized (BtkWidget *widget)
{
  BtkWidget *toplevel = btk_widget_get_toplevel (widget);

  if (btk_widget_is_toplevel (toplevel))
    bdk_window_register_dnd (toplevel->window);
}

static void
btk_drag_dest_hierarchy_changed (BtkWidget *widget,
				 BtkWidget *previous_toplevel)
{
  BtkWidget *toplevel = btk_widget_get_toplevel (widget);

  if (btk_widget_is_toplevel (toplevel) && btk_widget_get_realized (toplevel))
    bdk_window_register_dnd (toplevel->window);
}

static void
btk_drag_dest_site_destroy (gpointer data)
{
  BtkDragDestSite *site = data;

  if (site->proxy_window)
    g_object_unref (site->proxy_window);
    
  if (site->target_list)
    btk_target_list_unref (site->target_list);

  g_free (site);
}

/*
 * Default drag handlers
 */
static void  
btk_drag_dest_leave (BtkWidget      *widget,
		     BdkDragContext *context,
		     guint           time)
{
  BtkDragDestSite *site;

  site = g_object_get_data (G_OBJECT (widget), "btk-drag-dest");
  g_return_if_fail (site != NULL);

  if (site->do_proxy)
    {
      BtkDragDestInfo *info = btk_drag_get_dest_info (context, FALSE);

      if (info->proxy_source && info->proxy_source->widget == widget && !info->dropped)
	{
	  bdk_drag_abort (info->proxy_source->context, time);
	  btk_drag_source_info_destroy (info->proxy_source);
	  info->proxy_source = NULL;
	}
      
      return;
    }
  else
    {
      if ((site->flags & BTK_DEST_DEFAULT_HIGHLIGHT) && site->have_drag)
	btk_drag_unhighlight (widget);

      if (!(site->flags & BTK_DEST_DEFAULT_MOTION) || site->have_drag ||
	  site->track_motion)
	g_signal_emit_by_name (widget, "drag-leave", context, time);
      
      site->have_drag = FALSE;
    }
}

static gboolean
btk_drag_dest_motion (BtkWidget	     *widget,
		      BdkDragContext *context,
		      gint            x,
		      gint            y,
		      guint           time)
{
  BtkDragDestSite *site;
  BdkDragAction action = 0;
  gboolean retval;

  site = g_object_get_data (G_OBJECT (widget), "btk-drag-dest");
  g_return_val_if_fail (site != NULL, FALSE);

  if (site->do_proxy)
    {
      BdkAtom selection;
      BdkEvent *current_event;
      BdkWindow *dest_window;
      BdkDragProtocol proto;
	
      BtkDragDestInfo *info = btk_drag_get_dest_info (context, FALSE);

      if (!info->proxy_source || info->proxy_source->widget != widget)
	btk_drag_proxy_begin (widget, info, time);

      current_event = btk_get_current_event ();

      if (site->proxy_window)
	{
	  dest_window = site->proxy_window;
	  proto = site->proxy_protocol;
	}
      else
	{
	  bdk_drag_find_window_for_screen (info->proxy_source->context,
					   NULL,
					   bdk_window_get_screen (current_event->dnd.window),
					   current_event->dnd.x_root, 
					   current_event->dnd.y_root,
					   &dest_window, &proto);
	}
      
      bdk_drag_motion (info->proxy_source->context, 
		       dest_window, proto,
		       current_event->dnd.x_root, 
		       current_event->dnd.y_root, 
                       bdk_drag_context_get_suggested_action (context),
                       bdk_drag_context_get_actions (context),
                       time);

      if (!site->proxy_window && dest_window)
	g_object_unref (dest_window);

      selection = bdk_drag_get_selection (info->proxy_source->context);
      if (selection && 
	  selection != bdk_drag_get_selection (info->context))
	btk_drag_source_check_selection (info->proxy_source, selection, time);

      bdk_event_free (current_event);
      
      return TRUE;
    }

  if (site->track_motion || site->flags & BTK_DEST_DEFAULT_MOTION)
    {
      if (bdk_drag_context_get_suggested_action (context) & site->actions)
	action = bdk_drag_context_get_suggested_action (context);
      else
	{
	  gint i;
	  
	  for (i = 0; i < 8; i++)
	    {
	      if ((site->actions & (1 << i)) &&
		  (bdk_drag_context_get_actions (context) & (1 << i)))
		{
		  action = (1 << i);
		  break;
		}
	    }
	}

      if (action && btk_drag_dest_find_target (widget, context, NULL))
	{
	  if (!site->have_drag)
	    {
	      site->have_drag = TRUE;
	      if (site->flags & BTK_DEST_DEFAULT_HIGHLIGHT)
		btk_drag_highlight (widget);
	    }

	  bdk_drag_status (context, action, time);
	}
      else
	{
	  bdk_drag_status (context, 0, time);
	  if (!site->track_motion)
	    return TRUE;
	}
    }

  g_signal_emit_by_name (widget, "drag-motion",
			 context, x, y, time, &retval);

  return (site->flags & BTK_DEST_DEFAULT_MOTION) ? TRUE : retval;
}

static gboolean
btk_drag_dest_drop (BtkWidget	     *widget,
		    BdkDragContext   *context,
		    gint              x,
		    gint              y,
		    guint             time)
{
  BtkDragDestSite *site;
  BtkDragDestInfo *info;

  site = g_object_get_data (G_OBJECT (widget), "btk-drag-dest");
  g_return_val_if_fail (site != NULL, FALSE);

  info = btk_drag_get_dest_info (context, FALSE);
  g_return_val_if_fail (info != NULL, FALSE);

  info->drop_x = x;
  info->drop_y = y;

  if (site->do_proxy)
    {
      if (info->proxy_source || 
	  (bdk_drag_context_get_protocol (info->context) == BDK_DRAG_PROTO_ROOTWIN))
	{
	  btk_drag_drop (info->proxy_source, time);
	}
      else
	{
	  /* We need to synthesize a motion event, wait for a status,
	   * and, if we get a good one, do a drop.
	   */
	  
	  BdkEvent *current_event;
	  BdkAtom selection;
	  BdkWindow *dest_window;
	  BdkDragProtocol proto;
	  
	  btk_drag_proxy_begin (widget, info, time);
	  info->proxy_drop_wait = TRUE;
	  info->proxy_drop_time = time;
	  
	  current_event = btk_get_current_event ();

	  if (site->proxy_window)
	    {
	      dest_window = site->proxy_window;
	      proto = site->proxy_protocol;
	    }
	  else
	    {
	      bdk_drag_find_window_for_screen (info->proxy_source->context,
					       NULL,
					       bdk_window_get_screen (current_event->dnd.window),
					       current_event->dnd.x_root, 
					       current_event->dnd.y_root,
					       &dest_window, &proto);
	    }

	  bdk_drag_motion (info->proxy_source->context, 
			   dest_window, proto,
			   current_event->dnd.x_root, 
			   current_event->dnd.y_root, 
                           bdk_drag_context_get_suggested_action (context),
                           bdk_drag_context_get_actions (context),
                           time);

	  if (!site->proxy_window && dest_window)
	    g_object_unref (dest_window);

	  selection = bdk_drag_get_selection (info->proxy_source->context);
	  if (selection && 
	      selection != bdk_drag_get_selection (info->context))
	    btk_drag_source_check_selection (info->proxy_source, selection, time);

	  bdk_event_free (current_event);
	}

      return TRUE;
    }
  else
    {
      gboolean retval;

      if (site->flags & BTK_DEST_DEFAULT_DROP)
	{
	  BdkAtom target = btk_drag_dest_find_target (widget, context, NULL);

	  if (target == BDK_NONE)
	    {
	      btk_drag_finish (context, FALSE, FALSE, time);
	      return TRUE;
	    }
	  else 
	    btk_drag_get_data (widget, context, target, time);
	}

      g_signal_emit_by_name (widget, "drag-drop",
			     context, x, y, time, &retval);

      return (site->flags & BTK_DEST_DEFAULT_DROP) ? TRUE : retval;
    }
}

/***************
 * Source side *
 ***************/

/* Like BtkDragBegin, but also takes a BtkDragSourceSite,
 * so that we can set the icon from the source site information
 */
static BdkDragContext *
btk_drag_begin_internal (BtkWidget         *widget,
			 BtkDragSourceSite *site,
			 BtkTargetList     *target_list,
			 BdkDragAction      actions,
			 gint               button,
			 BdkEvent          *event)
{
  BtkDragSourceInfo *info;
  GList *targets = NULL;
  GList *tmp_list;
  guint32 time = BDK_CURRENT_TIME;
  BdkDragAction possible_actions, suggested_action;
  BdkDragContext *context;
  BtkWidget *ipc_widget;
  BdkCursor *cursor;
 
  ipc_widget = btk_drag_get_ipc_widget (widget);
  
  btk_drag_get_event_actions (event, button, actions,
			      &suggested_action, &possible_actions);
  
  cursor = btk_drag_get_cursor (btk_widget_get_display (widget), 
			        suggested_action,
				NULL);
  
  if (event)
    {
      time = bdk_event_get_time (event);
      if (time == BDK_CURRENT_TIME)
        time = btk_get_current_event_time ();
    }

  if (bdk_pointer_grab (ipc_widget->window, FALSE,
			BDK_POINTER_MOTION_MASK |
			BDK_BUTTON_RELEASE_MASK, NULL,
			cursor, time) != BDK_GRAB_SUCCESS)
    {
      btk_drag_release_ipc_widget (ipc_widget);
      return NULL;
    }

  grab_dnd_keys (ipc_widget, time);

  /* We use a BTK grab here to override any grabs that the widget
   * we are dragging from might have held
   */
  btk_grab_add (ipc_widget);
  
  tmp_list = g_list_last (target_list->list);
  while (tmp_list)
    {
      BtkTargetPair *pair = tmp_list->data;
      targets = g_list_prepend (targets, 
				GINT_TO_POINTER (pair->target));
      tmp_list = tmp_list->prev;
    }

  source_widgets = g_slist_prepend (source_widgets, ipc_widget);

  context = bdk_drag_begin (ipc_widget->window, targets);
  g_list_free (targets);
  
  info = btk_drag_get_source_info (context, TRUE);
  
  info->ipc_widget = ipc_widget;
  g_object_set_data (G_OBJECT (info->ipc_widget), I_("btk-info"), info);

  info->widget = g_object_ref (widget);
  
  info->button = button;
  info->cursor = cursor;
  info->target_list = target_list;
  btk_target_list_ref (target_list);

  info->possible_actions = actions;

  info->status = BTK_DRAG_STATUS_DRAG;
  info->last_event = NULL;
  info->selections = NULL;
  info->icon_window = NULL;
  info->destroy_icon = FALSE;

  /* Set cur_x, cur_y here so if the "drag-begin" signal shows
   * the drag icon, it will be in the right place
   */
  if (event && event->type == BDK_MOTION_NOTIFY)
    {
      info->cur_screen = btk_widget_get_screen (widget);
      info->cur_x = event->motion.x_root;
      info->cur_y = event->motion.y_root;
    }
  else 
    {
      bdk_display_get_pointer (btk_widget_get_display (widget),
			       &info->cur_screen, &info->cur_x, &info->cur_y, NULL);
    }

  g_signal_emit_by_name (widget, "drag-begin", info->context);

  /* Ensure that we have an icon before we start the drag; the
   * application may have set one in ::drag_begin, or it may
   * not have set one.
   */
  if (!info->icon_window && !info->icon_pixbuf)
    {
      if (!site || site->icon_type == BTK_IMAGE_EMPTY)
	btk_drag_set_icon_default (context);
      else
	switch (site->icon_type)
	  {
	  case BTK_IMAGE_PIXMAP:
	    btk_drag_set_icon_pixmap (context,
				      site->colormap,
				      site->icon_data.pixmap.pixmap,
				      site->icon_mask,
				      -2, -2);
	    break;
	  case BTK_IMAGE_PIXBUF:
	    btk_drag_set_icon_pixbuf (context,
				      site->icon_data.pixbuf.pixbuf,
				      -2, -2);
	    break;
	  case BTK_IMAGE_STOCK:
	    btk_drag_set_icon_stock (context,
				     site->icon_data.stock.stock_id,
				     -2, -2);
	    break;
	  case BTK_IMAGE_ICON_NAME:
	    btk_drag_set_icon_name (context,
			    	    site->icon_data.name.icon_name,
				    -2, -2);
	    break;
	  case BTK_IMAGE_EMPTY:
	  default:
	    g_assert_not_reached();
	    break;
	  }
    }

  /* We need to composite the icon into the cursor, if we are
   * not using an icon window.
   */
  if (info->icon_pixbuf)  
    {
      cursor = btk_drag_get_cursor (btk_widget_get_display (widget), 
  			            suggested_action,
			  	    info);
  
      if (cursor != info->cursor)
        {
	  bdk_pointer_grab (widget->window, FALSE,
	 	            BDK_POINTER_MOTION_MASK |
		  	    BDK_BUTTON_RELEASE_MASK,
			    NULL,
			    cursor, time);
          info->cursor = cursor;
        }
    }
    
  if (event && event->type == BDK_MOTION_NOTIFY)
    btk_drag_motion_cb (info->ipc_widget, (BdkEventMotion *)event, info);
  else
    btk_drag_update (info, info->cur_screen, info->cur_x, info->cur_y, event);

  info->start_x = info->cur_x;
  info->start_y = info->cur_y;

  g_signal_connect (info->ipc_widget, "grab-broken-event",
		    G_CALLBACK (btk_drag_grab_broken_event_cb), info);
  g_signal_connect (info->ipc_widget, "grab-notify",
		    G_CALLBACK (btk_drag_grab_notify_cb), info);
  g_signal_connect (info->ipc_widget, "button-release-event",
		    G_CALLBACK (btk_drag_button_release_cb), info);
  g_signal_connect (info->ipc_widget, "motion-notify-event",
		    G_CALLBACK (btk_drag_motion_cb), info);
  g_signal_connect (info->ipc_widget, "key-press-event",
		    G_CALLBACK (btk_drag_key_cb), info);
  g_signal_connect (info->ipc_widget, "key-release-event",
		    G_CALLBACK (btk_drag_key_cb), info);
  g_signal_connect (info->ipc_widget, "selection-get",
		    G_CALLBACK (btk_drag_selection_get), info);

  info->have_grab = TRUE;
  info->grab_time = time;

  return info->context;
}

/**
 * btk_drag_begin:
 * @widget: the source widget.
 * @targets: The targets (data formats) in which the
 *    source can provide the data.
 * @actions: A bitmask of the allowed drag actions for this drag.
 * @button: The button the user clicked to start the drag.
 * @event: The event that triggered the start of the drag.
 * 
 * Initiates a drag on the source side. The function
 * only needs to be used when the application is
 * starting drags itself, and is not needed when
 * btk_drag_source_set() is used.
 *
 * The @event is used to retrieve the timestamp that will be used internally to
 * grab the pointer.  If @event is #NULL, then BDK_CURRENT_TIME will be used.
 * However, you should try to pass a real event in all cases, since that can be
 * used by BTK+ to get information about the start position of the drag, for
 * example if the @event is a BDK_MOTION_NOTIFY.
 *
 * Generally there are three cases when you want to start a drag by hand by calling
 * this function:
 *
 * 1. During a button-press-event handler, if you want to start a drag immediately
 * when the user presses the mouse button.  Pass the @event that you have in your
 * button-press-event handler.
 *
 * 2. During a motion-notify-event handler, if you want to start a drag when the mouse
 * moves past a certain threshold distance after a button-press.  Pass the @event that you
 * have in your motion-notify-event handler.
 *
 * 3. During a timeout handler, if you want to start a drag after the mouse
 * button is held down for some time.  Try to save the last event that you got
 * from the mouse, using bdk_event_copy(), and pass it to this function
 * (remember to free the event with bdk_event_free() when you are done).  If you
 * can really not pass a real event, pass #NULL instead.
 * 
 * Return value: (transfer none): the context for this drag.
 **/
BdkDragContext *
btk_drag_begin (BtkWidget         *widget,
		BtkTargetList     *targets,
		BdkDragAction      actions,
		gint               button,
		BdkEvent          *event)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (btk_widget_get_realized (widget), NULL);
  g_return_val_if_fail (targets != NULL, NULL);

  return btk_drag_begin_internal (widget, NULL, targets,
				  actions, button, event);
}

/**
 * btk_drag_source_set:
 * @widget: a #BtkWidget
 * @start_button_mask: the bitmask of buttons that can start the drag
 * @targets: (allow-none) (array length=n_targets): the table of targets that the drag will support,
 *     may be %NULL
 * @n_targets: the number of items in @targets
 * @actions: the bitmask of possible actions for a drag from this widget
 *
 * Sets up a widget so that BTK+ will start a drag operation when the user
 * clicks and drags on the widget. The widget must have a window.
 */
void
btk_drag_source_set (BtkWidget            *widget,
		     BdkModifierType       start_button_mask,
		     const BtkTargetEntry *targets,
		     gint                  n_targets,
		     BdkDragAction         actions)
{
  BtkDragSourceSite *site;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  site = g_object_get_data (G_OBJECT (widget), "btk-site-data");

  btk_widget_add_events (widget,
			 btk_widget_get_events (widget) |
			 BDK_BUTTON_PRESS_MASK | BDK_BUTTON_RELEASE_MASK |
			 BDK_BUTTON_MOTION_MASK);

  if (site)
    {
      if (site->target_list)
	btk_target_list_unref (site->target_list);
    }
  else
    {
      site = g_new0 (BtkDragSourceSite, 1);

      site->icon_type = BTK_IMAGE_EMPTY;
      
      g_signal_connect (widget, "button-press-event",
			G_CALLBACK (btk_drag_source_event_cb),
			site);
      g_signal_connect (widget, "button-release-event",
			G_CALLBACK (btk_drag_source_event_cb),
			site);
      g_signal_connect (widget, "motion-notify-event",
			G_CALLBACK (btk_drag_source_event_cb),
			site);
      
      g_object_set_data_full (G_OBJECT (widget),
			      I_("btk-site-data"), 
			      site, btk_drag_source_site_destroy);
    }

  site->start_button_mask = start_button_mask;

  site->target_list = btk_target_list_new (targets, n_targets);

  site->actions = actions;
}

/*************************************************************
 * btk_drag_source_unset
 *     Unregister this widget as a drag source.
 *   arguments:
 *     widget:
 *   results:
 *************************************************************/

void 
btk_drag_source_unset (BtkWidget        *widget)
{
  BtkDragSourceSite *site;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  site = g_object_get_data (G_OBJECT (widget), "btk-site-data");

  if (site)
    {
      g_signal_handlers_disconnect_by_func (widget,
					    btk_drag_source_event_cb,
					    site);
      g_object_set_data (G_OBJECT (widget), I_("btk-site-data"), NULL);
    }
}

/**
 * btk_drag_source_get_target_list:
 * @widget: a #BtkWidget
 *
 * Gets the list of targets this widget can provide for
 * drag-and-drop.
 *
 * Return value: (transfer none): the #BtkTargetList, or %NULL if none
 *
 * Since: 2.4
 **/
BtkTargetList *
btk_drag_source_get_target_list (BtkWidget *widget)
{
  BtkDragSourceSite *site;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

  site = g_object_get_data (G_OBJECT (widget), "btk-site-data");

  return site ? site->target_list : NULL;
}

/**
 * btk_drag_source_set_target_list:
 * @widget: a #BtkWidget that's a drag source
 * @target_list: (allow-none): list of draggable targets, or %NULL for none
 *
 * Changes the target types that this widget offers for drag-and-drop.
 * The widget must first be made into a drag source with
 * btk_drag_source_set().
 *
 * Since: 2.4
 **/
void
btk_drag_source_set_target_list (BtkWidget     *widget,
                                 BtkTargetList *target_list)
{
  BtkDragSourceSite *site;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  site = g_object_get_data (G_OBJECT (widget), "btk-site-data");
  if (site == NULL)
    {
      g_warning ("btk_drag_source_set_target_list() requires the widget "
		 "to already be a drag source.");
      return;
    }

  if (target_list)
    btk_target_list_ref (target_list);

  if (site->target_list)
    btk_target_list_unref (site->target_list);

  site->target_list = target_list;
}

/**
 * btk_drag_source_add_text_targets:
 * @widget: a #BtkWidget that's is a drag source
 *
 * Add the text targets supported by #BtkSelection to
 * the target list of the drag source.  The targets
 * are added with @info = 0. If you need another value, 
 * use btk_target_list_add_text_targets() and
 * btk_drag_source_set_target_list().
 * 
 * Since: 2.6
 **/
void
btk_drag_source_add_text_targets (BtkWidget *widget)
{
  BtkTargetList *target_list;

  target_list = btk_drag_source_get_target_list (widget);
  if (target_list)
    btk_target_list_ref (target_list);
  else
    target_list = btk_target_list_new (NULL, 0);
  btk_target_list_add_text_targets (target_list, 0);
  btk_drag_source_set_target_list (widget, target_list);
  btk_target_list_unref (target_list);
}

/**
 * btk_drag_source_add_image_targets:
 * @widget: a #BtkWidget that's is a drag source
 *
 * Add the writable image targets supported by #BtkSelection to
 * the target list of the drag source. The targets
 * are added with @info = 0. If you need another value, 
 * use btk_target_list_add_image_targets() and
 * btk_drag_source_set_target_list().
 * 
 * Since: 2.6
 **/
void
btk_drag_source_add_image_targets (BtkWidget *widget)
{
  BtkTargetList *target_list;

  target_list = btk_drag_source_get_target_list (widget);
  if (target_list)
    btk_target_list_ref (target_list);
  else
    target_list = btk_target_list_new (NULL, 0);
  btk_target_list_add_image_targets (target_list, 0, TRUE);
  btk_drag_source_set_target_list (widget, target_list);
  btk_target_list_unref (target_list);
}

/**
 * btk_drag_source_add_uri_targets:
 * @widget: a #BtkWidget that's is a drag source
 *
 * Add the URI targets supported by #BtkSelection to
 * the target list of the drag source.  The targets
 * are added with @info = 0. If you need another value, 
 * use btk_target_list_add_uri_targets() and
 * btk_drag_source_set_target_list().
 * 
 * Since: 2.6
 **/
void
btk_drag_source_add_uri_targets (BtkWidget *widget)
{
  BtkTargetList *target_list;

  target_list = btk_drag_source_get_target_list (widget);
  if (target_list)
    btk_target_list_ref (target_list);
  else
    target_list = btk_target_list_new (NULL, 0);
  btk_target_list_add_uri_targets (target_list, 0);
  btk_drag_source_set_target_list (widget, target_list);
  btk_target_list_unref (target_list);
}

static void
btk_drag_source_unset_icon (BtkDragSourceSite *site)
{
  switch (site->icon_type)
    {
    case BTK_IMAGE_EMPTY:
      break;
    case BTK_IMAGE_PIXMAP:
      if (site->icon_data.pixmap.pixmap)
	g_object_unref (site->icon_data.pixmap.pixmap);
      if (site->icon_mask)
	g_object_unref (site->icon_mask);
      break;
    case BTK_IMAGE_PIXBUF:
      g_object_unref (site->icon_data.pixbuf.pixbuf);
      break;
    case BTK_IMAGE_STOCK:
      g_free (site->icon_data.stock.stock_id);
      break;
    case BTK_IMAGE_ICON_NAME:
      g_free (site->icon_data.name.icon_name);
      break;
    default:
      g_assert_not_reached();
      break;
    }
  site->icon_type = BTK_IMAGE_EMPTY;
  
  if (site->colormap)
    g_object_unref (site->colormap);
  site->colormap = NULL;
}

/**
 * btk_drag_source_set_icon:
 * @widget: a #BtkWidget
 * @colormap: the colormap of the icon
 * @pixmap: the image data for the icon
 * @mask: (allow-none): the transparency mask for an image.
 *
 * Sets the icon that will be used for drags from a particular widget
 * from a pixmap/mask. BTK+ retains references for the arguments, and
 * will release them when they are no longer needed.
 * Use btk_drag_source_set_icon_pixbuf() instead.
 **/
void 
btk_drag_source_set_icon (BtkWidget     *widget,
			  BdkColormap   *colormap,
			  BdkPixmap     *pixmap,
			  BdkBitmap     *mask)
{
  BtkDragSourceSite *site;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (BDK_IS_COLORMAP (colormap));
  g_return_if_fail (BDK_IS_PIXMAP (pixmap));
  g_return_if_fail (!mask || BDK_IS_PIXMAP (mask));

  site = g_object_get_data (G_OBJECT (widget), "btk-site-data");
  g_return_if_fail (site != NULL);
  
  g_object_ref (colormap);
  g_object_ref (pixmap);
  if (mask)
    g_object_ref (mask);

  btk_drag_source_unset_icon (site);

  site->icon_type = BTK_IMAGE_PIXMAP;
  
  site->icon_data.pixmap.pixmap = pixmap;
  site->icon_mask = mask;
  site->colormap = colormap;
}

/**
 * btk_drag_source_set_icon_pixbuf:
 * @widget: a #BtkWidget
 * @pixbuf: the #BdkPixbuf for the drag icon
 * 
 * Sets the icon that will be used for drags from a particular widget
 * from a #BdkPixbuf. BTK+ retains a reference for @pixbuf and will 
 * release it when it is no longer needed.
 **/
void 
btk_drag_source_set_icon_pixbuf (BtkWidget   *widget,
				 BdkPixbuf   *pixbuf)
{
  BtkDragSourceSite *site;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (BDK_IS_PIXBUF (pixbuf));

  site = g_object_get_data (G_OBJECT (widget), "btk-site-data");
  g_return_if_fail (site != NULL); 
  g_object_ref (pixbuf);

  btk_drag_source_unset_icon (site);

  site->icon_type = BTK_IMAGE_PIXBUF;
  site->icon_data.pixbuf.pixbuf = pixbuf;
}

/**
 * btk_drag_source_set_icon_stock:
 * @widget: a #BtkWidget
 * @stock_id: the ID of the stock icon to use
 *
 * Sets the icon that will be used for drags from a particular source
 * to a stock icon. 
 **/
void 
btk_drag_source_set_icon_stock (BtkWidget   *widget,
				const gchar *stock_id)
{
  BtkDragSourceSite *site;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (stock_id != NULL);

  site = g_object_get_data (G_OBJECT (widget), "btk-site-data");
  g_return_if_fail (site != NULL);
  
  btk_drag_source_unset_icon (site);

  site->icon_type = BTK_IMAGE_STOCK;
  site->icon_data.stock.stock_id = g_strdup (stock_id);
}

/**
 * btk_drag_source_set_icon_name:
 * @widget: a #BtkWidget
 * @icon_name: name of icon to use
 * 
 * Sets the icon that will be used for drags from a particular source
 * to a themed icon. See the docs for #BtkIconTheme for more details.
 *
 * Since: 2.8
 **/
void 
btk_drag_source_set_icon_name (BtkWidget   *widget,
			       const gchar *icon_name)
{
  BtkDragSourceSite *site;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (icon_name != NULL);

  site = g_object_get_data (G_OBJECT (widget), "btk-site-data");
  g_return_if_fail (site != NULL);

  btk_drag_source_unset_icon (site);

  site->icon_type = BTK_IMAGE_ICON_NAME;
  site->icon_data.name.icon_name = g_strdup (icon_name);
}

static void
btk_drag_get_icon (BtkDragSourceInfo *info,
		   BtkWidget        **icon_window,
		   gint              *hot_x,
		   gint              *hot_y)
{
  if (get_can_change_screen (info->icon_window))
    btk_window_set_screen (BTK_WINDOW (info->icon_window),
			   info->cur_screen);
      
  if (btk_widget_get_screen (info->icon_window) != info->cur_screen)
    {
      if (!info->fallback_icon)
	{
	  gint save_hot_x, save_hot_y;
	  gboolean save_destroy_icon;
	  BtkWidget *save_icon_window;
	  
	  /* HACK to get the appropriate icon
	   */
	  save_icon_window = info->icon_window;
	  save_hot_x = info->hot_x;
	  save_hot_y = info->hot_x;
	  save_destroy_icon = info->destroy_icon;

	  info->icon_window = NULL;
	  if (!default_icon_pixmap)
	    set_icon_stock_pixbuf (info->context, 
				   BTK_STOCK_DND, NULL, -2, -2, TRUE);
	  else
	    btk_drag_set_icon_pixmap (info->context, 
				      default_icon_colormap, 
				      default_icon_pixmap, 
				      default_icon_mask,
				      default_icon_hot_x,
				      default_icon_hot_y);
	  info->fallback_icon = info->icon_window;
	  
	  info->icon_window = save_icon_window;
	  info->hot_x = save_hot_x;
	  info->hot_y = save_hot_y;
	  info->destroy_icon = save_destroy_icon;
	}
      
      btk_widget_hide (info->icon_window);
      
      *icon_window = info->fallback_icon;
      btk_window_set_screen (BTK_WINDOW (*icon_window), info->cur_screen);
      
      if (!default_icon_pixmap)
	{
	  *hot_x = -2;
	  *hot_y = -2;
	}
      else
	{
	  *hot_x = default_icon_hot_x;
	  *hot_y = default_icon_hot_y;
	}
    }
  else
    {
      if (info->fallback_icon)
	btk_widget_hide (info->fallback_icon);
      
      *icon_window = info->icon_window;
      *hot_x = info->hot_x;
      *hot_y = info->hot_y;
    }
}

static void
btk_drag_update_icon (BtkDragSourceInfo *info)
{
  if (info->icon_window)
    {
      BtkWidget *icon_window;
      gint hot_x, hot_y;
  
      btk_drag_get_icon (info, &icon_window, &hot_x, &hot_y);
      
      btk_window_move (BTK_WINDOW (icon_window), 
		       info->cur_x - hot_x, 
		       info->cur_y - hot_y);

      if (btk_widget_get_visible (icon_window))
	bdk_window_raise (icon_window->window);
      else
	btk_widget_show (icon_window);
    }
}

static void 
btk_drag_set_icon_window (BdkDragContext *context,
			  BtkWidget      *widget,
			  gint            hot_x,
			  gint            hot_y,
			  gboolean        destroy_on_release)
{
  BtkDragSourceInfo *info;

  info = btk_drag_get_source_info (context, FALSE);
  if (info == NULL)
    {
      if (destroy_on_release)
	btk_widget_destroy (widget);
      return;
    }

  btk_drag_remove_icon (info);

  if (widget)
    g_object_ref (widget);  
  
  info->icon_window = widget;
  info->hot_x = hot_x;
  info->hot_y = hot_y;
  info->destroy_icon = destroy_on_release;

  if (widget && info->icon_pixbuf)
    {
      g_object_unref (info->icon_pixbuf);
      info->icon_pixbuf = NULL;
    }

  btk_drag_update_cursor (info);
  btk_drag_update_icon (info);
}

/**
 * btk_drag_set_icon_widget:
 * @context: the context for a drag. (This must be called 
          with a  context for the source side of a drag)
 * @widget: a toplevel window to use as an icon.
 * @hot_x: the X offset within @widget of the hotspot.
 * @hot_y: the Y offset within @widget of the hotspot.
 * 
 * Changes the icon for a widget to a given widget. BTK+
 * will not destroy the icon, so if you don't want
 * it to persist, you should connect to the "drag-end" 
 * signal and destroy it yourself.
 **/
void 
btk_drag_set_icon_widget (BdkDragContext    *context,
			  BtkWidget         *widget,
			  gint               hot_x,
			  gint               hot_y)
{
  g_return_if_fail (BDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (BTK_IS_WIDGET (widget));

  btk_drag_set_icon_window (context, widget, hot_x, hot_y, FALSE);
}

static void
icon_window_realize (BtkWidget *window,
		     BdkPixbuf *pixbuf)
{
  BdkPixmap *pixmap;
  BdkPixmap *mask;

  bdk_pixbuf_render_pixmap_and_mask_for_colormap (pixbuf,
						  btk_widget_get_colormap (window),
						  &pixmap, &mask, 128);
  
  bdk_window_set_back_pixmap (window->window, pixmap, FALSE);
  g_object_unref (pixmap);
  
  if (mask)
    {
      btk_widget_shape_combine_mask (window, mask, 0, 0);
      g_object_unref (mask);
    }
}

static void
set_icon_stock_pixbuf (BdkDragContext    *context,
		       const gchar       *stock_id,
		       BdkPixbuf         *pixbuf,
		       gint               hot_x,
		       gint               hot_y,
		       gboolean           force_window)
{
  BtkWidget *window;
  gint width, height;
  BdkScreen *screen;
  BdkDisplay *display;

  g_return_if_fail (context != NULL);
  g_return_if_fail (pixbuf != NULL || stock_id != NULL);
  g_return_if_fail (pixbuf == NULL || stock_id == NULL);

  screen = bdk_window_get_screen (bdk_drag_context_get_source_window (context));

  /* Push a NULL colormap to guard against btk_widget_push_colormap() */
  btk_widget_push_colormap (NULL);
  window = btk_window_new (BTK_WINDOW_POPUP);
  btk_window_set_type_hint (BTK_WINDOW (window), BDK_WINDOW_TYPE_HINT_DND);
  btk_window_set_screen (BTK_WINDOW (window), screen);
  set_can_change_screen (window, TRUE);
  btk_widget_pop_colormap ();

  btk_widget_set_events (window, BDK_BUTTON_PRESS_MASK | BDK_BUTTON_RELEASE_MASK);
  btk_widget_set_app_paintable (window, TRUE);

  if (stock_id)
    {
      pixbuf = btk_widget_render_icon (window, stock_id,
				       BTK_ICON_SIZE_DND, NULL);

      if (!pixbuf)
	{
	  g_warning ("Cannot load drag icon from stock_id %s", stock_id);
	  btk_widget_destroy (window);
	  return;
	}

    }
  else
    g_object_ref (pixbuf);

  display = bdk_window_get_display (bdk_drag_context_get_source_window (context));
  width = bdk_pixbuf_get_width (pixbuf);
  height = bdk_pixbuf_get_height (pixbuf);

  if (!force_window &&
      btk_drag_can_use_rgba_cursor (display, width + 2, height + 2))
    {
      BtkDragSourceInfo *info;

      btk_widget_destroy (window);

      info = btk_drag_get_source_info (context, FALSE);

      if (info->icon_pixbuf)
	g_object_unref (info->icon_pixbuf);
      info->icon_pixbuf = pixbuf;

      btk_drag_set_icon_window (context, NULL, hot_x, hot_y, TRUE);
    }
  else
    {
      btk_widget_set_size_request (window, width, height);

      g_signal_connect_closure (window, "realize",
  			        g_cclosure_new (G_CALLBACK (icon_window_realize),
					        pixbuf,
					        (GClosureNotify)g_object_unref),
			        FALSE);
		    
      btk_drag_set_icon_window (context, window, hot_x, hot_y, TRUE);
   }
}

/**
 * btk_drag_set_icon_pixbuf:
 * @context: the context for a drag. (This must be called 
 *            with a  context for the source side of a drag)
 * @pixbuf: the #BdkPixbuf to use as the drag icon.
 * @hot_x: the X offset within @widget of the hotspot.
 * @hot_y: the Y offset within @widget of the hotspot.
 * 
 * Sets @pixbuf as the icon for a given drag.
 **/
void 
btk_drag_set_icon_pixbuf  (BdkDragContext *context,
			   BdkPixbuf      *pixbuf,
			   gint            hot_x,
			   gint            hot_y)
{
  g_return_if_fail (BDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (BDK_IS_PIXBUF (pixbuf));

  set_icon_stock_pixbuf (context, NULL, pixbuf, hot_x, hot_y, FALSE);
}

/**
 * btk_drag_set_icon_stock:
 * @context: the context for a drag. (This must be called 
 *            with a  context for the source side of a drag)
 * @stock_id: the ID of the stock icon to use for the drag.
 * @hot_x: the X offset within the icon of the hotspot.
 * @hot_y: the Y offset within the icon of the hotspot.
 * 
 * Sets the icon for a given drag from a stock ID.
 **/
void 
btk_drag_set_icon_stock  (BdkDragContext *context,
			  const gchar    *stock_id,
			  gint            hot_x,
			  gint            hot_y)
{
  g_return_if_fail (BDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (stock_id != NULL);
  
  set_icon_stock_pixbuf (context, stock_id, NULL, hot_x, hot_y, FALSE);
}

/**
 * btk_drag_set_icon_pixmap:
 * @context: the context for a drag. (This must be called 
 *            with a  context for the source side of a drag)
 * @colormap: the colormap of the icon 
 * @pixmap: the image data for the icon 
 * @mask: (allow-none): the transparency mask for the icon or %NULL for none.
 * @hot_x: the X offset within @pixmap of the hotspot.
 * @hot_y: the Y offset within @pixmap of the hotspot.
 * 
 * Sets @pixmap as the icon for a given drag. BTK+ retains
 * references for the arguments, and will release them when
 * they are no longer needed. In general, btk_drag_set_icon_pixbuf()
 * will be more convenient to use.
 **/
void 
btk_drag_set_icon_pixmap (BdkDragContext    *context,
			  BdkColormap       *colormap,
			  BdkPixmap         *pixmap,
			  BdkBitmap         *mask,
			  gint               hot_x,
			  gint               hot_y)
{
  BtkWidget *window;
  BdkScreen *screen;
  gint width, height;
      
  g_return_if_fail (BDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (BDK_IS_COLORMAP (colormap));
  g_return_if_fail (BDK_IS_PIXMAP (pixmap));
  g_return_if_fail (!mask || BDK_IS_PIXMAP (mask));

  screen = bdk_colormap_get_screen (colormap);
  
  g_return_if_fail (bdk_drawable_get_screen (pixmap) == screen);
  g_return_if_fail (!mask || bdk_drawable_get_screen (mask) == screen);

  bdk_drawable_get_size (pixmap, &width, &height);

  btk_widget_push_colormap (colormap);

  window = btk_window_new (BTK_WINDOW_POPUP);
  btk_window_set_type_hint (BTK_WINDOW (window), BDK_WINDOW_TYPE_HINT_DND);
  btk_window_set_screen (BTK_WINDOW (window), screen);
  set_can_change_screen (window, FALSE);
  btk_widget_set_events (window, BDK_BUTTON_PRESS_MASK | BDK_BUTTON_RELEASE_MASK);
  btk_widget_set_app_paintable (BTK_WIDGET (window), TRUE);

  btk_widget_pop_colormap ();

  btk_widget_set_size_request (window, width, height);
  btk_widget_realize (window);

  bdk_window_set_back_pixmap (window->window, pixmap, FALSE);
  
  if (mask)
    btk_widget_shape_combine_mask (window, mask, 0, 0);

  btk_drag_set_icon_window (context, window, hot_x, hot_y, TRUE);
}

/**
 * btk_drag_set_icon_name:
 * @context: the context for a drag. (This must be called 
 *            with a context for the source side of a drag)
 * @icon_name: name of icon to use
 * @hot_x: the X offset of the hotspot within the icon
 * @hot_y: the Y offset of the hotspot within the icon
 * 
 * Sets the icon for a given drag from a named themed icon. See
 * the docs for #BtkIconTheme for more details. Note that the
 * size of the icon depends on the icon theme (the icon is
 * loaded at the symbolic size #BTK_ICON_SIZE_DND), thus 
 * @hot_x and @hot_y have to be used with care.
 *
 * Since: 2.8
 **/
void 
btk_drag_set_icon_name (BdkDragContext *context,
			const gchar    *icon_name,
			gint            hot_x,
			gint            hot_y)
{
  BdkScreen *screen;
  BtkSettings *settings;
  BtkIconTheme *icon_theme;
  BdkPixbuf *pixbuf;
  gint width, height, icon_size;

  g_return_if_fail (BDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (icon_name != NULL);

  screen = bdk_window_get_screen (bdk_drag_context_get_source_window (context));
  g_return_if_fail (screen != NULL);

  settings = btk_settings_get_for_screen (screen);
  if (btk_icon_size_lookup_for_settings (settings,
					 BTK_ICON_SIZE_DND,
					 &width, &height))
    icon_size = MAX (width, height);
  else 
    icon_size = 32; /* default value for BTK_ICON_SIZE_DND */ 

  icon_theme = btk_icon_theme_get_for_screen (screen);

  pixbuf = btk_icon_theme_load_icon (icon_theme, icon_name,
		  		     icon_size, 0, NULL);
  if (pixbuf)
    set_icon_stock_pixbuf (context, NULL, pixbuf, hot_x, hot_y, FALSE);
  else
    g_warning ("Cannot load drag icon from icon name %s", icon_name);
}

/**
 * btk_drag_set_icon_default:
 * @context: the context for a drag. (This must be called 
             with a  context for the source side of a drag)
 * 
 * Sets the icon for a particular drag to the default
 * icon.
 **/
void 
btk_drag_set_icon_default (BdkDragContext *context)
{
  g_return_if_fail (BDK_IS_DRAG_CONTEXT (context));

  if (!default_icon_pixmap)
    btk_drag_set_icon_stock (context, BTK_STOCK_DND, -2, -2);
  else
    btk_drag_set_icon_pixmap (context, 
			      default_icon_colormap, 
			      default_icon_pixmap, 
			      default_icon_mask,
			      default_icon_hot_x,
			      default_icon_hot_y);
}

/**
 * btk_drag_set_default_icon:
 * @colormap: the colormap of the icon
 * @pixmap: the image data for the icon
 * @mask: (allow-none): the transparency mask for an image, or %NULL
 * @hot_x: The X offset within @widget of the hotspot.
 * @hot_y: The Y offset within @widget of the hotspot.
 * 
 * Changes the default drag icon. BTK+ retains references for the
 * arguments, and will release them when they are no longer needed.
 *
 * Deprecated: Change the default drag icon via the stock system by 
 *     changing the stock pixbuf for #BTK_STOCK_DND instead.
 **/
void 
btk_drag_set_default_icon (BdkColormap   *colormap,
			   BdkPixmap     *pixmap,
			   BdkBitmap     *mask,
			   gint           hot_x,
			   gint           hot_y)
{
  g_return_if_fail (BDK_IS_COLORMAP (colormap));
  g_return_if_fail (BDK_IS_PIXMAP (pixmap));
  g_return_if_fail (!mask || BDK_IS_PIXMAP (mask));
  
  if (default_icon_colormap)
    g_object_unref (default_icon_colormap);
  if (default_icon_pixmap)
    g_object_unref (default_icon_pixmap);
  if (default_icon_mask)
    g_object_unref (default_icon_mask);

  default_icon_colormap = colormap;
  g_object_ref (colormap);
  
  default_icon_pixmap = pixmap;
  g_object_ref (pixmap);

  default_icon_mask = mask;
  if (mask)
    g_object_ref (mask);
  
  default_icon_hot_x = hot_x;
  default_icon_hot_y = hot_y;
}


/*************************************************************
 * _btk_drag_source_handle_event:
 *     Called from widget event handling code on Drag events
 *     for drag sources.
 *
 *   arguments:
 *     toplevel: Toplevel widget that received the event
 *     event:
 *   results:
 *************************************************************/

void
_btk_drag_source_handle_event (BtkWidget *widget,
			       BdkEvent  *event)
{
  BtkDragSourceInfo *info;
  BdkDragContext *context;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (event != NULL);

  context = event->dnd.context;
  info = btk_drag_get_source_info (context, FALSE);
  if (!info)
    return;

  switch (event->type)
    {
    case BDK_DRAG_STATUS:
      {
	BdkCursor *cursor;

	if (info->proxy_dest)
	  {
	    if (!event->dnd.send_event)
	      {
		if (info->proxy_dest->proxy_drop_wait)
		  {
		    gboolean result = bdk_drag_context_get_selected_action (context) != 0;
		    
		    /* Aha - we can finally pass the MOTIF DROP on... */
		    bdk_drop_reply (info->proxy_dest->context, result, info->proxy_dest->proxy_drop_time);
		    if (result)
		      bdk_drag_drop (info->context, info->proxy_dest->proxy_drop_time);
		    else
		      btk_drag_finish (info->proxy_dest->context, FALSE, FALSE, info->proxy_dest->proxy_drop_time);
		  }
		else
		  {
		    bdk_drag_status (info->proxy_dest->context,
				     bdk_drag_context_get_selected_action (event->dnd.context),
				     event->dnd.time);
		  }
	      }
	  }
	else if (info->have_grab)
	  {
	    cursor = btk_drag_get_cursor (btk_widget_get_display (widget),
					  bdk_drag_context_get_selected_action (event->dnd.context),
					  info);
	    if (info->cursor != cursor)
	      {
		bdk_pointer_grab (widget->window, FALSE,
				  BDK_POINTER_MOTION_MASK |
				  BDK_BUTTON_RELEASE_MASK,
				  NULL,
				  cursor, info->grab_time);
		info->cursor = cursor;
	      }
	    
	    btk_drag_add_update_idle (info);
	  }
      }
      break;
      
    case BDK_DROP_FINISHED:
      btk_drag_drop_finished (info, BTK_DRAG_RESULT_SUCCESS, event->dnd.time);
      break;
    default:
      g_assert_not_reached ();
    }
}

/*************************************************************
 * btk_drag_source_check_selection:
 *     Check if we've set up handlers/claimed the selection
 *     for a given drag. If not, add them.
 *   arguments:
 *     
 *   results:
 *************************************************************/

static void
btk_drag_source_check_selection (BtkDragSourceInfo *info, 
				 BdkAtom            selection,
				 guint32            time)
{
  GList *tmp_list;

  tmp_list = info->selections;
  while (tmp_list)
    {
      if (BDK_POINTER_TO_ATOM (tmp_list->data) == selection)
	return;
      tmp_list = tmp_list->next;
    }

  btk_selection_owner_set_for_display (btk_widget_get_display (info->widget),
				       info->ipc_widget,
				       selection,
				       time);
  info->selections = g_list_prepend (info->selections,
				     GUINT_TO_POINTER (selection));

  tmp_list = info->target_list->list;
  while (tmp_list)
    {
      BtkTargetPair *pair = tmp_list->data;

      btk_selection_add_target (info->ipc_widget,
				selection,
				pair->target,
				pair->info);
      tmp_list = tmp_list->next;
    }
  
  if (bdk_drag_context_get_protocol (info->context) == BDK_DRAG_PROTO_MOTIF)
    {
      btk_selection_add_target (info->ipc_widget,
				selection,
				bdk_atom_intern_static_string ("XmTRANSFER_SUCCESS"),
				TARGET_MOTIF_SUCCESS);
      btk_selection_add_target (info->ipc_widget,
				selection,
				bdk_atom_intern_static_string ("XmTRANSFER_FAILURE"),
				TARGET_MOTIF_FAILURE);
    }

  btk_selection_add_target (info->ipc_widget,
			    selection,
			    bdk_atom_intern_static_string ("DELETE"),
			    TARGET_DELETE);
}

/*************************************************************
 * btk_drag_drop_finished:
 *     Clean up from the drag, and display snapback, if necessary.
 *   arguments:
 *     info:
 *     success:
 *     time:
 *   results:
 *************************************************************/

static void
btk_drag_drop_finished (BtkDragSourceInfo *info,
			BtkDragResult      result,
			guint              time)
{
  gboolean success;

  success = (result == BTK_DRAG_RESULT_SUCCESS);
  btk_drag_source_release_selections (info, time); 

  if (info->proxy_dest)
    {
      /* The time from the event isn't reliable for Xdnd drags */
      btk_drag_finish (info->proxy_dest->context, success, FALSE, 
		       info->proxy_dest->proxy_drop_time);
      btk_drag_source_info_destroy (info);
    }
  else
    {
      if (!success)
	g_signal_emit_by_name (info->widget, "drag-failed",
			       info->context, result, &success);

      if (success)
	{
	  btk_drag_source_info_destroy (info);
	}
      else
	{
	  BtkDragAnim *anim = g_new (BtkDragAnim, 1);
	  anim->info = info;
	  anim->step = 0;
	  
	  anim->n_steps = MAX (info->cur_x - info->start_x,
			       info->cur_y - info->start_y) / ANIM_STEP_LENGTH;
	  anim->n_steps = CLAMP (anim->n_steps, ANIM_MIN_STEPS, ANIM_MAX_STEPS);

	  info->cur_screen = btk_widget_get_screen (info->widget);

	  if (!info->icon_window)
	    set_icon_stock_pixbuf (info->context, NULL, info->icon_pixbuf, 
				   0, 0, TRUE);

	  btk_drag_update_icon (info);
	  
	  /* Mark the context as dead, so if the destination decides
	   * to respond really late, we still are OK.
	   */
	  btk_drag_clear_source_info (info->context);
	  bdk_threads_add_timeout (ANIM_STEP_TIME, btk_drag_anim_timeout, anim);
	}
    }
}

static void
btk_drag_source_release_selections (BtkDragSourceInfo *info,
				    guint32            time)
{
  BdkDisplay *display = btk_widget_get_display (info->widget);
  GList *tmp_list = info->selections;
  
  while (tmp_list)
    {
      BdkAtom selection = BDK_POINTER_TO_ATOM (tmp_list->data);
      if (bdk_selection_owner_get_for_display (display, selection) == info->ipc_widget->window)
	btk_selection_owner_set_for_display (display, NULL, selection, time);

      tmp_list = tmp_list->next;
    }

  g_list_free (info->selections);
  info->selections = NULL;
}

/*************************************************************
 * btk_drag_drop:
 *     Send a drop event.
 *   arguments:
 *     
 *   results:
 *************************************************************/

static void
btk_drag_drop (BtkDragSourceInfo *info, 
	       guint32            time)
{
  if (bdk_drag_context_get_protocol (info->context) == BDK_DRAG_PROTO_ROOTWIN)
    {
      BtkSelectionData selection_data;
      GList *tmp_list;
      /* BTK+ traditionally has used application/x-rootwin-drop, but the
       * XDND spec specifies x-rootwindow-drop.
       */
      BdkAtom target1 = bdk_atom_intern_static_string ("application/x-rootwindow-drop");
      BdkAtom target2 = bdk_atom_intern_static_string ("application/x-rootwin-drop");
      
      tmp_list = info->target_list->list;
      while (tmp_list)
	{
	  BtkTargetPair *pair = tmp_list->data;
	  
	  if (pair->target == target1 || pair->target == target2)
	    {
	      selection_data.selection = BDK_NONE;
	      selection_data.target = pair->target;
	      selection_data.data = NULL;
	      selection_data.length = -1;
	      
	      g_signal_emit_by_name (info->widget, "drag-data-get",
				     info->context, &selection_data,
				     pair->info,
				     time);
	      
	      /* FIXME: Should we check for length >= 0 here? */
	      btk_drag_drop_finished (info, BTK_DRAG_RESULT_SUCCESS, time);
	      return;
	    }
	  tmp_list = tmp_list->next;
	}
      btk_drag_drop_finished (info, BTK_DRAG_RESULT_NO_TARGET, time);
    }
  else
    {
      if (info->icon_window)
	btk_widget_hide (info->icon_window);
	
      bdk_drag_drop (info->context, time);
      info->drop_timeout = bdk_threads_add_timeout (DROP_ABORT_TIME,
					  btk_drag_abort_timeout,
					  info);
    }
}

/*
 * Source side callbacks.
 */

static gboolean
btk_drag_source_event_cb (BtkWidget      *widget,
			  BdkEvent       *event,
			  gpointer        data)
{
  BtkDragSourceSite *site;
  gboolean retval = FALSE;
  site = (BtkDragSourceSite *)data;

  switch (event->type)
    {
    case BDK_BUTTON_PRESS:
      if ((BDK_BUTTON1_MASK << (event->button.button - 1)) & site->start_button_mask)
	{
	  site->state |= (BDK_BUTTON1_MASK << (event->button.button - 1));
	  site->x = event->button.x;
	  site->y = event->button.y;
	}
      break;
      
    case BDK_BUTTON_RELEASE:
      if ((BDK_BUTTON1_MASK << (event->button.button - 1)) & site->start_button_mask)
	site->state &= ~(BDK_BUTTON1_MASK << (event->button.button - 1));
      break;
      
    case BDK_MOTION_NOTIFY:
      if (site->state & event->motion.state & site->start_button_mask)
	{
	  /* FIXME: This is really broken and can leave us
	   * with a stuck grab
	   */
	  int i;
	  for (i=1; i<6; i++)
	    {
	      if (site->state & event->motion.state & 
		  BDK_BUTTON1_MASK << (i - 1))
		break;
	    }

	  if (btk_drag_check_threshold (widget, site->x, site->y,
					event->motion.x, event->motion.y))
	    {
	      site->state = 0;
	      btk_drag_begin_internal (widget, site, site->target_list,
				       site->actions, 
				       i, event);

	      retval = TRUE;
	    }
	}
      break;
      
    default:			/* hit for 2/3BUTTON_PRESS */
      break;
    }
  
  return retval;
}

static void 
btk_drag_source_site_destroy (gpointer data)
{
  BtkDragSourceSite *site = data;

  if (site->target_list)
    btk_target_list_unref (site->target_list);

  btk_drag_source_unset_icon (site);
  g_free (site);
}

static void
btk_drag_selection_get (BtkWidget        *widget, 
			BtkSelectionData *selection_data,
			guint             sel_info,
			guint32           time,
			gpointer          data)
{
  BtkDragSourceInfo *info = data;
  static BdkAtom null_atom = BDK_NONE;
  guint target_info;

  if (!null_atom)
    null_atom = bdk_atom_intern_static_string ("NULL");

  switch (sel_info)
    {
    case TARGET_DELETE:
      g_signal_emit_by_name (info->widget,
			     "drag-data-delete", 
			     info->context);
      btk_selection_data_set (selection_data, null_atom, 8, NULL, 0);
      break;
    case TARGET_MOTIF_SUCCESS:
      btk_drag_drop_finished (info, BTK_DRAG_RESULT_SUCCESS, time);
      btk_selection_data_set (selection_data, null_atom, 8, NULL, 0);
      break;
    case TARGET_MOTIF_FAILURE:
      btk_drag_drop_finished (info, BTK_DRAG_RESULT_NO_TARGET, time);
      btk_selection_data_set (selection_data, null_atom, 8, NULL, 0);
      break;
    default:
      if (info->proxy_dest)
	{
	  /* This is sort of dangerous and needs to be thought
	   * through better
	   */
	  info->proxy_dest->proxy_data = selection_data;
	  btk_drag_get_data (info->widget,
			     info->proxy_dest->context,
			     selection_data->target,
			     time);
	  btk_main ();
	  info->proxy_dest->proxy_data = NULL;
	}
      else
	{
	  if (btk_target_list_find (info->target_list, 
				    selection_data->target, 
				    &target_info))
	    {
	      g_signal_emit_by_name (info->widget, "drag-data-get",
				     info->context,
				     selection_data,
				     target_info,
				     time);
	    }
	}
      break;
    }
}

static gboolean
btk_drag_anim_timeout (gpointer data)
{
  BtkDragAnim *anim = data;
  gint x, y;
  gboolean retval;

  if (anim->step == anim->n_steps)
    {
      btk_drag_source_info_destroy (anim->info);
      g_free (anim);

      retval = FALSE;
    }
  else
    {
      x = (anim->info->start_x * (anim->step + 1) +
	   anim->info->cur_x * (anim->n_steps - anim->step - 1)) / anim->n_steps;
      y = (anim->info->start_y * (anim->step + 1) +
	   anim->info->cur_y * (anim->n_steps - anim->step - 1)) / anim->n_steps;
      if (anim->info->icon_window)
	{
	  BtkWidget *icon_window;
	  gint hot_x, hot_y;
	  
	  btk_drag_get_icon (anim->info, &icon_window, &hot_x, &hot_y);	  
	  btk_window_move (BTK_WINDOW (icon_window), 
			   x - hot_x, 
			   y - hot_y);
	}
  
      anim->step++;

      retval = TRUE;
    }

  return retval;
}

static void
btk_drag_remove_icon (BtkDragSourceInfo *info)
{
  if (info->icon_window)
    {
      btk_widget_hide (info->icon_window);
      if (info->destroy_icon)
	btk_widget_destroy (info->icon_window);

      if (info->fallback_icon)
	{
	  btk_widget_destroy (info->fallback_icon);
	  info->fallback_icon = NULL;
	}

      g_object_unref (info->icon_window);
      info->icon_window = NULL;
    }
}

static void
btk_drag_source_info_destroy (BtkDragSourceInfo *info)
{
  gint i;

  for (i = 0; i < n_drag_cursors; i++)
    {
      if (info->drag_cursors[i] != NULL)
        {
          bdk_cursor_unref (info->drag_cursors[i]);
          info->drag_cursors[i] = NULL;
        }
    }

  btk_drag_remove_icon (info);

  if (info->icon_pixbuf)
    {
      g_object_unref (info->icon_pixbuf);
      info->icon_pixbuf = NULL;
    }

  g_signal_handlers_disconnect_by_func (info->ipc_widget,
					btk_drag_grab_broken_event_cb,
					info);
  g_signal_handlers_disconnect_by_func (info->ipc_widget,
					btk_drag_grab_notify_cb,
					info);
  g_signal_handlers_disconnect_by_func (info->ipc_widget,
					btk_drag_button_release_cb,
					info);
  g_signal_handlers_disconnect_by_func (info->ipc_widget,
					btk_drag_motion_cb,
					info);
  g_signal_handlers_disconnect_by_func (info->ipc_widget,
					btk_drag_key_cb,
					info);
  g_signal_handlers_disconnect_by_func (info->ipc_widget,
					btk_drag_selection_get,
					info);

  if (!info->proxy_dest)
    g_signal_emit_by_name (info->widget, "drag-end", info->context);

  if (info->widget)
    g_object_unref (info->widget);

  btk_selection_remove_all (info->ipc_widget);
  g_object_set_data (G_OBJECT (info->ipc_widget), I_("btk-info"), NULL);
  source_widgets = g_slist_remove (source_widgets, info->ipc_widget);
  btk_drag_release_ipc_widget (info->ipc_widget);

  btk_target_list_unref (info->target_list);

  btk_drag_clear_source_info (info->context);
  g_object_unref (info->context);

  if (info->drop_timeout)
    g_source_remove (info->drop_timeout);

  if (info->update_idle)
    g_source_remove (info->update_idle);

  g_free (info);
}

static gboolean
btk_drag_update_idle (gpointer data)
{
  BtkDragSourceInfo *info = data;
  BdkWindow *dest_window;
  BdkDragProtocol protocol;
  BdkAtom selection;

  BdkDragAction action;
  BdkDragAction possible_actions;
  guint32 time;

  info->update_idle = 0;
    
  if (info->last_event)
    {
      time = btk_drag_get_event_time (info->last_event);
      btk_drag_get_event_actions (info->last_event,
				  info->button, 
				  info->possible_actions,
				  &action, &possible_actions);
      btk_drag_update_icon (info);
      bdk_drag_find_window_for_screen (info->context,
				       info->icon_window ? info->icon_window->window : NULL,
				       info->cur_screen, info->cur_x, info->cur_y,
				       &dest_window, &protocol);
      
      if (!bdk_drag_motion (info->context, dest_window, protocol,
			    info->cur_x, info->cur_y, action, 
			    possible_actions,
			    time))
	{
	  bdk_event_free ((BdkEvent *)info->last_event);
	  info->last_event = NULL;
	}
  
      if (dest_window)
	g_object_unref (dest_window);
      
      selection = bdk_drag_get_selection (info->context);
      if (selection)
	btk_drag_source_check_selection (info, selection, time);

    }

  return FALSE;
}

static void
btk_drag_add_update_idle (BtkDragSourceInfo *info)
{
  /* We use an idle lower than BDK_PRIORITY_REDRAW so that exposes
   * from the last move can catch up before we move again.
   */
  if (!info->update_idle)
    info->update_idle = bdk_threads_add_idle_full (BDK_PRIORITY_REDRAW + 5,
					 btk_drag_update_idle,
					 info,
					 NULL);
}

/**
 * btk_drag_update:
 * @info: DragSourceInfo for the drag
 * @screen: new screen
 * @x_root: new X position 
 * @y_root: new y position
 * @event: event received requiring update
 * 
 * Updates the status of the drag; called when the
 * cursor moves or the modifier changes
 **/
static void
btk_drag_update (BtkDragSourceInfo *info,
		 BdkScreen         *screen,
		 gint               x_root,
		 gint               y_root,
		 BdkEvent          *event)
{
  info->cur_screen = screen;
  info->cur_x = x_root;
  info->cur_y = y_root;
  if (info->last_event)
    {
      bdk_event_free ((BdkEvent *)info->last_event);
      info->last_event = NULL;
    }
  if (event)
    info->last_event = bdk_event_copy ((BdkEvent *)event);

  btk_drag_add_update_idle (info);
}

/*************************************************************
 * btk_drag_end:
 *     Called when the user finishes to drag, either by
 *     releasing the mouse, or by pressing Esc.
 *   arguments:
 *     info: Source info for the drag
 *     time: Timestamp for ending the drag
 *   results:
 *************************************************************/

static void
btk_drag_end (BtkDragSourceInfo *info, guint32 time)
{
  BtkWidget *source_widget = info->widget;
  BdkDisplay *display = btk_widget_get_display (source_widget);

  /* Prevent ungrab before grab (see bug 623865) */
  if (info->grab_time == BDK_CURRENT_TIME)
    time = BDK_CURRENT_TIME;

  if (info->update_idle)
    {
      g_source_remove (info->update_idle);
      info->update_idle = 0;
    }
  
  if (info->last_event)
    {
      bdk_event_free (info->last_event);
      info->last_event = NULL;
    }
  
  info->have_grab = FALSE;
  
  g_signal_handlers_disconnect_by_func (info->ipc_widget,
					btk_drag_grab_broken_event_cb,
					info);
  g_signal_handlers_disconnect_by_func (info->ipc_widget,
					btk_drag_grab_notify_cb,
					info);
  g_signal_handlers_disconnect_by_func (info->ipc_widget,
					btk_drag_button_release_cb,
					info);
  g_signal_handlers_disconnect_by_func (info->ipc_widget,
					btk_drag_motion_cb,
					info);
  g_signal_handlers_disconnect_by_func (info->ipc_widget,
					btk_drag_key_cb,
					info);

  bdk_display_pointer_ungrab (display, time);
  ungrab_dnd_keys (info->ipc_widget, time);
  btk_grab_remove (info->ipc_widget);

  if (btk_widget_get_realized (source_widget))
    {
      BdkEvent *send_event;

      /* Send on a release pair to the original widget to convince it
       * to release its grab. We need to call btk_propagate_event()
       * here, instead of btk_widget_event() because widget like
       * BtkList may expect propagation.
       */

      send_event = bdk_event_new (BDK_BUTTON_RELEASE);
      send_event->button.window = g_object_ref (btk_widget_get_root_window (source_widget));
      send_event->button.send_event = TRUE;
      send_event->button.time = time;
      send_event->button.x = 0;
      send_event->button.y = 0;
      send_event->button.axes = NULL;
      send_event->button.state = 0;
      send_event->button.button = info->button;
      send_event->button.device = bdk_display_get_core_pointer (display);
      send_event->button.x_root = 0;
      send_event->button.y_root = 0;

      btk_propagate_event (source_widget, send_event);
      bdk_event_free (send_event);
    }
}

/*************************************************************
 * btk_drag_cancel:
 *    Called on cancellation of a drag, either by the user
 *    or programmatically.
 *   arguments:
 *     info: Source info for the drag
 *     time: Timestamp for ending the drag
 *   results:
 *************************************************************/

static void
btk_drag_cancel (BtkDragSourceInfo *info, BtkDragResult result, guint32 time)
{
  btk_drag_end (info, time);
  bdk_drag_abort (info->context, time);
  btk_drag_drop_finished (info, result, time);
}

/*************************************************************
 * btk_drag_motion_cb:
 *     "motion-notify-event" callback during drag.
 *   arguments:
 *     
 *   results:
 *************************************************************/

static gboolean
btk_drag_motion_cb (BtkWidget      *widget, 
		    BdkEventMotion *event, 
		    gpointer        data)
{
  BtkDragSourceInfo *info = (BtkDragSourceInfo *)data;
  BdkScreen *screen;
  gint x_root, y_root;

  if (event->is_hint)
    {
      BdkDisplay *display = btk_widget_get_display (widget);
      
      bdk_display_get_pointer (display, &screen, &x_root, &y_root, NULL);
      event->x_root = x_root;
      event->y_root = y_root;
    }
  else
    screen = bdk_event_get_screen ((BdkEvent *)event);

  btk_drag_update (info, screen, event->x_root, event->y_root, (BdkEvent *)event);

  return TRUE;
}

/*************************************************************
 * btk_drag_key_cb:
 *     "key-press/release-event" callback during drag.
 *   arguments:
 *     
 *   results:
 *************************************************************/

#define BIG_STEP 20
#define SMALL_STEP 1

static gboolean
btk_drag_key_cb (BtkWidget         *widget, 
		 BdkEventKey       *event, 
		 gpointer           data)
{
  BtkDragSourceInfo *info = (BtkDragSourceInfo *)data;
  BdkModifierType state;
  BdkWindow *root_window;
  gint dx, dy;

  dx = dy = 0;
  state = event->state & btk_accelerator_get_default_mod_mask ();

  if (event->type == BDK_KEY_PRESS)
    {
      switch (event->keyval)
	{
	case BDK_Escape:
	  btk_drag_cancel (info, BTK_DRAG_RESULT_USER_CANCELLED, event->time);
	  return TRUE;

	case BDK_space:
	case BDK_Return:
        case BDK_ISO_Enter:
	case BDK_KP_Enter:
	case BDK_KP_Space:
	  btk_drag_end (info, event->time);
	  btk_drag_drop (info, event->time);
	  return TRUE;

	case BDK_Up:
	case BDK_KP_Up:
	  dy = (state & BDK_MOD1_MASK) ? -BIG_STEP : -SMALL_STEP;
	  break;
	  
	case BDK_Down:
	case BDK_KP_Down:
	  dy = (state & BDK_MOD1_MASK) ? BIG_STEP : SMALL_STEP;
	  break;
	  
	case BDK_Left:
	case BDK_KP_Left:
	  dx = (state & BDK_MOD1_MASK) ? -BIG_STEP : -SMALL_STEP;
	  break;
	  
	case BDK_Right:
	case BDK_KP_Right:
	  dx = (state & BDK_MOD1_MASK) ? BIG_STEP : SMALL_STEP;
	  break;
	}
      
    }

  /* Now send a "motion" so that the modifier state is updated */

  /* The state is not yet updated in the event, so we need
   * to query it here. We could use XGetModifierMapping, but
   * that would be overkill.
   */
  root_window = btk_widget_get_root_window (widget);
  bdk_window_get_pointer (root_window, NULL, NULL, &state);
  event->state = state;

  if (dx != 0 || dy != 0)
    {
      info->cur_x += dx;
      info->cur_y += dy;
      bdk_display_warp_pointer (btk_widget_get_display (widget), 
				btk_widget_get_screen (widget), 
				info->cur_x, info->cur_y);
    }

  btk_drag_update (info, info->cur_screen, info->cur_x, info->cur_y, (BdkEvent *)event);

  return TRUE;
}

static gboolean
btk_drag_grab_broken_event_cb (BtkWidget          *widget,
			       BdkEventGrabBroken *event,
			       gpointer            data)
{
  BtkDragSourceInfo *info = (BtkDragSourceInfo *)data;

  /* Don't cancel if we break the implicit grab from the initial button_press.
   * Also, don't cancel if we re-grab on the widget or on our IPC window, for
   * example, when changing the drag cursor.
   */
  if (event->implicit
      || event->grab_window == info->widget->window
      || event->grab_window == info->ipc_widget->window)
    return FALSE;

  btk_drag_cancel (info, BTK_DRAG_RESULT_GRAB_BROKEN, btk_get_current_event_time ());
  return TRUE;
}

static void
btk_drag_grab_notify_cb (BtkWidget        *widget,
			 gboolean          was_grabbed,
			 gpointer          data)
{
  BtkDragSourceInfo *info = (BtkDragSourceInfo *)data;

  if (!was_grabbed)
    {
      /* We have to block callbacks to avoid recursion here, because
	 btk_drag_cancel calls btk_grab_remove (via btk_drag_end) */
      g_signal_handlers_block_by_func (widget, btk_drag_grab_notify_cb, data);
      btk_drag_cancel (info, BTK_DRAG_RESULT_GRAB_BROKEN, btk_get_current_event_time ());
      g_signal_handlers_unblock_by_func (widget, btk_drag_grab_notify_cb, data);
    }
}


/*************************************************************
 * btk_drag_button_release_cb:
 *     "button-release-event" callback during drag.
 *   arguments:
 *     
 *   results:
 *************************************************************/

static gboolean
btk_drag_button_release_cb (BtkWidget      *widget, 
			    BdkEventButton *event, 
			    gpointer        data)
{
  BtkDragSourceInfo *info = (BtkDragSourceInfo *)data;

  if (event->button != info->button)
    return FALSE;

  if ((bdk_drag_context_get_selected_action (info->context) != 0) &&
      (bdk_drag_context_get_dest_window (info->context) != NULL))
    {
      btk_drag_end (info, event->time);
      btk_drag_drop (info, event->time);
    }
  else
    {
      btk_drag_cancel (info, BTK_DRAG_RESULT_NO_TARGET, event->time);
    }

  return TRUE;
}

static gboolean
btk_drag_abort_timeout (gpointer data)
{
  BtkDragSourceInfo *info = data;
  guint32 time = BDK_CURRENT_TIME;

  if (info->proxy_dest)
    time = info->proxy_dest->proxy_drop_time;

  info->drop_timeout = 0;
  btk_drag_drop_finished (info, BTK_DRAG_RESULT_TIMEOUT_EXPIRED, time);
  
  return FALSE;
}

/**
 * btk_drag_check_threshold:
 * @widget: a #BtkWidget
 * @start_x: X coordinate of start of drag
 * @start_y: Y coordinate of start of drag
 * @current_x: current X coordinate
 * @current_y: current Y coordinate
 * 
 * Checks to see if a mouse drag starting at (@start_x, @start_y) and ending
 * at (@current_x, @current_y) has passed the BTK+ drag threshold, and thus
 * should trigger the beginning of a drag-and-drop operation.
 *
 * Return Value: %TRUE if the drag threshold has been passed.
 **/
gboolean
btk_drag_check_threshold (BtkWidget *widget,
			  gint       start_x,
			  gint       start_y,
			  gint       current_x,
			  gint       current_y)
{
  gint drag_threshold;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  g_object_get (btk_widget_get_settings (widget),
		"btk-dnd-drag-threshold", &drag_threshold,
		NULL);
  
  return (ABS (current_x - start_x) > drag_threshold ||
	  ABS (current_y - start_y) > drag_threshold);
}

#define __BTK_DND_C__
#include "btkaliasdef.c"
