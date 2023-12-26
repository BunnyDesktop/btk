/* BDK - The GIMP Drawing Kit
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

#ifndef __BDK_EVENTS_H__
#define __BDK_EVENTS_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bdk/bdkcolor.h>
#include <bdk/bdktypes.h>
#include <bdk/bdkdnd.h>
#include <bdk/bdkinput.h>

B_BEGIN_DECLS

#define BDK_TYPE_EVENT          (bdk_event_get_type ())

#define BDK_PRIORITY_EVENTS	(G_PRIORITY_DEFAULT)
#define BDK_PRIORITY_REDRAW     (G_PRIORITY_HIGH_IDLE + 20)


typedef struct _BdkEventAny	    BdkEventAny;
typedef struct _BdkEventExpose	    BdkEventExpose;
typedef struct _BdkEventNoExpose    BdkEventNoExpose;
typedef struct _BdkEventVisibility  BdkEventVisibility;
typedef struct _BdkEventMotion	    BdkEventMotion;
typedef struct _BdkEventButton	    BdkEventButton;
typedef struct _BdkEventScroll      BdkEventScroll;  
typedef struct _BdkEventKey	    BdkEventKey;
typedef struct _BdkEventFocus	    BdkEventFocus;
typedef struct _BdkEventCrossing    BdkEventCrossing;
typedef struct _BdkEventConfigure   BdkEventConfigure;
typedef struct _BdkEventProperty    BdkEventProperty;
typedef struct _BdkEventSelection   BdkEventSelection;
typedef struct _BdkEventOwnerChange BdkEventOwnerChange;
typedef struct _BdkEventProximity   BdkEventProximity;
typedef struct _BdkEventClient	    BdkEventClient;
typedef struct _BdkEventDND         BdkEventDND;
typedef struct _BdkEventWindowState BdkEventWindowState;
typedef struct _BdkEventSetting     BdkEventSetting;
typedef struct _BdkEventGrabBroken  BdkEventGrabBroken;

typedef union  _BdkEvent	    BdkEvent;

typedef void (*BdkEventFunc) (BdkEvent *event,
			      bpointer	data);

/* Event filtering */

typedef void BdkXEvent;	  /* Can be cast to window system specific
			   * even type, XEvent on X11, MSG on Win32.
			   */

typedef enum {
  BDK_FILTER_CONTINUE,	  /* Event not handled, continue processesing */
  BDK_FILTER_TRANSLATE,	  /* Native event translated into a BDK event and
                             stored in the "event" structure that was
                             passed in */
  BDK_FILTER_REMOVE	  /* Terminate processing, removing event */
} BdkFilterReturn;

typedef BdkFilterReturn (*BdkFilterFunc) (BdkXEvent *xevent,
					  BdkEvent *event,
					  bpointer  data);


/* Event types.
 *   Nothing: No event occurred.
 *   Delete: A window delete event was sent by the window manager.
 *	     The specified window should be deleted.
 *   Destroy: A window has been destroyed.
 *   Expose: Part of a window has been uncovered.
 *   NoExpose: Same as expose, but no expose event was generated.
 *   VisibilityNotify: A window has become fully/partially/not obscured.
 *   MotionNotify: The mouse has moved.
 *   ButtonPress: A mouse button was pressed.
 *   ButtonRelease: A mouse button was release.
 *   KeyPress: A key was pressed.
 *   KeyRelease: A key was released.
 *   EnterNotify: A window was entered.
 *   LeaveNotify: A window was exited.
 *   FocusChange: The focus window has changed. (The focus window gets
 *		  keyboard events).
 *   Resize: A window has been resized.
 *   Map: A window has been mapped. (It is now visible on the screen).
 *   Unmap: A window has been unmapped. (It is no longer visible on
 *	    the screen).
 *   Scroll: A mouse wheel was scrolled either up or down.
 */
