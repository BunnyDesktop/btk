/* BDK - The GIMP Drawing Kit
 * bdkdisplay-x11.c
 * 
 * Copyright 2001 Sun Microsystems Inc.
 * Copyright (C) 2004 Nokia Corporation
 *
 * Erwann Chenede <erwann.chenede@sun.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <bunnylib.h>
#include "bdkx.h"
#include "bdkasync.h"
#include "bdkdisplay.h"
#include "bdkdisplay-x11.h"
#include "bdkscreen.h"
#include "bdkscreen-x11.h"
#include "bdkinternals.h"
#include "bdkinputprivate.h"
#include "xsettings-client.h"
#include "bdkalias.h"

#include <X11/Xatom.h>

#ifdef HAVE_XKB
#include <X11/XKBlib.h>
#endif

#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif

#include <X11/extensions/shape.h>

#ifdef HAVE_XCOMPOSITE
#include <X11/extensions/Xcomposite.h>
#endif

#ifdef HAVE_XDAMAGE
#include <X11/extensions/Xdamage.h>
#endif

#ifdef HAVE_RANDR
#include <X11/extensions/Xrandr.h>
#endif


static void   bdk_display_x11_dispose            (BObject            *object);
static void   bdk_display_x11_finalize           (BObject            *object);

#ifdef HAVE_X11R6
static void bdk_internal_connection_watch (Display  *display,
					   XPointer  arg,
					   gint      fd,
					   gboolean  opening,
					   XPointer *watch_data);
#endif /* HAVE_X11R6 */

/* Note that we never *directly* use WM_LOCALE_NAME, WM_PROTOCOLS,
 * but including them here has the side-effect of getting them
 * into the internal Xlib cache
 */
static const char *const precache_atoms[] = {
  "UTF8_STRING",
  "WM_CLIENT_LEADER",
  "WM_DELETE_WINDOW",
  "WM_ICON_NAME",
  "WM_LOCALE_NAME",
  "WM_NAME",
  "WM_PROTOCOLS",
  "WM_TAKE_FOCUS",
  "WM_WINDOW_ROLE",
  "_NET_ACTIVE_WINDOW",
  "_NET_CURRENT_DESKTOP",
  "_NET_FRAME_EXTENTS",
  "_NET_STARTUP_ID",
  "_NET_WM_CM_S0",
  "_NET_WM_DESKTOP",
  "_NET_WM_ICON",
  "_NET_WM_ICON_NAME",
  "_NET_WM_NAME",
  "_NET_WM_PID",
  "_NET_WM_PING",
  "_NET_WM_STATE",
  "_NET_WM_STATE_ABOVE",
  "_NET_WM_STATE_BELOW",
  "_NET_WM_STATE_FULLSCREEN",
  "_NET_WM_STATE_MODAL",
  "_NET_WM_STATE_MAXIMIZED_VERT",
  "_NET_WM_STATE_MAXIMIZED_HORZ",
  "_NET_WM_STATE_SKIP_TASKBAR",
  "_NET_WM_STATE_SKIP_PAGER",
  "_NET_WM_STATE_STICKY",
  "_NET_WM_SYNC_REQUEST",
  "_NET_WM_SYNC_REQUEST_COUNTER",
  "_NET_WM_WINDOW_TYPE",
  "_NET_WM_WINDOW_TYPE_NORMAL",
  "_NET_WM_USER_TIME",
  "_NET_VIRTUAL_ROOTS"
};

G_DEFINE_TYPE (BdkDisplayX11, _bdk_display_x11, BDK_TYPE_DISPLAY)

static void
_bdk_display_x11_class_init (BdkDisplayX11Class * class)
{
  BObjectClass *object_class = B_OBJECT_CLASS (class);
  
  object_class->dispose = bdk_display_x11_dispose;
  object_class->finalize = bdk_display_x11_finalize;
}

static void
_bdk_display_x11_init (BdkDisplayX11 *display)
{
}

/**
 * bdk_display_open:
 * @display_name: the name of the display to open
 * @returns: a #BdkDisplay, or %NULL if the display
 *  could not be opened.
 *
 * Opens a display.
 *
 * Since: 2.2
 */
