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

#include <bunnylib/gprintf.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef HAVE_XKB
#include <X11/XKBlib.h>
#endif

#include "bdk.h"

#include "bdkx.h"
#include "bdkasync.h"
#include "bdkdisplay-x11.h"
#include "bdkinternals.h"
#include "bdkintl.h"
#include "bdkrebunnyion-generic.h"
#include "bdkinputprivate.h"
#include "bdkalias.h"

typedef struct _BdkPredicate  BdkPredicate;
typedef struct _BdkErrorTrap  BdkErrorTrap;

struct _BdkPredicate
{
  BdkEventFunc func;
  gpointer data;
};

struct _BdkErrorTrap
{
  int (*old_handler) (Display *, XErrorEvent *);
  gint error_warnings;
  gint error_code;
};

/* 
 * Private function declarations
 */

#ifndef HAVE_XCONVERTCASE
static void	 bdkx_XConvertCase	(KeySym	       symbol,
					 KeySym	      *lower,
					 KeySym	      *upper);
#define XConvertCase bdkx_XConvertCase
#endif

static int	    bdk_x_error			 (Display     *display, 
						  XErrorEvent *error);
static int	    bdk_x_io_error		 (Display     *display);

/* Private variable declarations
 */
static GSList *bdk_error_traps = NULL;               /* List of error traps */
static GSList *bdk_error_trap_free_list = NULL;      /* Free list */

const GOptionEntry _bdk_windowing_args[] = {
  { "sync", 0, 0, G_OPTION_ARG_NONE, &_bdk_synchronize, 
    /* Description of --sync in --help output */ N_("Make X calls synchronous"), NULL },
  { NULL }
};

void
_bdk_windowing_init (void)
{
  _bdk_x11_initialize_locale ();
  
  XSetErrorHandler (bdk_x_error);
  XSetIOErrorHandler (bdk_x_io_error);

  _bdk_selection_property = bdk_atom_intern_static_string ("BDK_SELECTION");
}

void
bdk_set_use_xshm (gboolean use_xshm)
{
}

gboolean
bdk_get_use_xshm (void)
{
  return BDK_DISPLAY_X11 (bdk_display_get_default ())->use_xshm;
}

static BdkGrabStatus
bdk_x11_convert_grab_status (gint status)
{
  switch (status)
    {
    case GrabSuccess:
      return BDK_GRAB_SUCCESS;
    case AlreadyGrabbed:
      return BDK_GRAB_ALREADY_GRABBED;
    case GrabInvalidTime:
      return BDK_GRAB_INVALID_TIME;
    case GrabNotViewable:
      return BDK_GRAB_NOT_VIEWABLE;
    case GrabFrozen:
      return BDK_GRAB_FROZEN;
    }

  g_assert_not_reached();

  return 0;
}

static void
has_pointer_grab_callback (BdkDisplay *display,
			   gpointer data,
			   gulong serial)
{
  _bdk_display_pointer_grab_update (display, serial);
}