typedef enum
{
  BDK_NOTHING		= -1,
  BDK_DELETE		= 0,
  BDK_DESTROY		= 1,
  BDK_EXPOSE		= 2,
  BDK_MOTION_NOTIFY	= 3,
  BDK_BUTTON_PRESS	= 4,
  BDK_2BUTTON_PRESS	= 5,
  BDK_3BUTTON_PRESS	= 6,
  BDK_BUTTON_RELEASE	= 7,
  BDK_KEY_PRESS		= 8,
  BDK_KEY_RELEASE	= 9,
  BDK_ENTER_NOTIFY	= 10,
  BDK_LEAVE_NOTIFY	= 11,
  BDK_FOCUS_CHANGE	= 12,
  BDK_CONFIGURE		= 13,
  BDK_MAP		= 14,
  BDK_UNMAP		= 15,
  BDK_PROPERTY_NOTIFY	= 16,
  BDK_SELECTION_CLEAR	= 17,
  BDK_SELECTION_REQUEST = 18,
  BDK_SELECTION_NOTIFY	= 19,
  BDK_PROXIMITY_IN	= 20,
  BDK_PROXIMITY_OUT	= 21,
  BDK_DRAG_ENTER        = 22,
  BDK_DRAG_LEAVE        = 23,
  BDK_DRAG_MOTION       = 24,
  BDK_DRAG_STATUS       = 25,
  BDK_DROP_START        = 26,
  BDK_DROP_FINISHED     = 27,
  BDK_CLIENT_EVENT	= 28,
  BDK_VISIBILITY_NOTIFY = 29,
  BDK_NO_EXPOSE		= 30,
  BDK_SCROLL            = 31,
  BDK_WINDOW_STATE      = 32,
  BDK_SETTING           = 33,
  BDK_OWNER_CHANGE      = 34,
  BDK_GRAB_BROKEN       = 35,
  BDK_DAMAGE            = 36,
  BDK_EVENT_LAST        /* helper variable for decls */
} BdkEventType;

/* Event masks. (Used to select what types of events a window
 *  will receive).
 */
typedef enum
{
  BDK_EXPOSURE_MASK		= 1 << 1,
  BDK_POINTER_MOTION_MASK	= 1 << 2,
  BDK_POINTER_MOTION_HINT_MASK	= 1 << 3,
  BDK_BUTTON_MOTION_MASK	= 1 << 4,
  BDK_BUTTON1_MOTION_MASK	= 1 << 5,
  BDK_BUTTON2_MOTION_MASK	= 1 << 6,
  BDK_BUTTON3_MOTION_MASK	= 1 << 7,
  BDK_BUTTON_PRESS_MASK		= 1 << 8,
  BDK_BUTTON_RELEASE_MASK	= 1 << 9,
  BDK_KEY_PRESS_MASK		= 1 << 10,
  BDK_KEY_RELEASE_MASK		= 1 << 11,
  BDK_ENTER_NOTIFY_MASK		= 1 << 12,
  BDK_LEAVE_NOTIFY_MASK		= 1 << 13,
  BDK_FOCUS_CHANGE_MASK		= 1 << 14,
  BDK_STRUCTURE_MASK		= 1 << 15,
  BDK_PROPERTY_CHANGE_MASK	= 1 << 16,
  BDK_VISIBILITY_NOTIFY_MASK	= 1 << 17,
  BDK_PROXIMITY_IN_MASK		= 1 << 18,
  BDK_PROXIMITY_OUT_MASK	= 1 << 19,
  BDK_SUBSTRUCTURE_MASK		= 1 << 20,
  BDK_SCROLL_MASK               = 1 << 21,
  BDK_ALL_EVENTS_MASK		= 0x3FFFFE
} BdkEventMask;

typedef enum
{
  BDK_VISIBILITY_UNOBSCURED,
  BDK_VISIBILITY_PARTIAL,
  BDK_VISIBILITY_FULLY_OBSCURED
} BdkVisibilityState;

typedef enum
{
  BDK_SCROLL_UP,
  BDK_SCROLL_DOWN,
  BDK_SCROLL_LEFT,
  BDK_SCROLL_RIGHT
} BdkScrollDirection;

/* Types of enter/leave notifications.
 *   Ancestor:
 *   Virtual:
 *   Inferior:
 *   Nonlinear:
 *   NonlinearVirtual:
 *   Unknown: An unknown type of enter/leave event occurred.
 */
typedef enum
{
  BDK_NOTIFY_ANCESTOR		= 0,
  BDK_NOTIFY_VIRTUAL		= 1,
  BDK_NOTIFY_INFERIOR		= 2,
  BDK_NOTIFY_NONLINEAR		= 3,
  BDK_NOTIFY_NONLINEAR_VIRTUAL	= 4,
  BDK_NOTIFY_UNKNOWN		= 5
} BdkNotifyType;

/* Enter/leave event modes.
 *   NotifyNormal
 *   NotifyGrab
 *   NotifyUngrab
 */