BdkDisplay *
bdk_display_open (const gchar *display_name)
{
  Display *xdisplay;
  BdkDisplay *display;
  BdkDisplayX11 *display_x11;
  BdkWindowAttr attr;
  gint argc;
  gchar *argv[1];
  const char *sm_client_id;
  
  XClassHint *class_hint;
  gulong pid;
  gint i;
  gint ignore;
  gint maj, min;

  xdisplay = XOpenDisplay (display_name);
  if (!xdisplay)
    return NULL;
  
  display = g_object_new (BDK_TYPE_DISPLAY_X11, NULL);
  display_x11 = BDK_DISPLAY_X11 (display);

  display_x11->use_xshm = TRUE;
  display_x11->xdisplay = xdisplay;

#ifdef HAVE_X11R6  
  /* Set up handlers for Xlib internal connections */
  XAddConnectionWatch (xdisplay, bdk_internal_connection_watch, NULL);
#endif /* HAVE_X11R6 */
  
  _bdk_x11_precache_atoms (display, precache_atoms, G_N_ELEMENTS (precache_atoms));

  /* RandR must be initialized before we initialize the screens */
  display_x11->have_randr13 = FALSE;
  display_x11->have_randr15 = FALSE;
#ifdef HAVE_RANDR
  if (XRRQueryExtension (display_x11->xdisplay,
			 &display_x11->xrandr_event_base, &ignore))
  {
      int major, minor;
      
      XRRQueryVersion (display_x11->xdisplay, &major, &minor);

      if ((major == 1 && minor >= 3) || major > 1)
	  display_x11->have_randr13 = TRUE;

#ifdef HAVE_RANDR15
      if (minor >= 5 || major > 1)
	display_x11->have_randr15 = TRUE;
#endif

       bdk_x11_register_standard_event_type (display, display_x11->xrandr_event_base, RRNumberEvents);
  }
#endif
  
  /* initialize the display's screens */ 
  display_x11->screens = g_new (BdkScreen *, ScreenCount (display_x11->xdisplay));
  for (i = 0; i < ScreenCount (display_x11->xdisplay); i++)
    display_x11->screens[i] = _bdk_x11_screen_new (display, i);

  /* We need to initialize events after we have the screen
   * structures in places
   */
  for (i = 0; i < ScreenCount (display_x11->xdisplay); i++)
    _bdk_x11_events_init_screen (display_x11->screens[i]);
  
  /*set the default screen */
  display_x11->default_screen = display_x11->screens[DefaultScreen (display_x11->xdisplay)];

  attr.window_type = BDK_WINDOW_TOPLEVEL;
  attr.wclass = BDK_INPUT_OUTPUT;
  attr.x = 10;
  attr.y = 10;
  attr.width = 10;
  attr.height = 10;
  attr.event_mask = 0;

  display_x11->leader_bdk_window = bdk_window_new (BDK_SCREEN_X11 (display_x11->default_screen)->root_window, 
						   &attr, BDK_WA_X | BDK_WA_Y);
  (_bdk_x11_window_get_toplevel (display_x11->leader_bdk_window))->is_leader = TRUE;

  display_x11->leader_window = BDK_WINDOW_XID (display_x11->leader_bdk_window);

  display_x11->leader_window_title_set = FALSE;

  display_x11->have_render = BDK_UNKNOWN;

#ifdef HAVE_XFIXES
  if (XFixesQueryExtension (display_x11->xdisplay, 
			    &display_x11->xfixes_event_base, 
			    &ignore))
    {
      display_x11->have_xfixes = TRUE;

      bdk_x11_register_standard_event_type (display,
					    display_x11->xfixes_event_base, 
					    XFixesNumberEvents);
    }
  else
#endif
    display_x11->have_xfixes = FALSE;

#ifdef HAVE_XCOMPOSITE
  if (XCompositeQueryExtension (display_x11->xdisplay,
				&ignore, &ignore))
    {
      int major, minor;

      XCompositeQueryVersion (display_x11->xdisplay, &major, &minor);

      /* Prior to Composite version 0.4, composited windows clipped their
       * parents, so you had to use IncludeInferiors to draw to the parent
       * This isn't useful for our purposes, so require 0.4
       */
      display_x11->have_xcomposite = major > 0 || (major == 0 && minor >= 4);
    }
  else
#endif
    display_x11->have_xcomposite = FALSE;

#ifdef HAVE_XDAMAGE
  if (XDamageQueryExtension (display_x11->xdisplay,
			     &display_x11->xdamage_event_base,
			     &ignore))
    {
      display_x11->have_xdamage = TRUE;

      bdk_x11_register_standard_event_type (display,
					    display_x11->xdamage_event_base,
					    XDamageNumberEvents);
    }
  else
#endif
    display_x11->have_xdamage = FALSE;

  display_x11->have_shapes = FALSE;
  display_x11->have_input_shapes = FALSE;

  if (XShapeQueryExtension (BDK_DISPLAY_XDISPLAY (display), &display_x11->shape_event_base, &ignore))
    {
      display_x11->have_shapes = TRUE;
#ifdef ShapeInput
      if (XShapeQueryVersion (BDK_DISPLAY_XDISPLAY (display), &maj, &min))
	display_x11->have_input_shapes = (maj == 1 && min >= 1);
#endif
    }

  display_x11->trusted_client = TRUE;
  {
    Window root, child;
    int rootx, rooty, winx, winy;
    unsigned int xmask;

    bdk_error_trap_push ();
    XQueryPointer (display_x11->xdisplay, 
		   BDK_SCREEN_X11 (display_x11->default_screen)->xroot_window,
		   &root, &child, &rootx, &rooty, &winx, &winy, &xmask);
    bdk_flush ();
    if (B_UNLIKELY (bdk_error_trap_pop () == BadWindow)) 
      {
	g_warning ("Connection to display %s appears to be untrusted. Pointer and keyboard grabs and inter-client communication may not work as expected.", bdk_display_get_name (display));
	display_x11->trusted_client = FALSE;
      }
  }

  if (_bdk_synchronize)
    XSynchronize (display_x11->xdisplay, True);
  
  class_hint = XAllocClassHint();
  class_hint->res_name = g_get_prgname ();
  
  class_hint->res_class = (char *)bdk_get_program_class ();

  /* XmbSetWMProperties sets the RESOURCE_NAME environment variable
   * from argv[0], so we just synthesize an argument array here.
   */
  argc = 1;
  argv[0] = g_get_prgname ();
  
  XmbSetWMProperties (display_x11->xdisplay,
		      display_x11->leader_window,
		      NULL, NULL, argv, argc, NULL, NULL,
		      class_hint);
  XFree (class_hint);

  sm_client_id = _bdk_get_sm_client_id ();
  if (sm_client_id)
    _bdk_windowing_display_set_sm_client_id (display, sm_client_id);

  pid = getpid ();
  XChangeProperty (display_x11->xdisplay,
		   display_x11->leader_window,
		   bdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_PID"),
		   XA_CARDINAL, 32, PropModeReplace, (guchar *) & pid, 1);

  /* We don't yet know a valid time. */
  display_x11->user_time = 0;
  
#ifdef HAVE_XKB
  {
    gint xkb_major = XkbMajorVersion;
    gint xkb_minor = XkbMinorVersion;
    if (XkbLibraryVersion (&xkb_major, &xkb_minor))
      {
        xkb_major = XkbMajorVersion;
        xkb_minor = XkbMinorVersion;
	    
        if (XkbQueryExtension (display_x11->xdisplay, 
			       NULL, &display_x11->xkb_event_type, NULL,
                               &xkb_major, &xkb_minor))
          {
	    Bool detectable_autorepeat_supported;
	    
	    display_x11->use_xkb = TRUE;

            XkbSelectEvents (display_x11->xdisplay,
                             XkbUseCoreKbd,
                             XkbNewKeyboardNotifyMask | XkbMapNotifyMask | XkbStateNotifyMask,
                             XkbNewKeyboardNotifyMask | XkbMapNotifyMask | XkbStateNotifyMask);

	    /* keep this in sync with _bdk_keymap_state_changed() */ 
	    XkbSelectEventDetails (display_x11->xdisplay,
				   XkbUseCoreKbd, XkbStateNotify,
				   XkbAllStateComponentsMask,
                                   XkbGroupLockMask|XkbModifierLockMask);

	    XkbSetDetectableAutoRepeat (display_x11->xdisplay,
					True,
					&detectable_autorepeat_supported);

	    BDK_NOTE (MISC, g_message ("Detectable autorepeat %s.",
				       detectable_autorepeat_supported ? 
				       "supported" : "not supported"));
	    
	    display_x11->have_xkb_autorepeat = detectable_autorepeat_supported;
          }
      }
  }
#endif

  display_x11->use_sync = FALSE;
#ifdef HAVE_XSYNC
  {
    int major, minor;
    int error_base, event_base;
    
    if (XSyncQueryExtension (display_x11->xdisplay,
			     &event_base, &error_base) &&
        XSyncInitialize (display_x11->xdisplay,
                         &major, &minor))
      display_x11->use_sync = TRUE;
  }
#endif
  
  _bdk_windowing_image_init (display);
  _bdk_events_init (display);
  _bdk_input_init (display);
  _bdk_dnd_init (display);

  for (i = 0; i < ScreenCount (display_x11->xdisplay); i++)
    _bdk_x11_screen_setup (display_x11->screens[i]);

  g_signal_emit_by_name (bdk_display_manager_get(),
			 "display_opened", display);

  return display;
}