BdkGrabStatus
_bdk_windowing_pointer_grab (BdkWindow *window,
			     BdkWindow *native,
			     gboolean owner_events,
			     BdkEventMask event_mask,
			     BdkWindow *confine_to,
			     BdkCursor *cursor,
			     guint32 time)
{
  gint return_val;
  BdkCursorPrivate *cursor_private;
  BdkDisplayX11 *display_x11;
  guint xevent_mask;
  Window xwindow;
  Window xconfine_to;
  Cursor xcursor;
  int i;

  if (confine_to)
    confine_to = _bdk_window_get_impl_window (confine_to);

  display_x11 = BDK_DISPLAY_X11 (BDK_WINDOW_DISPLAY (native));

  cursor_private = (BdkCursorPrivate*) cursor;

  xwindow = BDK_WINDOW_XID (native);

  if (!confine_to || BDK_WINDOW_DESTROYED (confine_to))
    xconfine_to = None;
  else
    xconfine_to = BDK_WINDOW_XID (confine_to);

  if (!cursor)
    xcursor = None;
  else
    {
      _bdk_x11_cursor_update_theme (cursor);
      xcursor = cursor_private->xcursor;
    }

  xevent_mask = 0;
  for (i = 0; i < _bdk_nenvent_masks; i++)
    {
      if (event_mask & (1 << (i + 1)))
	xevent_mask |= _bdk_event_mask_table[i];
    }

  /* We don't want to set a native motion hint mask, as we're emulating motion
   * hints. If we set a native one we just wouldn't get any events.
   */
  xevent_mask &= ~PointerMotionHintMask;

  return_val = _bdk_input_grab_pointer (window,
					native,
					owner_events,
					event_mask,
					confine_to,
					time);

  if (return_val == GrabSuccess ||
      B_UNLIKELY (!display_x11->trusted_client && return_val == AlreadyGrabbed))
    {
      if (!BDK_WINDOW_DESTROYED (native))
	{
#ifdef G_ENABLE_DEBUG
	  if (_bdk_debug_flags & BDK_DEBUG_NOGRABS)
	    return_val = GrabSuccess;
	  else
#endif
	    return_val = XGrabPointer (BDK_WINDOW_XDISPLAY (native),
				       xwindow,
				       owner_events,
				       xevent_mask,
				       GrabModeAsync, GrabModeAsync,
				       xconfine_to,
				       xcursor,
				       time);
	}
      else
	return_val = AlreadyGrabbed;
    }

  if (return_val == GrabSuccess)
    _bdk_x11_roundtrip_async (BDK_DISPLAY_OBJECT (display_x11),
			      has_pointer_grab_callback,
			      NULL);

  return bdk_x11_convert_grab_status (return_val);
}

/*
 *--------------------------------------------------------------
 * bdk_keyboard_grab
 *
 *   Grabs the keyboard to a specific window
 *
 * Arguments:
 *   "window" is the window which will receive the grab
 *   "owner_events" specifies whether events will be reported as is,
 *     or relative to "window"
 *   "time" specifies the time
 *
 * Results:
 *
 * Side effects:
 *   requires a corresponding call to bdk_keyboard_ungrab
 *
 *--------------------------------------------------------------
 */

BdkGrabStatus
bdk_keyboard_grab (BdkWindow *	   window,
		   gboolean	   owner_events,
		   guint32	   time)
{
  gint return_val;
  unsigned long serial;
  BdkDisplay *display;
  BdkDisplayX11 *display_x11;
  BdkWindow *native;

  g_return_val_if_fail (window != NULL, 0);
  g_return_val_if_fail (BDK_IS_WINDOW (window), 0);

  native = bdk_window_get_toplevel (window);

  /* TODO: What do we do for offscreens and  children? We need to proxy the grab somehow */
  if (!BDK_IS_WINDOW_IMPL_X11 (BDK_WINDOW_OBJECT (native)->impl))
    return BDK_GRAB_SUCCESS;

  display = BDK_WINDOW_DISPLAY (native);
  display_x11 = BDK_DISPLAY_X11 (display);

  serial = NextRequest (BDK_WINDOW_XDISPLAY (native));

  if (!BDK_WINDOW_DESTROYED (native))
    {
#ifdef G_ENABLE_DEBUG
      if (_bdk_debug_flags & BDK_DEBUG_NOGRABS)
	return_val = GrabSuccess;
      else
#endif
	return_val = XGrabKeyboard (BDK_WINDOW_XDISPLAY (native),
				    BDK_WINDOW_XID (native),
				    owner_events,
				    GrabModeAsync, GrabModeAsync,
				    time);
	if (B_UNLIKELY (!display_x11->trusted_client && 
			return_val == AlreadyGrabbed))
	  /* we can't grab the keyboard, but we can do a BTK-local grab */
	  return_val = GrabSuccess;
    }
  else
    return_val = AlreadyGrabbed;

  if (return_val == GrabSuccess)
    _bdk_display_set_has_keyboard_grab (display,
					window,	native,
					owner_events,
					serial, time);

  return bdk_x11_convert_grab_status (return_val);
}

/**
 * _bdk_xgrab_check_unmap:
 * @window: a #BdkWindow
 * @serial: serial from Unmap event (or from NextRequest(display)
 *   if the unmap is being done by this client.)
 * 
 * Checks to see if an unmap request or event causes the current
 * grab window to become not viewable, and if so, clear the
 * the pointer we keep to it.
 **/