typedef enum
{
  BDK_CROSSING_NORMAL,
  BDK_CROSSING_GRAB,
  BDK_CROSSING_UNGRAB,
  BDK_CROSSING_BTK_GRAB,
  BDK_CROSSING_BTK_UNGRAB,
  BDK_CROSSING_STATE_CHANGED
} BdkCrossingMode;

typedef enum
{
  BDK_PROPERTY_NEW_VALUE,
  BDK_PROPERTY_DELETE
} BdkPropertyState;

typedef enum
{
  BDK_WINDOW_STATE_WITHDRAWN  = 1 << 0,
  BDK_WINDOW_STATE_ICONIFIED  = 1 << 1,
  BDK_WINDOW_STATE_MAXIMIZED  = 1 << 2,
  BDK_WINDOW_STATE_STICKY     = 1 << 3,
  BDK_WINDOW_STATE_FULLSCREEN = 1 << 4,
  BDK_WINDOW_STATE_ABOVE      = 1 << 5,
  BDK_WINDOW_STATE_BELOW      = 1 << 6
} BdkWindowState;

typedef enum
{
  BDK_SETTING_ACTION_NEW,
  BDK_SETTING_ACTION_CHANGED,
  BDK_SETTING_ACTION_DELETED
} BdkSettingAction;

typedef enum
{
  BDK_OWNER_CHANGE_NEW_OWNER,
  BDK_OWNER_CHANGE_DESTROY,
  BDK_OWNER_CHANGE_CLOSE
} BdkOwnerChange;

struct _BdkEventAny
{
  BdkEventType type;
  BdkWindow *window;
  bint8 send_event;
};

struct _BdkEventExpose
{
  BdkEventType type;
  BdkWindow *window;
  bint8 send_event;
  BdkRectangle area;
  BdkRebunnyion *rebunnyion;
  bint count; /* If non-zero, how many more events follow. */
};

struct _BdkEventNoExpose
{
  BdkEventType type;
  BdkWindow *window;
  bint8 send_event;
};

struct _BdkEventVisibility
{
  BdkEventType type;
  BdkWindow *window;
  bint8 send_event;
  BdkVisibilityState state;
};

struct _BdkEventMotion
{
  BdkEventType type;
  BdkWindow *window;
  bint8 send_event;
  buint32 time;
  bdouble x;
  bdouble y;
  bdouble *axes;
  buint state;
  bint16 is_hint;
  BdkDevice *device;
  bdouble x_root, y_root;
};

struct _BdkEventButton
{
  BdkEventType type;
  BdkWindow *window;
  bint8 send_event;
  buint32 time;
  bdouble x;
  bdouble y;
  bdouble *axes;
  buint state;
  buint button;
  BdkDevice *device;
  bdouble x_root, y_root;
};

struct _BdkEventScroll
{
  BdkEventType type;
  BdkWindow *window;
  bint8 send_event;
  buint32 time;
  bdouble x;
  bdouble y;
  buint state;
  BdkScrollDirection direction;
  BdkDevice *device;
  bdouble x_root, y_root;
};

struct _BdkEventKey
{
  BdkEventType type;
  BdkWindow *window;
  bint8 send_event;
  buint32 time;
  buint state;
  buint keyval;
  bint length;
  bchar *string;
  buint16 hardware_keycode;
  buint8 group;
  buint is_modifier : 1;
};

struct _BdkEventCrossing
{
  BdkEventType type;
  BdkWindow *window;
  bint8 send_event;
  BdkWindow *subwindow;
  buint32 time;
  bdouble x;
  bdouble y;
  bdouble x_root;
  bdouble y_root;
  BdkCrossingMode mode;
  BdkNotifyType detail;
  bboolean focus;
  buint state;
};

struct _BdkEventFocus
{
  BdkEventType type;
  BdkWindow *window;
  bint8 send_event;
  bint16 in;
};

struct _BdkEventConfigure
{
  BdkEventType type;
  BdkWindow *window;
  bint8 send_event;
  bint x, y;
  bint width;
  bint height;
};

struct _BdkEventProperty
{
  BdkEventType type;
  BdkWindow *window;
  bint8 send_event;
  BdkAtom atom;
  buint32 time;
  buint state;
};

struct _BdkEventSelection
{
  BdkEventType type;
  BdkWindow *window;
  bint8 send_event;
  BdkAtom selection;
  BdkAtom target;
  BdkAtom property;
  buint32 time;
  BdkNativeWindow requestor;
};