#ifdef HAVE_X11R6
/*
 * XLib internal connection handling
 */
typedef struct _BdkInternalConnection BdkInternalConnection;

struct _BdkInternalConnection
{
  gint	         fd;
  GSource	*source;
  Display	*display;
};

static gboolean
process_internal_connection (BUNNYIOChannel  *bunnyioc,
			     BUNNYIOCondition cond,
			     gpointer     data)
{
  BdkInternalConnection *connection = (BdkInternalConnection *)data;

  BDK_THREADS_ENTER ();

  XProcessInternalConnection ((Display*)connection->display, connection->fd);

  BDK_THREADS_LEAVE ();

  return TRUE;
}

gulong
_bdk_windowing_window_get_next_serial (BdkDisplay *display)
{
  return NextRequest (BDK_DISPLAY_XDISPLAY (display));
}


static BdkInternalConnection *
bdk_add_connection_handler (Display *display,
			    guint    fd)
{
  BUNNYIOChannel *io_channel;
  BdkInternalConnection *connection;

  connection = g_new (BdkInternalConnection, 1);

  connection->fd = fd;
  connection->display = display;
  
  io_channel = g_io_channel_unix_new (fd);
  
  connection->source = g_io_create_watch (io_channel, G_IO_IN);
  g_source_set_callback (connection->source,
			 (GSourceFunc)process_internal_connection, connection, NULL);
  g_source_attach (connection->source, NULL);
  
  g_io_channel_unref (io_channel);
  
  return connection;
}

static void
bdk_remove_connection_handler (BdkInternalConnection *connection)
{
  g_source_destroy (connection->source);
  g_free (connection);
}

static void
bdk_internal_connection_watch (Display  *display,
			       XPointer  arg,
			       gint      fd,
			       gboolean  opening,
			       XPointer *watch_data)
{
  if (opening)
    *watch_data = (XPointer)bdk_add_connection_handler (display, fd);
  else
    bdk_remove_connection_handler ((BdkInternalConnection *)*watch_data);
}
#endif /* HAVE_X11R6 */

/**
 * bdk_display_get_name:
 * @display: a #BdkDisplay
 *
 * Gets the name of the display.
 * 
 * Returns: a string representing the display name. This string is owned
 * by BDK and should not be modified or freed.
 * 
 * Since: 2.2
 */
const gchar *
bdk_display_get_name (BdkDisplay *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);
  
  return (gchar *) DisplayString (BDK_DISPLAY_X11 (display)->xdisplay);
}

/**
 * bdk_display_get_n_screens:
 * @display: a #BdkDisplay
 *
 * Gets the number of screen managed by the @display.
 * 
 * Returns: number of screens.
 * 
 * Since: 2.2
 */
gint
bdk_display_get_n_screens (BdkDisplay *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), 0);
  
  return ScreenCount (BDK_DISPLAY_X11 (display)->xdisplay);
}

/**
 * bdk_display_get_screen:
 * @display: a #BdkDisplay
 * @screen_num: the screen number
 *
 * Returns a screen object for one of the screens of the display.
 *
 * Returns: the #BdkScreen object
 *
 * Since: 2.2
 */