void
_bdk_xgrab_check_unmap (BdkWindow *window,
			gulong     serial)
{
  BdkDisplay *display = bdk_drawable_get_display (window);

  _bdk_display_end_pointer_grab (display, serial, window, TRUE);

  if (display->keyboard_grab.window &&
      serial >= display->keyboard_grab.serial)
    {
      BdkWindowObject *private = BDK_WINDOW_OBJECT (window);
      BdkWindowObject *tmp = BDK_WINDOW_OBJECT (display->keyboard_grab.window);

      while (tmp && tmp != private)
	tmp = tmp->parent;

      if (tmp)
	_bdk_display_unset_has_keyboard_grab (display, TRUE);
    }
}

/**
 * _bdk_xgrab_check_destroy:
 * @window: a #BdkWindow
 * 
 * Checks to see if window is the current grab window, and if
 * so, clear the current grab window.
 **/
void
_bdk_xgrab_check_destroy (BdkWindow *window)
{
  BdkDisplay *display = bdk_drawable_get_display (window);
  BdkPointerGrabInfo *grab;

  /* Make sure there is no lasting grab in this native
     window */
  grab = _bdk_display_get_last_pointer_grab (display);
  if (grab && grab->native_window == window)
    {
      /* We don't know the actual serial to end, but it
	 doesn't really matter as this only happens
	 after we get told of the destroy from the
	 server so we know its ended in the server,
	 just make sure its ended. */
      grab->serial_end = grab->serial_start;
      grab->implicit_ungrab = TRUE;
    }
  
  if (window == display->keyboard_grab.native_window &&
      display->keyboard_grab.window != NULL)
    _bdk_display_unset_has_keyboard_grab (display, TRUE);
}

void
_bdk_windowing_display_set_sm_client_id (BdkDisplay  *display,
					 const gchar *sm_client_id)
{
  BdkDisplayX11 *display_x11 = BDK_DISPLAY_X11 (display);

  if (display->closed)
    return;

  if (sm_client_id && strcmp (sm_client_id, ""))
    {
      XChangeProperty (display_x11->xdisplay, display_x11->leader_window,
		       bdk_x11_get_xatom_by_name_for_display (display, "SM_CLIENT_ID"),
		       XA_STRING, 8, PropModeReplace, (guchar *)sm_client_id,
		       strlen (sm_client_id));
    }
  else
    XDeleteProperty (display_x11->xdisplay, display_x11->leader_window,
		     bdk_x11_get_xatom_by_name_for_display (display, "SM_CLIENT_ID"));
}

/**
 * bdk_x11_set_sm_client_id:
 * @sm_client_id: the client id assigned by the session manager when the
 *    connection was opened, or %NULL to remove the property.
 *
 * Sets the <literal>SM_CLIENT_ID</literal> property on the application's leader window so that
 * the window manager can save the application's state using the X11R6 ICCCM
 * session management protocol.
 *
 * See the X Session Management Library documentation for more information on
 * session management and the Inter-Client Communication Conventions Manual
 *
 * Since: 2.24
 */
void
bdk_x11_set_sm_client_id (const gchar *sm_client_id)
{
  bdk_set_sm_client_id (sm_client_id);
}

/* Close all open displays
 */
void
_bdk_windowing_exit (void)
{
  GSList *tmp_list = _bdk_displays;
    
  while (tmp_list)
    {
      XCloseDisplay (BDK_DISPLAY_XDISPLAY (tmp_list->data));
      
      tmp_list = tmp_list->next;
  }
}