struct _BdkEventOwnerChange
{
  BdkEventType type;
  BdkWindow *window;
  bint8 send_event;
  BdkNativeWindow owner;
  BdkOwnerChange reason;
  BdkAtom selection;
  buint32 time;
  buint32 selection_time;
};

/* This event type will be used pretty rarely. It only is important
   for XInput aware programs that are drawing their own cursor */

struct _BdkEventProximity
{
  BdkEventType type;
  BdkWindow *window;
  bint8 send_event;
  buint32 time;
  BdkDevice *device;
};

struct _BdkEventClient
{
  BdkEventType type;
  BdkWindow *window;
  bint8 send_event;
  BdkAtom message_type;
  bushort data_format;
  union {
    char b[20];
    short s[10];
    long l[5];
  } data;
};

struct _BdkEventSetting
{
  BdkEventType type;
  BdkWindow *window;
  bint8 send_event;
  BdkSettingAction action;
  char *name;
};

struct _BdkEventWindowState
{
  BdkEventType type;
  BdkWindow *window;
  bint8 send_event;
  BdkWindowState changed_mask;
  BdkWindowState new_window_state;
};

struct _BdkEventGrabBroken {
  BdkEventType type;
  BdkWindow *window;
  bint8 send_event;
  bboolean keyboard;
  bboolean implicit;
  BdkWindow *grab_window;
};

/* Event types for DND */

struct _BdkEventDND {
  BdkEventType type;
  BdkWindow *window;
  bint8 send_event;
  BdkDragContext *context;

  buint32 time;
  bshort x_root, y_root;
};

union _BdkEvent
{
  BdkEventType		    type;
  BdkEventAny		    any;
  BdkEventExpose	    expose;
  BdkEventNoExpose	    no_expose;
  BdkEventVisibility	    visibility;
  BdkEventMotion	    motion;
  BdkEventButton	    button;
  BdkEventScroll            scroll;
  BdkEventKey		    key;
  BdkEventCrossing	    crossing;
  BdkEventFocus		    focus_change;
  BdkEventConfigure	    configure;
  BdkEventProperty	    property;
  BdkEventSelection	    selection;
  BdkEventOwnerChange  	    owner_change;
  BdkEventProximity	    proximity;
  BdkEventClient	    client;
  BdkEventDND               dnd;
  BdkEventWindowState       window_state;
  BdkEventSetting           setting;
  BdkEventGrabBroken        grab_broken;
};

GType     bdk_event_get_type            (void) B_GNUC_CONST;

bboolean  bdk_events_pending	 	(void);
BdkEvent* bdk_event_get			(void);

BdkEvent* bdk_event_peek                (void);
#ifndef BDK_DISABLE_DEPRECATED
BdkEvent* bdk_event_get_graphics_expose (BdkWindow 	*window);
#endif
void      bdk_event_put	 		(const BdkEvent *event);

BdkEvent* bdk_event_new                 (BdkEventType    type);
BdkEvent* bdk_event_copy     		(const BdkEvent *event);
void	  bdk_event_free     		(BdkEvent 	*event);

buint32   bdk_event_get_time            (const BdkEvent  *event);
bboolean  bdk_event_get_state           (const BdkEvent  *event,
                                         BdkModifierType *state);
bboolean  bdk_event_get_coords		(const BdkEvent  *event,
					 bdouble	 *x_win,
					 bdouble	 *y_win);
bboolean  bdk_event_get_root_coords	(const BdkEvent  *event,
					 bdouble	 *x_root,
					 bdouble	 *y_root);
bboolean  bdk_event_get_axis            (const BdkEvent  *event,
                                         BdkAxisUse       axis_use,
                                         bdouble         *value);
void      bdk_event_request_motions     (const BdkEventMotion *event);
void	  bdk_event_handler_set 	(BdkEventFunc    func,
					 bpointer        data,
					 GDestroyNotify  notify);

void       bdk_event_set_screen         (BdkEvent        *event,
                                         BdkScreen       *screen);
BdkScreen *bdk_event_get_screen         (const BdkEvent  *event);

void	  bdk_set_show_events		(bboolean	 show_events);
bboolean  bdk_get_show_events		(void);

#ifndef BDK_MULTIHEAD_SAFE
void bdk_add_client_message_filter (BdkAtom       message_type,
				    BdkFilterFunc func,
				    bpointer      data);

bboolean bdk_setting_get (const bchar *name,
			  BValue      *value); 
#endif /* BDK_MULTIHEAD_SAFE */

B_END_DECLS

#endif /* __BDK_EVENTS_H__ */