BdkScreen *
bdk_display_get_screen (BdkDisplay *display, 
			gint        screen_num)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (ScreenCount (BDK_DISPLAY_X11 (display)->xdisplay) > screen_num, NULL);
  
  return BDK_DISPLAY_X11 (display)->screens[screen_num];
}

/**
 * bdk_display_get_default_screen:
 * @display: a #BdkDisplay
 *
 * Get the default #BdkScreen for @display.
 * 
 * Returns: the default #BdkScreen object for @display
 *
 * Since: 2.2
 */
BdkScreen *
bdk_display_get_default_screen (BdkDisplay *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);
  
  return BDK_DISPLAY_X11 (display)->default_screen;
}

gboolean
_bdk_x11_display_is_root_window (BdkDisplay *display,
				 Window      xroot_window)
{
  BdkDisplayX11 *display_x11;
  gint i;
  
  g_return_val_if_fail (BDK_IS_DISPLAY (display), FALSE);
  
  display_x11 = BDK_DISPLAY_X11 (display);
  
  for (i = 0; i < ScreenCount (display_x11->xdisplay); i++)
    {
      if (BDK_SCREEN_XROOTWIN (display_x11->screens[i]) == xroot_window)
	return TRUE;
    }
  return FALSE;
}

struct XPointerUngrabInfo {
  BdkDisplay *display;
  guint32 time;
};

static void
pointer_ungrab_callback (BdkDisplay *display,
			 gpointer data,
			 gulong serial)
{
  _bdk_display_pointer_grab_update (display, serial);
}


#define XSERVER_TIME_IS_LATER(time1, time2)                        \
  ( (( time1 > time2 ) && ( time1 - time2 < ((guint32)-1)/2 )) ||  \
    (( time1 < time2 ) && ( time2 - time1 > ((guint32)-1)/2 ))     \
  )

/**
 * bdk_display_pointer_ungrab:
 * @display: a #BdkDisplay.
 * @time_: a timestap (e.g. %BDK_CURRENT_TIME).
 *
 * Release any pointer grab.
 *
 * Since: 2.2
 */
void
bdk_display_pointer_ungrab (BdkDisplay *display,
			    guint32     time_)
{
  Display *xdisplay;
  BdkDisplayX11 *display_x11;
  BdkPointerGrabInfo *grab;
  unsigned long serial;

  g_return_if_fail (BDK_IS_DISPLAY (display));

  display_x11 = BDK_DISPLAY_X11 (display);
  xdisplay = BDK_DISPLAY_XDISPLAY (display);

  serial = NextRequest (xdisplay);
  
  _bdk_input_ungrab_pointer (display, time_);
  XUngrabPointer (xdisplay, time_);
  XFlush (xdisplay);

  grab = _bdk_display_get_last_pointer_grab (display);
  if (grab &&
      (time_ == BDK_CURRENT_TIME ||
       grab->time == BDK_CURRENT_TIME ||
       !XSERVER_TIME_IS_LATER (grab->time, time_)))
    {
      grab->serial_end = serial;
      _bdk_x11_roundtrip_async (display, 
				pointer_ungrab_callback,
				NULL);
    }
}

/**
 * bdk_display_keyboard_ungrab:
 * @display: a #BdkDisplay.
 * @time_: a timestap (e.g #BDK_CURRENT_TIME).
 *
 * Release any keyboard grab
 *
 * Since: 2.2
 */
void
bdk_display_keyboard_ungrab (BdkDisplay *display,
			     guint32     time)
{
  Display *xdisplay;
  BdkDisplayX11 *display_x11;
  
  g_return_if_fail (BDK_IS_DISPLAY (display));

  display_x11 = BDK_DISPLAY_X11 (display);
  xdisplay = BDK_DISPLAY_XDISPLAY (display);
  
  XUngrabKeyboard (xdisplay, time);
  XFlush (xdisplay);
  
  if (time == BDK_CURRENT_TIME || 
      display->keyboard_grab.time == BDK_CURRENT_TIME ||
      !XSERVER_TIME_IS_LATER (display->keyboard_grab.time, time))
    _bdk_display_unset_has_keyboard_grab (display, FALSE);
}

/**
 * bdk_display_beep:
 * @display: a #BdkDisplay
 *
 * Emits a short beep on @display
 *
 * Since: 2.2
 */
void
bdk_display_beep (BdkDisplay *display)
{
  g_return_if_fail (BDK_IS_DISPLAY (display));

#ifdef HAVE_XKB
  XkbBell (BDK_DISPLAY_XDISPLAY (display), None, 0, None);
#else
  XBell (BDK_DISPLAY_XDISPLAY (display), 0);
#endif
}

/**
 * bdk_display_sync:
 * @display: a #BdkDisplay
 *
 * Flushes any requests queued for the windowing system and waits until all
 * requests have been handled. This is often used for making sure that the
 * display is synchronized with the current state of the program. Calling
 * bdk_display_sync() before bdk_error_trap_pop() makes sure that any errors
 * generated from earlier requests are handled before the error trap is 
 * removed.
 *
 * This is most useful for X11. On windowing systems where requests are
 * handled synchronously, this function will do nothing.
 *
 * Since: 2.2
 */
void
bdk_display_sync (BdkDisplay *display)
{
  g_return_if_fail (BDK_IS_DISPLAY (display));
  
  XSync (BDK_DISPLAY_XDISPLAY (display), False);
}