/*
 *--------------------------------------------------------------
 * bdk_x_error
 *
 *   The X error handling routine.
 *
 * Arguments:
 *   "display" is the X display the error orignated from.
 *   "error" is the XErrorEvent that we are handling.
 *
 * Results:
 *   Either we were expecting some sort of error to occur,
 *   in which case we set the "_bdk_error_code" flag, or this
 *   error was unexpected, in which case we will print an
 *   error message and exit. (Since trying to continue will
 *   most likely simply lead to more errors).
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

static int
bdk_x_error (Display	 *display,
	     XErrorEvent *error)
{
  if (error->error_code)
    {
      if (_bdk_error_warnings)
	{
	  gchar buf[64];
          gchar *msg;
          
	  XGetErrorText (display, error->error_code, buf, 63);

          msg =
            g_strdup_printf ("The program '%s' received an X Window System error.\n"
                             "This probably reflects a bug in the program.\n"
                             "The error was '%s'.\n"
                             "  (Details: serial %ld error_code %d request_code %d minor_code %d)\n"
                             "  (Note to programmers: normally, X errors are reported asynchronously;\n"
                             "   that is, you will receive the error a while after causing it.\n"
                             "   To debug your program, run it with the --sync command line\n"
                             "   option to change this behavior. You can then get a meaningful\n"
                             "   backtrace from your debugger if you break on the bdk_x_error() function.)",
                             g_get_prgname (),
                             buf,
                             error->serial, 
                             error->error_code, 
                             error->request_code,
                             error->minor_code);
          
#ifdef G_ENABLE_DEBUG	  
	  g_error ("%s", msg);
#else /* !G_ENABLE_DEBUG */
	  g_fprintf (stderr, "%s\n", msg);

	  exit (1);
#endif /* G_ENABLE_DEBUG */
	}
      _bdk_error_code = error->error_code;
    }
  
  return 0;
}

/*
 *--------------------------------------------------------------
 * bdk_x_io_error
 *
 *   The X I/O error handling routine.
 *
 * Arguments:
 *   "display" is the X display the error orignated from.
 *
 * Results:
 *   An X I/O error basically means we lost our connection
 *   to the X server. There is not much we can do to
 *   continue, so simply print an error message and exit.
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

static int
bdk_x_io_error (Display *display)
{
  /* This is basically modelled after the code in XLib. We need
   * an explicit error handler here, so we can disable our atexit()
   * which would otherwise cause a nice segfault.
   * We fprintf(stderr, instead of g_warning() because g_warning()
   * could possibly be redirected to a dialog
   */
  if (errno == EPIPE)
    {
      g_fprintf (stderr,
               "The application '%s' lost its connection to the display %s;\n"
               "most likely the X server was shut down or you killed/destroyed\n"
               "the application.\n",
               g_get_prgname (),
               display ? DisplayString (display) : bdk_get_display_arg_name ());
    }
  else
    {
      g_fprintf (stderr, "%s: Fatal IO error %d (%s) on X server %s.\n",
               g_get_prgname (),
	       errno, g_strerror (errno),
	       display ? DisplayString (display) : bdk_get_display_arg_name ());
    }

  exit(1);
}

/*************************************************************
 * bdk_error_trap_push:
 *     Push an error trap. X errors will be trapped until
 *     the corresponding bdk_error_pop(), which will return
 *     the error code, if any.
 *   arguments:
 *     
 *   results:
 *************************************************************/

void
bdk_error_trap_push (void)
{
  GSList *node;
  BdkErrorTrap *trap;

  if (bdk_error_trap_free_list)
    {
      node = bdk_error_trap_free_list;
      bdk_error_trap_free_list = bdk_error_trap_free_list->next;
    }
  else
    {
      node = g_slist_alloc ();
      node->data = g_new (BdkErrorTrap, 1);
    }

  node->next = bdk_error_traps;
  bdk_error_traps = node;
  
  trap = node->data;
  trap->old_handler = XSetErrorHandler (bdk_x_error);
  trap->error_code = _bdk_error_code;
  trap->error_warnings = _bdk_error_warnings;

  _bdk_error_code = 0;
  _bdk_error_warnings = 0;
}

/*************************************************************
 * bdk_error_trap_pop:
 *     Pop an error trap added with bdk_error_push()
 *   arguments:
 *     
 *   results:
 *     0, if no error occured, otherwise the error code.
 *************************************************************/

gint
bdk_error_trap_pop (void)
{
  GSList *node;
  BdkErrorTrap *trap;
  gint result;

  g_return_val_if_fail (bdk_error_traps != NULL, 0);

  node = bdk_error_traps;
  bdk_error_traps = bdk_error_traps->next;

  node->next = bdk_error_trap_free_list;
  bdk_error_trap_free_list = node;
  
  result = _bdk_error_code;
  
  trap = node->data;
  _bdk_error_code = trap->error_code;
  _bdk_error_warnings = trap->error_warnings;
  XSetErrorHandler (trap->old_handler);
  
  return result;
}

gchar *
bdk_get_display (void)
{
  return g_strdup (bdk_display_get_name (bdk_display_get_default ()));
}