/**
 * bdk_display_flush:
 * @display: a #BdkDisplay
 *
 * Flushes any requests queued for the windowing system; this happens automatically
 * when the main loop blocks waiting for new events, but if your application
 * is drawing without returning control to the main loop, you may need
 * to call this function explicitely. A common case where this function
 * needs to be called is when an application is executing drawing commands
 * from a thread other than the thread where the main loop is running.
 *
 * This is most useful for X11. On windowing systems where requests are
 * handled synchronously, this function will do nothing.
 *
 * Since: 2.4
 */
void 
bdk_display_flush (BdkDisplay *display)
{
  g_return_if_fail (BDK_IS_DISPLAY (display));

  if (!display->closed)
    XFlush (BDK_DISPLAY_XDISPLAY (display));
}

/**
 * bdk_display_get_default_group:
 * @display: a #BdkDisplay
 * 
 * Returns the default group leader window for all toplevel windows
 * on @display. This window is implicitly created by BDK. 
 * See bdk_window_set_group().
 * 
 * Return value: The default group leader window for @display
 *
 * Since: 2.4
 **/
BdkWindow *
bdk_display_get_default_group (BdkDisplay *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);

  return BDK_DISPLAY_X11 (display)->leader_bdk_window;
}

/**
 * bdk_x11_display_grab:
 * @display: a #BdkDisplay 
 * 
 * Call XGrabServer() on @display. 
 * To ungrab the display again, use bdk_x11_display_ungrab(). 
 *
 * bdk_x11_display_grab()/bdk_x11_display_ungrab() calls can be nested.
 *
 * Since: 2.2
 **/
void
bdk_x11_display_grab (BdkDisplay *display)
{
  BdkDisplayX11 *display_x11;
  
  g_return_if_fail (BDK_IS_DISPLAY (display));
  
  display_x11 = BDK_DISPLAY_X11 (display);
  
  if (display_x11->grab_count == 0)
    XGrabServer (display_x11->xdisplay);
  display_x11->grab_count++;
}

/**
 * bdk_x11_display_ungrab:
 * @display: a #BdkDisplay
 * 
 * Ungrab @display after it has been grabbed with 
 * bdk_x11_display_grab(). 
 *
 * Since: 2.2
 **/
void
bdk_x11_display_ungrab (BdkDisplay *display)
{
  BdkDisplayX11 *display_x11;
  
  g_return_if_fail (BDK_IS_DISPLAY (display));
  
  display_x11 = BDK_DISPLAY_X11 (display);;
  g_return_if_fail (display_x11->grab_count > 0);
  
  display_x11->grab_count--;
  if (display_x11->grab_count == 0)
    {
      XUngrabServer (display_x11->xdisplay);
      XFlush (display_x11->xdisplay);
    }
}

static void
bdk_display_x11_dispose (BObject *object)
{
  BdkDisplayX11 *display_x11 = BDK_DISPLAY_X11 (object);
  gint           i;

  g_list_foreach (display_x11->input_devices, (GFunc) g_object_run_dispose, NULL);

  for (i = 0; i < ScreenCount (display_x11->xdisplay); i++)
    _bdk_screen_close (display_x11->screens[i]);

  _bdk_events_uninit (BDK_DISPLAY_OBJECT (object));

  B_OBJECT_CLASS (_bdk_display_x11_parent_class)->dispose (object);
}

static void
bdk_display_x11_finalize (BObject *object)
{
  BdkDisplayX11 *display_x11 = BDK_DISPLAY_X11 (object);
  gint           i;

  /* Keymap */
  if (display_x11->keymap)
    g_object_unref (display_x11->keymap);

  /* Free motif Dnd */
  if (display_x11->motif_target_lists)
    {
      for (i = 0; i < display_x11->motif_n_target_lists; i++)
        g_list_free (display_x11->motif_target_lists[i]);
      g_free (display_x11->motif_target_lists);
    }

  _bdk_x11_cursor_display_finalize (BDK_DISPLAY_OBJECT(display_x11));

  /* Atom Hashtable */
  g_hash_table_destroy (display_x11->atom_from_virtual);
  g_hash_table_destroy (display_x11->atom_to_virtual);

  /* Leader Window */
  XDestroyWindow (display_x11->xdisplay, display_x11->leader_window);

  /* list of filters for client messages */
  g_list_foreach (display_x11->client_filters, (GFunc) g_free, NULL);
  g_list_free (display_x11->client_filters);

  /* List of event window extraction functions */
  b_slist_foreach (display_x11->event_types, (GFunc)g_free, NULL);
  b_slist_free (display_x11->event_types);

  /* input BdkDevice list */
  g_list_foreach (display_x11->input_devices, (GFunc) g_object_unref, NULL);
  g_list_free (display_x11->input_devices);

  /* input BdkWindow list */
  g_list_foreach (display_x11->input_windows, (GFunc) g_free, NULL);
  g_list_free (display_x11->input_windows);

  /* Free all BdkScreens */
  for (i = 0; i < ScreenCount (display_x11->xdisplay); i++)
    g_object_unref (display_x11->screens[i]);
  g_free (display_x11->screens);

  g_free (display_x11->startup_notification_id);

  /* X ID hashtable */
  g_hash_table_destroy (display_x11->xid_ht);

  XCloseDisplay (display_x11->xdisplay);

  B_OBJECT_CLASS (_bdk_display_x11_parent_class)->finalize (object);
}

/**
 * bdk_x11_lookup_xdisplay:
 * @xdisplay: a pointer to an X Display
 * 
 * Find the #BdkDisplay corresponding to @display, if any exists.
 * 
 * Return value: the #BdkDisplay, if found, otherwise %NULL.
 *
 * Since: 2.2
 **/
BdkDisplay *
bdk_x11_lookup_xdisplay (Display *xdisplay)
{
  GSList *tmp_list;

  for (tmp_list = _bdk_displays; tmp_list; tmp_list = tmp_list->next)
    {
      if (BDK_DISPLAY_XDISPLAY (tmp_list->data) == xdisplay)
	return tmp_list->data;
    }
  
  return NULL;
}

/**
 * _bdk_x11_display_screen_for_xrootwin:
 * @display: a #BdkDisplay
 * @xrootwin: window ID for one of of the screen's of the display.
 * 
 * Given the root window ID of one of the screen's of a #BdkDisplay,
 * finds the screen.
 * 
 * Return value: (transfer none): the #BdkScreen corresponding to
 *     @xrootwin, or %NULL.
 **/
BdkScreen *
_bdk_x11_display_screen_for_xrootwin (BdkDisplay *display,
				      Window      xrootwin)
{
  gint i;

  for (i = 0; i < ScreenCount (BDK_DISPLAY_X11 (display)->xdisplay); i++)
    {
      BdkScreen *screen = bdk_display_get_screen (display, i);
      if (BDK_SCREEN_XROOTWIN (screen) == xrootwin)
	return screen;
    }

  return NULL;
}

/**
 * bdk_x11_display_get_xdisplay:
 * @display: a #BdkDisplay
 * @returns: an X display.
 *
 * Returns the X display of a #BdkDisplay.
 *
 * Since: 2.2
 */
Display *
bdk_x11_display_get_xdisplay (BdkDisplay *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);
  return BDK_DISPLAY_X11 (display)->xdisplay;
}

void
_bdk_windowing_set_default_display (BdkDisplay *display)
{
  BdkDisplayX11 *display_x11 = BDK_DISPLAY_X11 (display);
  const gchar *startup_id;
  
  if (!display)
    {
      bdk_display = NULL;
      return;
    }

  bdk_display = BDK_DISPLAY_XDISPLAY (display);

  g_free (display_x11->startup_notification_id);
  display_x11->startup_notification_id = NULL;
  
  startup_id = g_getenv ("DESKTOP_STARTUP_ID");
  if (startup_id && *startup_id != '\0')
    {
      gchar *time_str;

      if (!g_utf8_validate (startup_id, -1, NULL))
	g_warning ("DESKTOP_STARTUP_ID contains invalid UTF-8");
      else
	display_x11->startup_notification_id = g_strdup (startup_id);

      /* Find the launch time from the startup_id, if it's there.  Newer spec
       * states that the startup_id is of the form <unique>_TIME<timestamp>
       */
      time_str = g_strrstr (startup_id, "_TIME");
      if (time_str != NULL)
        {
	  gulong retval;
          gchar *end;
          errno = 0;

          /* Skip past the "_TIME" part */
          time_str += 5;

          retval = strtoul (time_str, &end, 0);
          if (end != time_str && errno == 0)
            display_x11->user_time = retval;
        }
      
      /* Clear the environment variable so it won't be inherited by
       * child processes and confuse things.  
       */
      g_unsetenv ("DESKTOP_STARTUP_ID");

      /* Set the startup id on the leader window so it
       * applies to all windows we create on this display
       */
      XChangeProperty (display_x11->xdisplay,
		       display_x11->leader_window,
		       bdk_x11_get_xatom_by_name_for_display (display, "_NET_STARTUP_ID"),
		       bdk_x11_get_xatom_by_name_for_display (display, "UTF8_STRING"), 8,
		       PropModeReplace,
		       (guchar *)startup_id, strlen (startup_id));
    }
}

static void
broadcast_xmessage (BdkDisplay *display,
		    const char *message_type,
		    const char *message_type_begin,
		    const char *message)
{
  Display *xdisplay = BDK_DISPLAY_XDISPLAY (display);
  BdkScreen *screen = bdk_display_get_default_screen (display);
  BdkWindow *root_window = bdk_screen_get_root_window (screen);
  Window xroot_window = BDK_WINDOW_XID (root_window);
  
  Atom type_atom;
  Atom type_atom_begin;
  Window xwindow;

  if (!B_LIKELY (BDK_DISPLAY_X11 (display)->trusted_client))
    return;

  {
    XSetWindowAttributes attrs;

    attrs.override_redirect = True;
    attrs.event_mask = PropertyChangeMask | StructureNotifyMask;

    xwindow =
      XCreateWindow (xdisplay,
                     xroot_window,
                     -100, -100, 1, 1,
                     0,
                     CopyFromParent,
                     CopyFromParent,
                     (Visual *)CopyFromParent,
                     CWOverrideRedirect | CWEventMask,
                     &attrs);
  }

  type_atom = bdk_x11_get_xatom_by_name_for_display (display,
                                                     message_type);
  type_atom_begin = bdk_x11_get_xatom_by_name_for_display (display,
                                                           message_type_begin);
  
  {
    XClientMessageEvent xclient;
    const char *src;
    const char *src_end;
    char *dest;
    char *dest_end;
    
		memset(&xclient, 0, sizeof (xclient));
    xclient.type = ClientMessage;
    xclient.message_type = type_atom_begin;
    xclient.display =xdisplay;
    xclient.window = xwindow;
    xclient.format = 8;

    src = message;
    src_end = message + strlen (message) + 1; /* +1 to include nul byte */
    
    while (src != src_end)
      {
        dest = &xclient.data.b[0];
        dest_end = dest + 20;        
        
        while (dest != dest_end &&
               src != src_end)
          {
            *dest = *src;
            ++dest;
            ++src;
          }

	while (dest != dest_end)
	  {
	    *dest = 0;
	    ++dest;
	  }
        
        XSendEvent (xdisplay,
                    xroot_window,
                    False,
                    PropertyChangeMask,
                    (XEvent *)&xclient);

        xclient.message_type = type_atom;
      }
  }

  XDestroyWindow (xdisplay, xwindow);
  XFlush (xdisplay);
}