/**
 * _bdk_send_xevent:
 * @display: #BdkDisplay which @window is on
 * @window: window ID to which to send the event
 * @propagate: %TRUE if the event should be propagated if the target window
 *             doesn't handle it.
 * @event_mask: event mask to match against, or 0 to send it to @window
 *              without regard to event masks.
 * @event_send: #XEvent to send
 * 
 * Send an event, like XSendEvent(), but trap errors and check
 * the result.
 * 
 * Return value: %TRUE if sending the event succeeded.
 **/
gint 
_bdk_send_xevent (BdkDisplay *display,
		  Window      window, 
		  gboolean    propagate, 
		  glong       event_mask,
		  XEvent     *event_send)
{
  gboolean result;

  if (display->closed)
    return FALSE;

  bdk_error_trap_push ();
  result = XSendEvent (BDK_DISPLAY_XDISPLAY (display), window, 
		       propagate, event_mask, event_send);
  XSync (BDK_DISPLAY_XDISPLAY (display), False);
  
  if (bdk_error_trap_pop ())
    return FALSE;
 
  return result;
}

void
_bdk_rebunnyion_get_xrectangles (const BdkRebunnyion *rebunnyion,
                             gint             x_offset,
                             gint             y_offset,
                             XRectangle     **rects,
                             gint            *n_rects)
{
  XRectangle *rectangles = g_new (XRectangle, rebunnyion->numRects);
  BdkRebunnyionBox *boxes = rebunnyion->rects;
  gint i;
  
  for (i = 0; i < rebunnyion->numRects; i++)
    {
      rectangles[i].x = CLAMP (boxes[i].x1 + x_offset, G_MINSHORT, G_MAXSHORT);
      rectangles[i].y = CLAMP (boxes[i].y1 + y_offset, G_MINSHORT, G_MAXSHORT);
      rectangles[i].width = CLAMP (boxes[i].x2 + x_offset, G_MINSHORT, G_MAXSHORT) - rectangles[i].x;
      rectangles[i].height = CLAMP (boxes[i].y2 + y_offset, G_MINSHORT, G_MAXSHORT) - rectangles[i].y;
    }

  *rects = rectangles;
  *n_rects = rebunnyion->numRects;
}

/**
 * bdk_x11_grab_server:
 * 
 * Call bdk_x11_display_grab() on the default display. 
 * To ungrab the server again, use bdk_x11_ungrab_server(). 
 *
 * bdk_x11_grab_server()/bdk_x11_ungrab_server() calls can be nested.
 **/ 
void
bdk_x11_grab_server (void)
{
  bdk_x11_display_grab (bdk_display_get_default ());
}

/**
 * bdk_x11_ungrab_server:
 *
 * Ungrab the default display after it has been grabbed with 
 * bdk_x11_grab_server(). 
 **/
void
bdk_x11_ungrab_server (void)
{
  bdk_x11_display_ungrab (bdk_display_get_default ());
}

/**
 * bdk_x11_get_default_screen:
 * 
 * Gets the default BTK+ screen number.
 * 
 * Return value: returns the screen number specified by
 *   the --display command line option or the DISPLAY environment
 *   variable when bdk_init() calls XOpenDisplay().
 **/
gint
bdk_x11_get_default_screen (void)
{
  return bdk_screen_get_number (bdk_screen_get_default ());
}

/**
 * bdk_x11_get_default_root_xwindow:
 * 
 * Gets the root window of the default screen 
 * (see bdk_x11_get_default_screen()).  
 * 
 * Return value: an Xlib <type>Window</type>.
 **/
Window
bdk_x11_get_default_root_xwindow (void)
{
  return BDK_SCREEN_XROOTWIN (bdk_screen_get_default ());
}

/**
 * bdk_x11_get_default_xdisplay:
 * 
 * Gets the default BTK+ display.
 * 
 * Return value: the Xlib <type>Display*</type> for the display
 * specified in the <option>--display</option> command line option 
 * or the <envar>DISPLAY</envar> environment variable.
 **/
Display *
bdk_x11_get_default_xdisplay (void)
{
  return BDK_DISPLAY_XDISPLAY (bdk_display_get_default ());
}

#define __BDK_MAIN_X11_C__
#include "bdkaliasdef.c"