/**
 * bdk_x11_display_broadcast_startup_message:
 * @display: a #BdkDisplay
 * @message_type: startup notification message type ("new", "change",
 * or "remove")
 * @...: a list of key/value pairs (as strings), terminated by a
 * %NULL key. (A %NULL value for a key will cause that key to be
 * skipped in the output.)
 *
 * Sends a startup notification message of type @message_type to
 * @display. 
 *
 * This is a convenience function for use by code that implements the
 * freedesktop startup notification specification. Applications should
 * not normally need to call it directly. See the <ulink
 * url="http://standards.freedesktop.org/startup-notification-spec/startup-notification-latest.txt">Startup
 * Notification Protocol specification</ulink> for
 * definitions of the message types and keys that can be used.
 *
 * Since: 2.12
 **/
void
bdk_x11_display_broadcast_startup_message (BdkDisplay *display,
					   const char *message_type,
					   ...)
{
  GString *message;
  va_list ap;
  const char *key, *value, *p;

  message = g_string_new (message_type);
  g_string_append_c (message, ':');

  va_start (ap, message_type);
  while ((key = va_arg (ap, const char *)))
    {
      value = va_arg (ap, const char *);
      if (!value)
	continue;

      g_string_append_printf (message, " %s=\"", key);
      for (p = value; *p; p++)
	{
	  switch (*p)
	    {
	    case ' ':
	    case '"':
	    case '\\':
	      g_string_append_c (message, '\\');
	      break;
	    }

	  g_string_append_c (message, *p);
	}
      g_string_append_c (message, '\"');
    }
  va_end (ap);

  broadcast_xmessage (display,
		      "_NET_STARTUP_INFO",
                      "_NET_STARTUP_INFO_BEGIN",
                      message->str);

  g_string_free (message, TRUE);
}

/**
 * bdk_notify_startup_complete:
 * 
 * Indicates to the GUI environment that the application has finished
 * loading. If the applications opens windows, this function is
 * normally called after opening the application's initial set of
 * windows.
 * 
 * BTK+ will call this function automatically after opening the first
 * #BtkWindow unless btk_window_set_auto_startup_notification() is called 
 * to disable that feature.
 *
 * Since: 2.2
 **/
void
bdk_notify_startup_complete (void)
{
  BdkDisplay *display;
  BdkDisplayX11 *display_x11;

  display = bdk_display_get_default ();
  if (!display)
    return;
  
  display_x11 = BDK_DISPLAY_X11 (display);

  if (display_x11->startup_notification_id == NULL)
    return;

  bdk_notify_startup_complete_with_id (display_x11->startup_notification_id);
}

/**
 * bdk_notify_startup_complete_with_id:
 * @startup_id: a startup-notification identifier, for which notification
 *              process should be completed
 * 
 * Indicates to the GUI environment that the application has finished
 * loading, using a given identifier.
 * 
 * BTK+ will call this function automatically for #BtkWindow with custom
 * startup-notification identifier unless
 * btk_window_set_auto_startup_notification() is called to disable
 * that feature.
 *
 * Since: 2.12
 **/
void
bdk_notify_startup_complete_with_id (const gchar* startup_id)
{
  BdkDisplay *display;

  display = bdk_display_get_default ();
  if (!display)
    return;

  bdk_x11_display_broadcast_startup_message (display, "remove",
					     "ID", startup_id,
					     NULL);
}

/**
 * bdk_display_supports_selection_notification:
 * @display: a #BdkDisplay
 * 
 * Returns whether #BdkEventOwnerChange events will be 
 * sent when the owner of a selection changes.
 * 
 * Return value: whether #BdkEventOwnerChange events will 
 *               be sent.
 *
 * Since: 2.6
 **/
gboolean 
bdk_display_supports_selection_notification (BdkDisplay *display)
{
  BdkDisplayX11 *display_x11 = BDK_DISPLAY_X11 (display);

  return display_x11->have_xfixes;
}

/**
 * bdk_display_request_selection_notification:
 * @display: a #BdkDisplay
 * @selection: the #BdkAtom naming the selection for which
 *             ownership change notification is requested
 * 
 * Request #BdkEventOwnerChange events for ownership changes
 * of the selection named by the given atom.
 * 
 * Return value: whether #BdkEventOwnerChange events will 
 *               be sent.
 *
 * Since: 2.6
 **/
gboolean
bdk_display_request_selection_notification (BdkDisplay *display,
					    BdkAtom     selection)

{
#ifdef HAVE_XFIXES
  BdkDisplayX11 *display_x11 = BDK_DISPLAY_X11 (display);
  Atom atom;

  if (display_x11->have_xfixes)
    {
      atom = bdk_x11_atom_to_xatom_for_display (display, 
						selection);
      XFixesSelectSelectionInput (display_x11->xdisplay, 
				  display_x11->leader_window,
				  atom,
				  XFixesSetSelectionOwnerNotifyMask |
				  XFixesSelectionWindowDestroyNotifyMask |
				  XFixesSelectionClientCloseNotifyMask);
      return TRUE;
    }
  else
#endif
    return FALSE;
}

/**
 * bdk_display_supports_clipboard_persistence
 * @display: a #BdkDisplay
 *
 * Returns whether the speicifed display supports clipboard
 * persistance; i.e. if it's possible to store the clipboard data after an
 * application has quit. On X11 this checks if a clipboard daemon is
 * running.
 *
 * Returns: %TRUE if the display supports clipboard persistance.
 *
 * Since: 2.6
 */
gboolean
bdk_display_supports_clipboard_persistence (BdkDisplay *display)
{
  Atom clipboard_manager;

  /* It might make sense to cache this */
  clipboard_manager = bdk_x11_get_xatom_by_name_for_display (display, "CLIPBOARD_MANAGER");
  return XGetSelectionOwner (BDK_DISPLAY_X11 (display)->xdisplay, clipboard_manager) != None;
}

/**
 * bdk_display_store_clipboard
 * @display:          a #BdkDisplay
 * @clipboard_window: a #BdkWindow belonging to the clipboard owner
 * @time_:            a timestamp
 * @targets:	      an array of targets that should be saved, or %NULL 
 *                    if all available targets should be saved.
 * @n_targets:        length of the @targets array
 *
 * Issues a request to the clipboard manager to store the
 * clipboard data. On X11, this is a special program that works
 * according to the freedesktop clipboard specification, available at
 * <ulink url="http://www.freedesktop.org/Standards/clipboard-manager-spec">
 * http://www.freedesktop.org/Standards/clipboard-manager-spec</ulink>.
 *
 * Since: 2.6
 */
void
bdk_display_store_clipboard (BdkDisplay    *display,
			     BdkWindow     *clipboard_window,
			     guint32        time_,
			     const BdkAtom *targets,
			     gint           n_targets)
{
  BdkDisplayX11 *display_x11 = BDK_DISPLAY_X11 (display);
  Atom clipboard_manager, save_targets;

  g_return_if_fail (BDK_WINDOW_IS_X11 (clipboard_window));

  clipboard_manager = bdk_x11_get_xatom_by_name_for_display (display, "CLIPBOARD_MANAGER");
  save_targets = bdk_x11_get_xatom_by_name_for_display (display, "SAVE_TARGETS");

  bdk_error_trap_push ();

  if (XGetSelectionOwner (display_x11->xdisplay, clipboard_manager) != None)
    {
      Atom property_name = None;
      Atom *xatoms;
      int i;
      
      if (n_targets > 0)
	{
	  property_name = bdk_x11_atom_to_xatom_for_display (display, _bdk_selection_property);

	  xatoms = g_new (Atom, n_targets);
	  for (i = 0; i < n_targets; i++)
	    xatoms[i] = bdk_x11_atom_to_xatom_for_display (display, targets[i]);

	  XChangeProperty (display_x11->xdisplay, BDK_WINDOW_XID (clipboard_window),
			   property_name, XA_ATOM,
			   32, PropModeReplace, (guchar *)xatoms, n_targets);
	  g_free (xatoms);

	}
      
      XConvertSelection (display_x11->xdisplay,
			 clipboard_manager, save_targets, property_name,
			 BDK_WINDOW_XID (clipboard_window), time_);
      
    }
  bdk_error_trap_pop ();

}

/**
 * bdk_x11_display_get_user_time:
 * @display: a #BdkDisplay
 *
 * Returns the timestamp of the last user interaction on 
 * @display. The timestamp is taken from events caused
 * by user interaction such as key presses or pointer 
 * movements. See bdk_x11_window_set_user_time().
 *
 * Returns: the timestamp of the last user interaction 
 *
 * Since: 2.8
 */
guint32
bdk_x11_display_get_user_time (BdkDisplay *display)
{
  return BDK_DISPLAY_X11 (display)->user_time;
}

/**
 * bdk_display_supports_shapes:
 * @display: a #BdkDisplay
 *
 * Returns %TRUE if bdk_window_shape_combine_mask() can
 * be used to create shaped windows on @display.
 *
 * Returns: %TRUE if shaped windows are supported 
 *
 * Since: 2.10
 */
gboolean 
bdk_display_supports_shapes (BdkDisplay *display)
{
  return BDK_DISPLAY_X11 (display)->have_shapes;
}

/**
 * bdk_display_supports_input_shapes:
 * @display: a #BdkDisplay
 *
 * Returns %TRUE if bdk_window_input_shape_combine_mask() can
 * be used to modify the input shape of windows on @display.
 *
 * Returns: %TRUE if windows with modified input shape are supported 
 *
 * Since: 2.10
 */
gboolean 
bdk_display_supports_input_shapes (BdkDisplay *display)
{
  return BDK_DISPLAY_X11 (display)->have_input_shapes;
}


/**
 * bdk_x11_display_get_startup_notification_id:
 * @display: a #BdkDisplay
 *
 * Gets the startup notification ID for a display.
 * 
 * Returns: the startup notification ID for @display
 *
 * Since: 2.12
 */
const gchar *
bdk_x11_display_get_startup_notification_id (BdkDisplay *display)
{
  return BDK_DISPLAY_X11 (display)->startup_notification_id;
}

/**
 * bdk_display_supports_composite:
 * @display: a #BdkDisplay
 *
 * Returns %TRUE if bdk_window_set_composited() can be used
 * to redirect drawing on the window using compositing.
 *
 * Currently this only works on X11 with XComposite and
 * XDamage extensions available.
 *
 * Returns: %TRUE if windows may be composited.
 *
 * Since: 2.12
 */
gboolean
bdk_display_supports_composite (BdkDisplay *display)
{
  BdkDisplayX11 *x11_display = BDK_DISPLAY_X11 (display);

  return x11_display->have_xcomposite &&
	 x11_display->have_xdamage &&
	 x11_display->have_xfixes;
}


#define __BDK_DISPLAY_X11_C__
#include "bdkaliasdef.c"
