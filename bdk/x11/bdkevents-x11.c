/* BDK - The GIMP Drawing Kit
 * Copyright (C) 1995-2007 Peter Mattis, Spencer Kimball,
 * Josh MacDonald, Ryan Lortie
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
 * Modified by the BTK+ Team and others 1997-2007.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"

#include "bdk.h"
#include "bdkprivate-x11.h"
#include "bdkinternals.h"
#include "bdkx.h"
#include "bdkscreen-x11.h"
#include "bdkdisplay-x11.h"
#include "bdkasync.h"

#include "bdkkeysyms.h"

#include "xsettings-client.h"

#include <string.h>

#include "bdkinputprivate.h"
#include "bdksettings.c"
#include "bdkalias.h"


#ifdef HAVE_XKB
#include <X11/XKBlib.h>
#endif

#ifdef HAVE_XSYNC
#include <X11/extensions/sync.h>
#endif

#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif

#ifdef HAVE_RANDR
#include <X11/extensions/Xrandr.h>
#endif

#include <X11/Xatom.h>

typedef struct _BdkIOClosure BdkIOClosure;
typedef struct _BdkDisplaySource BdkDisplaySource;
typedef struct _BdkEventTypeX11 BdkEventTypeX11;

struct _BdkIOClosure
{
  BdkInputFunction function;
  BdkInputCondition condition;
  GDestroyNotify notify;
  gpointer data;
};

struct _BdkDisplaySource
{
  GSource source;
  
  BdkDisplay *display;
  GPollFD event_poll_fd;
};

struct _BdkEventTypeX11
{
  gint base;
  gint n_events;
};

/* 
 * Private function declarations
 */

static gint	 bdk_event_apply_filters (XEvent   *xevent,
					  BdkEvent *event,
					  BdkWindow *window);
static gboolean	 bdk_event_translate	 (BdkDisplay *display,
					  BdkEvent   *event, 
					  XEvent     *xevent,
					  gboolean    return_exposes);

static gboolean bdk_event_prepare  (GSource     *source,
				    gint        *timeout);
static gboolean bdk_event_check    (GSource     *source);
static gboolean bdk_event_dispatch (GSource     *source,
				    GSourceFunc  callback,
				    gpointer     user_data);

static BdkFilterReturn bdk_wm_protocols_filter (BdkXEvent *xev,
						BdkEvent  *event,
						gpointer   data);

static GSource *bdk_display_source_new (BdkDisplay *display);
static gboolean bdk_check_xpending     (BdkDisplay *display);

static Bool bdk_xsettings_watch_cb  (Window            window,
				     Bool              is_start,
				     long              mask,
				     void             *cb_data);
static void bdk_xsettings_notify_cb (const char       *name,
				     XSettingsAction   action,
				     XSettingsSetting *setting,
				     void             *data);

/* Private variable declarations
 */

static GList *display_sources;

static GSourceFuncs event_funcs = {
  bdk_event_prepare,
  bdk_event_check,
  bdk_event_dispatch,
  NULL
};

static GSource *
bdk_display_source_new (BdkDisplay *display)
{
  GSource *source = g_source_new (&event_funcs, sizeof (BdkDisplaySource));
  BdkDisplaySource *display_source = (BdkDisplaySource *)source;
  char *name;
  
  name = g_strdup_printf ("BDK X11 Event source (%s)",
			  bdk_display_get_name (display));
  g_source_set_name (source, name);
  g_free (name);
  display_source->display = display;
  
  return source;
}

static gboolean
bdk_check_xpending (BdkDisplay *display)
{
  return XPending (BDK_DISPLAY_XDISPLAY (display));
}

/*********************************************
 * Functions for maintaining the event queue *
 *********************************************/

static void
refcounted_grab_server (Display *xdisplay)
{
  BdkDisplay *display = bdk_x11_lookup_xdisplay (xdisplay);

  bdk_x11_display_grab (display);
}

static void
refcounted_ungrab_server (Display *xdisplay)
{
  BdkDisplay *display = bdk_x11_lookup_xdisplay (xdisplay);
  
  bdk_x11_display_ungrab (display);
}

void
_bdk_x11_events_init_screen (BdkScreen *screen)
{
  BdkScreenX11 *screen_x11 = BDK_SCREEN_X11 (screen);

  /* Keep a flag to avoid extra notifies that we don't need
   */
  screen_x11->xsettings_in_init = TRUE;
  screen_x11->xsettings_client = xsettings_client_new_with_grab_funcs (screen_x11->xdisplay,
						                       screen_x11->screen_num,
						                       bdk_xsettings_notify_cb,
						                       bdk_xsettings_watch_cb,
						                       screen,
                                                                       refcounted_grab_server,
                                                                       refcounted_ungrab_server);
  screen_x11->xsettings_in_init = FALSE;
}

void
_bdk_x11_events_uninit_screen (BdkScreen *screen)
{
  BdkScreenX11 *screen_x11 = BDK_SCREEN_X11 (screen);

  if (screen_x11->xsettings_client)
    {
      xsettings_client_destroy (screen_x11->xsettings_client);
      screen_x11->xsettings_client = NULL;
    }
}

void 
_bdk_events_init (BdkDisplay *display)
{
  GSource *source;
  BdkDisplaySource *display_source;
  BdkDisplayX11 *display_x11 = BDK_DISPLAY_X11 (display);
  
  int connection_number = ConnectionNumber (display_x11->xdisplay);
  BDK_NOTE (MISC, g_message ("connection number: %d", connection_number));


  source = display_x11->event_source = bdk_display_source_new (display);
  display_source = (BdkDisplaySource*) source;
  g_source_set_priority (source, BDK_PRIORITY_EVENTS);
  
  display_source->event_poll_fd.fd = connection_number;
  display_source->event_poll_fd.events = G_IO_IN;
  
  g_source_add_poll (source, &display_source->event_poll_fd);
  g_source_set_can_recurse (source, TRUE);
  g_source_attach (source, NULL);

  display_sources = g_list_prepend (display_sources,display_source);

  bdk_display_add_client_message_filter (display,
					 bdk_atom_intern_static_string ("WM_PROTOCOLS"), 
					 bdk_wm_protocols_filter,   
					 NULL);
}

void
_bdk_events_uninit (BdkDisplay *display)
{
  BdkDisplayX11 *display_x11 = BDK_DISPLAY_X11 (display);

  if (display_x11->event_source)
    {
      display_sources = g_list_remove (display_sources,
                                       display_x11->event_source);
      g_source_destroy (display_x11->event_source);
      g_source_unref (display_x11->event_source);
      display_x11->event_source = NULL;
    }
}

/**
 * bdk_events_pending:
 * 
 * Checks if any events are ready to be processed for any display.
 * 
 * Return value:  %TRUE if any events are pending.
 **/
gboolean
bdk_events_pending (void)
{
  GList *tmp_list;

  for (tmp_list = display_sources; tmp_list; tmp_list = tmp_list->next)
    {
      BdkDisplaySource *tmp_source = tmp_list->data;
      BdkDisplay *display = tmp_source->display;
      
      if (_bdk_event_queue_find_first (display))
	return TRUE;
    }

  for (tmp_list = display_sources; tmp_list; tmp_list = tmp_list->next)
    {
      BdkDisplaySource *tmp_source = tmp_list->data;
      BdkDisplay *display = tmp_source->display;
      
      if (bdk_check_xpending (display))
	return TRUE;
    }
  
  return FALSE;
}

static Bool
graphics_expose_predicate (Display  *display,
			   XEvent   *xevent,
			   XPointer  arg)
{
  if (xevent->xany.window == BDK_DRAWABLE_XID ((BdkDrawable *)arg) &&
      (xevent->xany.type == GraphicsExpose ||
       xevent->xany.type == NoExpose))
    return True;
  else
    return False;
}

/**
 * bdk_event_get_graphics_expose:
 * @window: the #BdkWindow to wait for the events for.
 * 
 * Waits for a GraphicsExpose or NoExpose event from the X server.
 * This is used in the #BtkText and #BtkCList widgets in BTK+ to make sure any
 * GraphicsExpose events are handled before the widget is scrolled.
 *
 * Return value:  a #BdkEventExpose if a GraphicsExpose was received, or %NULL if a
 * NoExpose event was received.
 *
 * Deprecated: 2.18:
 **/
BdkEvent*
bdk_event_get_graphics_expose (BdkWindow *window)
{
  XEvent xevent;
  BdkEvent *event;
  
  g_return_val_if_fail (window != NULL, NULL);

  XIfEvent (BDK_WINDOW_XDISPLAY (window), &xevent, 
	    graphics_expose_predicate, (XPointer) window);
  
  if (xevent.xany.type == GraphicsExpose)
    {
      event = bdk_event_new (BDK_NOTHING);
      
      if (bdk_event_translate (BDK_WINDOW_DISPLAY (window), event,
			       &xevent, TRUE))
	return event;
      else
	bdk_event_free (event);
    }
  
  return NULL;	
}

static gint
bdk_event_apply_filters (XEvent *xevent,
			 BdkEvent *event,
			 BdkWindow *window)
{
  GList *tmp_list;
  BdkFilterReturn result;
  
  if (window == NULL)
    tmp_list = _bdk_default_filters;
  else
    {
      BdkWindowObject *window_private;
      window_private = (BdkWindowObject *) window;
      tmp_list = window_private->filters;
    }

  
  while (tmp_list)
    {
      BdkEventFilter *filter = (BdkEventFilter*) tmp_list->data;
      GList *node;
      
      if ((filter->flags & BDK_EVENT_FILTER_REMOVED) != 0)
        {
          tmp_list = tmp_list->next;
          continue;
        }

      filter->ref_count++;
      result = filter->function (xevent, event, filter->data);

      /* Protect against unreffing the filter mutating the list */
      node = tmp_list->next;

      _bdk_event_filter_unref (window, filter);

      tmp_list = node;

      if (result != BDK_FILTER_CONTINUE)
        return result;
    }
  
  return BDK_FILTER_CONTINUE;
}

/**
 * bdk_display_add_client_message_filter:
 * @display: a #BdkDisplay for which this message filter applies
 * @message_type: the type of ClientMessage events to receive.
 *   This will be checked against the @message_type field 
 *   of the XClientMessage event struct.
 * @func: the function to call to process the event.
 * @data: user data to pass to @func.
 *
 * Adds a filter to be called when X ClientMessage events are received.
 * See bdk_window_add_filter() if you are interested in filtering other
 * types of events.
 *
 * Since: 2.2
 **/ 
void 
bdk_display_add_client_message_filter (BdkDisplay   *display,
				       BdkAtom       message_type,
				       BdkFilterFunc func,
				       gpointer      data)
{
  BdkClientFilter *filter;
  g_return_if_fail (BDK_IS_DISPLAY (display));
  filter = g_new (BdkClientFilter, 1);

  filter->type = message_type;
  filter->function = func;
  filter->data = data;
  
  BDK_DISPLAY_X11(display)->client_filters = 
    g_list_append (BDK_DISPLAY_X11 (display)->client_filters,
		   filter);
}

/**
 * bdk_add_client_message_filter:
 * @message_type: the type of ClientMessage events to receive. This will be
 *     checked against the <structfield>message_type</structfield> field of the
 *     XClientMessage event struct.
 * @func: the function to call to process the event.
 * @data: user data to pass to @func. 
 * 
 * Adds a filter to the default display to be called when X ClientMessage events
 * are received. See bdk_display_add_client_message_filter().
 **/
void 
bdk_add_client_message_filter (BdkAtom       message_type,
			       BdkFilterFunc func,
			       gpointer      data)
{
  bdk_display_add_client_message_filter (bdk_display_get_default (),
					 message_type, func, data);
}

static void
do_net_wm_state_changes (BdkWindow *window)
{
  BdkToplevelX11 *toplevel = _bdk_x11_window_get_toplevel (window);
  BdkWindowState old_state;
  
  if (BDK_WINDOW_DESTROYED (window) ||
      bdk_window_get_window_type (window) != BDK_WINDOW_TOPLEVEL)
    return;
  
  old_state = bdk_window_get_state (window);

  /* For found_sticky to remain TRUE, we have to also be on desktop
   * 0xFFFFFFFF
   */
  if (old_state & BDK_WINDOW_STATE_STICKY)
    {
      if (!(toplevel->have_sticky && toplevel->on_all_desktops))
        bdk_synthesize_window_state (window,
                                     BDK_WINDOW_STATE_STICKY,
                                     0);
    }
  else
    {
      if (toplevel->have_sticky && toplevel->on_all_desktops)
        bdk_synthesize_window_state (window,
                                     0,
                                     BDK_WINDOW_STATE_STICKY);
    }

  if (old_state & BDK_WINDOW_STATE_FULLSCREEN)
    {
      if (!toplevel->have_fullscreen)
        bdk_synthesize_window_state (window,
                                     BDK_WINDOW_STATE_FULLSCREEN,
                                     0);
    }
  else
    {
      if (toplevel->have_fullscreen)
        bdk_synthesize_window_state (window,
                                     0,
                                     BDK_WINDOW_STATE_FULLSCREEN);
    }
  
  /* Our "maximized" means both vertical and horizontal; if only one,
   * we don't expose that via BDK
   */
  if (old_state & BDK_WINDOW_STATE_MAXIMIZED)
    {
      if (!(toplevel->have_maxvert && toplevel->have_maxhorz))
        bdk_synthesize_window_state (window,
                                     BDK_WINDOW_STATE_MAXIMIZED,
                                     0);
    }
  else
    {
      if (toplevel->have_maxvert && toplevel->have_maxhorz)
        bdk_synthesize_window_state (window,
                                     0,
                                     BDK_WINDOW_STATE_MAXIMIZED);
    }

  if (old_state & BDK_WINDOW_STATE_ICONIFIED)
    {
      if (!toplevel->have_hidden)
        bdk_synthesize_window_state (window,
                                     BDK_WINDOW_STATE_ICONIFIED,
                                     0);
    }
  else
    {
      if (toplevel->have_hidden)
        bdk_synthesize_window_state (window,
                                     0,
                                     BDK_WINDOW_STATE_ICONIFIED);
    }
}

static void
bdk_check_wm_desktop_changed (BdkWindow *window)
{
  BdkToplevelX11 *toplevel = _bdk_x11_window_get_toplevel (window);
  BdkDisplay *display = BDK_WINDOW_DISPLAY (window);

  Atom type;
  gint format;
  gulong nitems;
  gulong bytes_after;
  guchar *data;
  gulong *desktop;

  type = None;
  bdk_error_trap_push ();
  XGetWindowProperty (BDK_DISPLAY_XDISPLAY (display), 
                      BDK_WINDOW_XID (window),
                      bdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_DESKTOP"),
                      0, G_MAXLONG, False, XA_CARDINAL, &type, 
                      &format, &nitems,
                      &bytes_after, &data);
  bdk_error_trap_pop ();

  if (type != None)
    {
      desktop = (gulong *)data;
      toplevel->on_all_desktops = ((*desktop & 0xFFFFFFFF) == 0xFFFFFFFF);
      XFree (desktop);
    }
  else
    toplevel->on_all_desktops = FALSE;
      
  do_net_wm_state_changes (window);
}

static void
bdk_check_wm_state_changed (BdkWindow *window)
{
  BdkToplevelX11 *toplevel = _bdk_x11_window_get_toplevel (window);
  BdkDisplay *display = BDK_WINDOW_DISPLAY (window);
  
  Atom type;
  gint format;
  gulong nitems;
  gulong bytes_after;
  guchar *data;
  Atom *atoms = NULL;
  gulong i;

  gboolean had_sticky = toplevel->have_sticky;

  toplevel->have_sticky = FALSE;
  toplevel->have_maxvert = FALSE;
  toplevel->have_maxhorz = FALSE;
  toplevel->have_fullscreen = FALSE;
  toplevel->have_hidden = FALSE;

  type = None;
  bdk_error_trap_push ();
  XGetWindowProperty (BDK_DISPLAY_XDISPLAY (display), BDK_WINDOW_XID (window),
		      bdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_STATE"),
		      0, G_MAXLONG, False, XA_ATOM, &type, &format, &nitems,
		      &bytes_after, &data);
  bdk_error_trap_pop ();

  if (type != None)
    {
      Atom sticky_atom = bdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_STATE_STICKY");
      Atom maxvert_atom = bdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_STATE_MAXIMIZED_VERT");
      Atom maxhorz_atom	= bdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_STATE_MAXIMIZED_HORZ");
      Atom fullscreen_atom = bdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_STATE_FULLSCREEN");
      Atom hidden_atom = bdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_STATE_HIDDEN");

      atoms = (Atom *)data;

      i = 0;
      while (i < nitems)
        {
          if (atoms[i] == sticky_atom)
            toplevel->have_sticky = TRUE;
          else if (atoms[i] == maxvert_atom)
            toplevel->have_maxvert = TRUE;
          else if (atoms[i] == maxhorz_atom)
            toplevel->have_maxhorz = TRUE;
          else if (atoms[i] == fullscreen_atom)
            toplevel->have_fullscreen = TRUE;
          else if (atoms[i] == hidden_atom)
            toplevel->have_hidden = TRUE;
          
          ++i;
        }

      XFree (atoms);
    }

  /* When have_sticky is turned on, we have to check the DESKTOP property
   * as well.
   */
  if (toplevel->have_sticky && !had_sticky)
    bdk_check_wm_desktop_changed (window);
  else
    do_net_wm_state_changes (window);
}

#define HAS_FOCUS(toplevel)                           \
  ((toplevel)->has_focus || (toplevel)->has_pointer_focus)

static void
generate_focus_event (BdkWindow *window,
		      gboolean   in)
{
  BdkEvent event;
  
  event.type = BDK_FOCUS_CHANGE;
  event.focus_change.window = window;
  event.focus_change.send_event = FALSE;
  event.focus_change.in = in;
  
  bdk_event_put (&event);
}

static gboolean
set_screen_from_root (BdkDisplay *display,
		      BdkEvent   *event,
		      Window      xrootwin)
{
  BdkScreen *screen;

  screen = _bdk_x11_display_screen_for_xrootwin (display, xrootwin);

  if (screen)
    {
      bdk_event_set_screen (event, screen);

      return TRUE;
    }
  
  return FALSE;
}

static void
translate_key_event (BdkDisplay *display,
		     BdkEvent   *event,
		     XEvent     *xevent)
{
  BdkKeymap *keymap = bdk_keymap_get_for_display (display);
  gunichar c = 0;
  gchar buf[7];
  BdkModifierType consumed, state;

  event->key.type = xevent->xany.type == KeyPress ? BDK_KEY_PRESS : BDK_KEY_RELEASE;
  event->key.time = xevent->xkey.time;

  event->key.state = (BdkModifierType) xevent->xkey.state;
  event->key.group = _bdk_x11_get_group_for_state (display, xevent->xkey.state);
  event->key.hardware_keycode = xevent->xkey.keycode;

  event->key.keyval = BDK_VoidSymbol;

  bdk_keymap_translate_keyboard_state (keymap,
				       event->key.hardware_keycode,
				       event->key.state,
				       event->key.group,
				       &event->key.keyval,
                                       NULL, NULL, &consumed);
   state = event->key.state & ~consumed;
   _bdk_keymap_add_virtual_modifiers_compat (keymap, &state);
   event->key.state |= state;

  event->key.is_modifier = _bdk_keymap_key_is_modifier (keymap, event->key.hardware_keycode);

  /* Fill in event->string crudely, since various programs
   * depend on it.
   */
  event->key.string = NULL;
  
  if (event->key.keyval != BDK_VoidSymbol)
    c = bdk_keyval_to_unicode (event->key.keyval);

  if (c)
    {
      gsize bytes_written;
      gint len;

      /* Apply the control key - Taken from Xlib
       */
      if (event->key.state & BDK_CONTROL_MASK)
	{
	  if ((c >= '@' && c < '\177') || c == ' ') c &= 0x1F;
	  else if (c == '2')
	    {
	      event->key.string = g_memdup ("\0\0", 2);
	      event->key.length = 1;
	      buf[0] = '\0';
	      goto out;
	    }
	  else if (c >= '3' && c <= '7') c -= ('3' - '\033');
	  else if (c == '8') c = '\177';
	  else if (c == '/') c = '_' & 0x1F;
	}
      
      len = g_unichar_to_utf8 (c, buf);
      buf[len] = '\0';
      
      event->key.string = g_locale_from_utf8 (buf, len,
					      NULL, &bytes_written,
					      NULL);
      if (event->key.string)
	event->key.length = bytes_written;
    }
  else if (event->key.keyval == BDK_Escape)
    {
      event->key.length = 1;
      event->key.string = g_strdup ("\033");
    }
  else if (event->key.keyval == BDK_Return ||
	  event->key.keyval == BDK_KP_Enter)
    {
      event->key.length = 1;
      event->key.string = g_strdup ("\r");
    }

  if (!event->key.string)
    {
      event->key.length = 0;
      event->key.string = g_strdup ("");
    }
  
 out:
#ifdef G_ENABLE_DEBUG
  if (_bdk_debug_flags & BDK_DEBUG_EVENTS)
    {
      g_message ("%s:\t\twindow: %ld	 key: %12s  %d",
		 event->type == BDK_KEY_PRESS ? "key press  " : "key release",
		 xevent->xkey.window,
		 event->key.keyval ? bdk_keyval_name (event->key.keyval) : "(none)",
		 event->key.keyval);
  
      if (event->key.length > 0)
	g_message ("\t\tlength: %4d string: \"%s\"",
		   event->key.length, buf);
    }
#endif /* G_ENABLE_DEBUG */  
  return;
}

/**
 * bdk_x11_register_standard_event_type:
 * @display: a #BdkDisplay
 * @event_base: first event type code to register
 * @n_events: number of event type codes to register
 * 
 * Registers interest in receiving extension events with type codes
 * between @event_base and <literal>event_base + n_events - 1</literal>.
 * The registered events must have the window field in the same place
 * as core X events (this is not the case for e.g. XKB extension events).
 *
 * If an event type is registered, events of this type will go through
 * global and window-specific filters (see bdk_window_add_filter()). 
 * Unregistered events will only go through global filters.
 * BDK may register the events of some X extensions on its own.
 * 
 * This function should only be needed in unusual circumstances, e.g.
 * when filtering XInput extension events on the root window.
 *
 * Since: 2.4
 **/
void
bdk_x11_register_standard_event_type (BdkDisplay          *display,
				      gint                 event_base,
				      gint                 n_events)
{
  BdkEventTypeX11 *event_type;
  BdkDisplayX11 *display_x11;

  display_x11 = BDK_DISPLAY_X11 (display);
  event_type = g_new (BdkEventTypeX11, 1);

  event_type->base = event_base;
  event_type->n_events = n_events;

  display_x11->event_types = g_slist_prepend (display_x11->event_types, event_type);
}

/* Return the window this has to do with, if any, rather
 * than the frame or root window that was selecting
 * for substructure
 */
static void
get_real_window (BdkDisplay *display,
		 XEvent     *event,
		 Window     *event_window,
		 Window     *filter_window)
{
  /* Core events all have an event->xany.window field, but that's
   * not true for extension events
   */
  if (event->type >= KeyPress &&
      event->type <= MappingNotify)
    {
      *filter_window = event->xany.window;
      switch (event->type)
	{      
	case CreateNotify:
	  *event_window = event->xcreatewindow.window;
	  break;
	case DestroyNotify:
	  *event_window = event->xdestroywindow.window;
	  break;
	case UnmapNotify:
	  *event_window = event->xunmap.window;
	  break;
	case MapNotify:
	  *event_window = event->xmap.window;
	  break;
	case MapRequest:
	  *event_window = event->xmaprequest.window;
	  break;
	case ReparentNotify:
	  *event_window = event->xreparent.window;
	  break;
	case ConfigureNotify:
	  *event_window = event->xconfigure.window;
	  break;
	case ConfigureRequest:
	  *event_window = event->xconfigurerequest.window;
	  break;
	case GravityNotify:
	  *event_window = event->xgravity.window;
	  break;
	case CirculateNotify:
	  *event_window = event->xcirculate.window;
	  break;
	case CirculateRequest:
	  *event_window = event->xcirculaterequest.window;
	  break;
	default:
	  *event_window = event->xany.window;
	}
    }
  else
    {
      BdkDisplayX11 *display_x11 = BDK_DISPLAY_X11 (display);
      GSList *tmp_list;

      for (tmp_list = display_x11->event_types;
	   tmp_list;
	   tmp_list = tmp_list->next)
	{
	  BdkEventTypeX11 *event_type = tmp_list->data;

	  if (event->type >= event_type->base &&
	      event->type < event_type->base + event_type->n_events)
	    {
	      *event_window = event->xany.window;
	      *filter_window = event->xany.window;
	      return;
	    }
	}
	   
      *event_window = None;
      *filter_window = None;
    }
}

#ifdef G_ENABLE_DEBUG
static const char notify_modes[][19] = {
  "NotifyNormal",
  "NotifyGrab",
  "NotifyUngrab",
  "NotifyWhileGrabbed"
};

static const char notify_details[][23] = {
  "NotifyAncestor",
  "NotifyVirtual",
  "NotifyInferior",
  "NotifyNonlinear",
  "NotifyNonlinearVirtual",
  "NotifyPointer",
  "NotifyPointerRoot",
  "NotifyDetailNone"
};
#endif

static void
set_user_time (BdkWindow *window,
	       BdkEvent  *event)
{
  g_return_if_fail (event != NULL);

  window = bdk_window_get_toplevel (event->client.window);
  g_return_if_fail (BDK_IS_WINDOW (window));

  /* If an event doesn't have a valid timestamp, we shouldn't use it
   * to update the latest user interaction time.
   */
  if (bdk_event_get_time (event) != BDK_CURRENT_TIME)
    bdk_x11_window_set_user_time (bdk_window_get_toplevel (window),
                                  bdk_event_get_time (event));
}

static gboolean
is_parent_of (BdkWindow *parent,
              BdkWindow *child)
{
  BdkWindow *w;

  w = child;
  while (w != NULL)
    {
      if (w == parent)
	return TRUE;

      w = bdk_window_get_parent (w);
    }

  return FALSE;
}

static gboolean
bdk_event_translate (BdkDisplay *display,
		     BdkEvent   *event,
		     XEvent     *xevent,
		     gboolean    return_exposes)
{
  
  BdkWindow *window;
  BdkWindowObject *window_private;
  BdkWindow *filter_window;
  BdkWindowImplX11 *window_impl = NULL;
  gboolean return_val;
  BdkScreen *screen = NULL;
  BdkScreenX11 *screen_x11 = NULL;
  BdkToplevelX11 *toplevel = NULL;
  BdkDisplayX11 *display_x11 = BDK_DISPLAY_X11 (display);
  Window xwindow, filter_xwindow;
  
  return_val = FALSE;

  /* init these, since the done: block uses them */
  window = NULL;
  window_private = NULL;
  event->any.window = NULL;

  if (_bdk_default_filters)
    {
      /* Apply global filters */
      BdkFilterReturn result;
      result = bdk_event_apply_filters (xevent, event, NULL);
      
      if (result != BDK_FILTER_CONTINUE)
        {
          return_val = (result == BDK_FILTER_TRANSLATE) ? TRUE : FALSE;
          goto done;
        }
    }  

  /* Find the BdkWindow that this event relates to.
   * Basically this means substructure events
   * are reported same as structure events
   */
  get_real_window (display, xevent, &xwindow, &filter_xwindow);
  
  window = bdk_window_lookup_for_display (display, xwindow);
  /* We may receive events such as NoExpose/GraphicsExpose
   * and ShmCompletion for pixmaps
   */
  if (window && !BDK_IS_WINDOW (window))
    window = NULL;
  window_private = (BdkWindowObject *) window;

  /* We always run the filters for the window where the event
   * is delivered, not the window that it relates to
   */
  if (filter_xwindow == xwindow)
    filter_window = window;
  else
    {
      filter_window = bdk_window_lookup_for_display (display, filter_xwindow);
      if (filter_window && !BDK_IS_WINDOW (filter_window))
	filter_window = NULL;
    }

  if (window)
    {
      screen = BDK_WINDOW_SCREEN (window);
      screen_x11 = BDK_SCREEN_X11 (screen);
      toplevel = _bdk_x11_window_get_toplevel (window);
    }
    
  if (window != NULL)
    {
      /* Apply keyboard grabs to non-native windows */
      if (/* Is key event */
	  (xevent->type == KeyPress || xevent->type == KeyRelease) &&
	  /* And we have a grab */
	  display->keyboard_grab.window != NULL &&
	  (
	   /* The window is not a descendant of the grabbed window */
	   !is_parent_of ((BdkWindow *)display->keyboard_grab.window, window) ||
	   /* Or owner event is false */
	   !display->keyboard_grab.owner_events
	   )
	  )
        {
	  /* Report key event against grab window */
          window = display->keyboard_grab.window;;
          window_private = (BdkWindowObject *) window;
        }

      window_impl = BDK_WINDOW_IMPL_X11 (window_private->impl);
      
      /* Move key events on focus window to the real toplevel, and
       * filter out all other events on focus window
       */          
      if (toplevel && xwindow == toplevel->focus_window)
	{
	  switch (xevent->type)
	    {
	    case KeyPress:
	    case KeyRelease:
	      xwindow = BDK_WINDOW_XID (window);
	      xevent->xany.window = xwindow;
	      break;
	    default:
	      return FALSE;
	    }
	}

      g_object_ref (window);
    }

  event->any.window = window;
  event->any.send_event = xevent->xany.send_event ? TRUE : FALSE;
  
  if (window_private && BDK_WINDOW_DESTROYED (window))
    {
      if (xevent->type != DestroyNotify)
	{
	  return_val = FALSE;
	  goto done;
	}
    }
  else if (filter_window)
    {
      /* Apply per-window filters */
      BdkWindowObject *filter_private = (BdkWindowObject *) filter_window;
      BdkFilterReturn result;

      if (filter_private->filters)
	{
	  g_object_ref (filter_window);
	  
	  result = bdk_event_apply_filters (xevent, event,
					    filter_window);
	  
	  g_object_unref (filter_window);
      
	  if (result != BDK_FILTER_CONTINUE)
	    {
	      return_val = (result == BDK_FILTER_TRANSLATE) ? TRUE : FALSE;
	      goto done;
	    }
	}
    }

  if (xevent->type == DestroyNotify)
    {
      int i, n;

      n = bdk_display_get_n_screens (display);
      for (i = 0; i < n; i++)
        {
          screen = bdk_display_get_screen (display, i);
          screen_x11 = BDK_SCREEN_X11 (screen);

          if (screen_x11->wmspec_check_window == xwindow)
            {
              screen_x11->wmspec_check_window = None;
              screen_x11->last_wmspec_check_time = 0;
              g_free (screen_x11->window_manager_name);
              screen_x11->window_manager_name = g_strdup ("unknown");

              /* careful, reentrancy */
              _bdk_x11_screen_window_manager_changed (screen);

              return_val = FALSE;
              goto done;
            }
        }
    }

  if (window &&
      (xevent->xany.type == MotionNotify ||
       xevent->xany.type == ButtonRelease))
    {
      if (_bdk_moveresize_handle_event (xevent))
	{
          return_val = FALSE;
          goto done;
        }
    }
  
  /* We do a "manual" conversion of the XEvent to a
   *  BdkEvent. The structures are mostly the same so
   *  the conversion is fairly straightforward. We also
   *  optionally print debugging info regarding events
   *  received.
   */

  return_val = TRUE;

  switch (xevent->type)
    {
    case KeyPress:
      if (window_private == NULL)
        {
          return_val = FALSE;
          break;
        }
      translate_key_event (display, event, xevent);
      set_user_time (window, event);
      break;

    case KeyRelease:
      if (window_private == NULL)
        {
          return_val = FALSE;
          break;
        }
      
      /* Emulate detectable auto-repeat by checking to see
       * if the next event is a key press with the same
       * keycode and timestamp, and if so, ignoring the event.
       */

      if (!display_x11->have_xkb_autorepeat && XPending (xevent->xkey.display))
	{
	  XEvent next_event;

	  XPeekEvent (xevent->xkey.display, &next_event);

	  if (next_event.type == KeyPress &&
	      next_event.xkey.keycode == xevent->xkey.keycode &&
	      next_event.xkey.time == xevent->xkey.time)
	    {
	      return_val = FALSE;
	      break;
	    }
	}

      translate_key_event (display, event, xevent);
      break;
      
    case ButtonPress:
      BDK_NOTE (EVENTS, 
		g_message ("button press:\t\twindow: %ld  x,y: %d %d  button: %d",
			   xevent->xbutton.window,
			   xevent->xbutton.x, xevent->xbutton.y,
			   xevent->xbutton.button));
      
      if (window_private == NULL)
	{
	  return_val = FALSE;
	  break;
	}
      
      /* If we get a ButtonPress event where the button is 4 or 5,
	 it's a Scroll event */
      switch (xevent->xbutton.button)
        {
        case 4: /* up */
        case 5: /* down */
        case 6: /* left */
        case 7: /* right */
	  event->scroll.type = BDK_SCROLL;

          if (xevent->xbutton.button == 4)
            event->scroll.direction = BDK_SCROLL_UP;
          else if (xevent->xbutton.button == 5)
            event->scroll.direction = BDK_SCROLL_DOWN;
          else if (xevent->xbutton.button == 6)
            event->scroll.direction = BDK_SCROLL_LEFT;
          else
            event->scroll.direction = BDK_SCROLL_RIGHT;

	  event->scroll.window = window;
	  event->scroll.time = xevent->xbutton.time;
	  event->scroll.x = xevent->xbutton.x;
	  event->scroll.y = xevent->xbutton.y;
	  event->scroll.x_root = (gfloat)xevent->xbutton.x_root;
	  event->scroll.y_root = (gfloat)xevent->xbutton.y_root;
	  event->scroll.state = (BdkModifierType) xevent->xbutton.state;
	  event->scroll.device = display->core_pointer;
	  
	  if (!set_screen_from_root (display, event, xevent->xbutton.root))
	    {
	      return_val = FALSE;
	      break;
	    }
	  
          break;
          
        default:
	  event->button.type = BDK_BUTTON_PRESS;
	  event->button.window = window;
	  event->button.time = xevent->xbutton.time;
	  event->button.x = xevent->xbutton.x;
	  event->button.y = xevent->xbutton.y;
	  event->button.x_root = (gfloat)xevent->xbutton.x_root;
	  event->button.y_root = (gfloat)xevent->xbutton.y_root;
	  event->button.axes = NULL;
	  event->button.state = (BdkModifierType) xevent->xbutton.state;
	  event->button.button = xevent->xbutton.button;
	  event->button.device = display->core_pointer;
	  
	  if (!set_screen_from_root (display, event, xevent->xbutton.root))
	    {
	      return_val = FALSE;
	      break;
	    }
          break;
	}

      set_user_time (window, event);

      break;
      
    case ButtonRelease:
      BDK_NOTE (EVENTS, 
		g_message ("button release:\twindow: %ld  x,y: %d %d  button: %d",
			   xevent->xbutton.window,
			   xevent->xbutton.x, xevent->xbutton.y,
			   xevent->xbutton.button));
      
      if (window_private == NULL)
	{
	  return_val = FALSE;
	  break;
	}
      
      /* We treat button presses as scroll wheel events, so ignore the release */
      if (xevent->xbutton.button == 4 || xevent->xbutton.button == 5 ||
          xevent->xbutton.button == 6 || xevent->xbutton.button ==7)
	{
	  return_val = FALSE;
	  break;
	}

      event->button.type = BDK_BUTTON_RELEASE;
      event->button.window = window;
      event->button.time = xevent->xbutton.time;
      event->button.x = xevent->xbutton.x;
      event->button.y = xevent->xbutton.y;
      event->button.x_root = (gfloat)xevent->xbutton.x_root;
      event->button.y_root = (gfloat)xevent->xbutton.y_root;
      event->button.axes = NULL;
      event->button.state = (BdkModifierType) xevent->xbutton.state;
      event->button.button = xevent->xbutton.button;
      event->button.device = display->core_pointer;

      if (!set_screen_from_root (display, event, xevent->xbutton.root))
	return_val = FALSE;
      
      break;
      
    case MotionNotify:
      BDK_NOTE (EVENTS,
		g_message ("motion notify:\t\twindow: %ld  x,y: %d %d  hint: %s", 
			   xevent->xmotion.window,
			   xevent->xmotion.x, xevent->xmotion.y,
			   (xevent->xmotion.is_hint) ? "true" : "false"));
      
      if (window_private == NULL)
	{
	  return_val = FALSE;
	  break;
	}

      event->motion.type = BDK_MOTION_NOTIFY;
      event->motion.window = window;
      event->motion.time = xevent->xmotion.time;
      event->motion.x = xevent->xmotion.x;
      event->motion.y = xevent->xmotion.y;
      event->motion.x_root = (gfloat)xevent->xmotion.x_root;
      event->motion.y_root = (gfloat)xevent->xmotion.y_root;
      event->motion.axes = NULL;
      event->motion.state = (BdkModifierType) xevent->xmotion.state;
      event->motion.is_hint = xevent->xmotion.is_hint;
      event->motion.device = display->core_pointer;
      
      if (!set_screen_from_root (display, event, xevent->xbutton.root))
	{
	  return_val = FALSE;
	  break;
	}
            
      break;
      
    case EnterNotify:
      BDK_NOTE (EVENTS,
                g_message ("enter notify:\t\twindow: %ld  detail: %d subwin: %ld mode: %d",
                           xevent->xcrossing.window,
                           xevent->xcrossing.detail,
                           xevent->xcrossing.subwindow,
                           xevent->xcrossing.mode));

      if (window_private == NULL)
        {
          return_val = FALSE;
          break;
        }
      
      if (!set_screen_from_root (display, event, xevent->xbutton.root))
	{
	  return_val = FALSE;
	  break;
	}
      
      /* Handle focusing (in the case where no window manager is running */
      if (toplevel && xevent->xcrossing.detail != NotifyInferior)
	{
	  toplevel->has_pointer = TRUE;

	  if (xevent->xcrossing.focus && !toplevel->has_focus_window)
	    {
	      gboolean had_focus = HAS_FOCUS (toplevel);
	      
	      toplevel->has_pointer_focus = TRUE;
	      
	      if (HAS_FOCUS (toplevel) != had_focus)
		generate_focus_event (window, TRUE);
	    }
	}

      event->crossing.type = BDK_ENTER_NOTIFY;
      event->crossing.window = window;
      
      /* If the subwindow field of the XEvent is non-NULL, then
       *  lookup the corresponding BdkWindow.
       */
      if (xevent->xcrossing.subwindow != None)
	event->crossing.subwindow = bdk_window_lookup_for_display (display, xevent->xcrossing.subwindow);
      else
	event->crossing.subwindow = NULL;
      
      event->crossing.time = xevent->xcrossing.time;
      event->crossing.x = xevent->xcrossing.x;
      event->crossing.y = xevent->xcrossing.y;
      event->crossing.x_root = xevent->xcrossing.x_root;
      event->crossing.y_root = xevent->xcrossing.y_root;
      
      /* Translate the crossing mode into Bdk terms.
       */
      switch (xevent->xcrossing.mode)
	{
	case NotifyNormal:
	  event->crossing.mode = BDK_CROSSING_NORMAL;
	  break;
	case NotifyGrab:
	  event->crossing.mode = BDK_CROSSING_GRAB;
	  break;
	case NotifyUngrab:
	  event->crossing.mode = BDK_CROSSING_UNGRAB;
	  break;
	};
      
      /* Translate the crossing detail into Bdk terms.
       */
      switch (xevent->xcrossing.detail)
	{
	case NotifyInferior:
	  event->crossing.detail = BDK_NOTIFY_INFERIOR;
	  break;
	case NotifyAncestor:
	  event->crossing.detail = BDK_NOTIFY_ANCESTOR;
	  break;
	case NotifyVirtual:
	  event->crossing.detail = BDK_NOTIFY_VIRTUAL;
	  break;
	case NotifyNonlinear:
	  event->crossing.detail = BDK_NOTIFY_NONLINEAR;
	  break;
	case NotifyNonlinearVirtual:
	  event->crossing.detail = BDK_NOTIFY_NONLINEAR_VIRTUAL;
	  break;
	default:
	  event->crossing.detail = BDK_NOTIFY_UNKNOWN;
	  break;
	}
      
      event->crossing.focus = xevent->xcrossing.focus;
      event->crossing.state = xevent->xcrossing.state;
  
      break;
      
    case LeaveNotify:
      BDK_NOTE (EVENTS, 
                g_message ("leave notify:\t\twindow: %ld  detail: %d subwin: %ld mode: %d",
                           xevent->xcrossing.window,
                           xevent->xcrossing.detail,
                           xevent->xcrossing.subwindow,
                           xevent->xcrossing.mode));

      if (window_private == NULL)
        {
          return_val = FALSE;
          break;
        }

      if (!set_screen_from_root (display, event, xevent->xbutton.root))
	{
	  return_val = FALSE;
	  break;
	}
                  
      /* Handle focusing (in the case where no window manager is running */
      if (toplevel && xevent->xcrossing.detail != NotifyInferior)
	{
	  toplevel->has_pointer = FALSE;

	  if (xevent->xcrossing.focus && !toplevel->has_focus_window)
	    {
	      gboolean had_focus = HAS_FOCUS (toplevel);
	      
	      toplevel->has_pointer_focus = FALSE;
	      
	      if (HAS_FOCUS (toplevel) != had_focus)
		generate_focus_event (window, FALSE);
	    }
	}

      event->crossing.type = BDK_LEAVE_NOTIFY;
      event->crossing.window = window;
      
      /* If the subwindow field of the XEvent is non-NULL, then
       *  lookup the corresponding BdkWindow.
       */
      if (xevent->xcrossing.subwindow != None)
	event->crossing.subwindow = bdk_window_lookup_for_display (display, xevent->xcrossing.subwindow);
      else
	event->crossing.subwindow = NULL;
      
      event->crossing.time = xevent->xcrossing.time;
      event->crossing.x = xevent->xcrossing.x;
      event->crossing.y = xevent->xcrossing.y;
      event->crossing.x_root = xevent->xcrossing.x_root;
      event->crossing.y_root = xevent->xcrossing.y_root;
      
      /* Translate the crossing mode into Bdk terms.
       */
      switch (xevent->xcrossing.mode)
	{
	case NotifyNormal:
	  event->crossing.mode = BDK_CROSSING_NORMAL;
	  break;
	case NotifyGrab:
	  event->crossing.mode = BDK_CROSSING_GRAB;
	  break;
	case NotifyUngrab:
	  event->crossing.mode = BDK_CROSSING_UNGRAB;
	  break;
	};
      
      /* Translate the crossing detail into Bdk terms.
       */
      switch (xevent->xcrossing.detail)
	{
	case NotifyInferior:
	  event->crossing.detail = BDK_NOTIFY_INFERIOR;
	  break;
	case NotifyAncestor:
	  event->crossing.detail = BDK_NOTIFY_ANCESTOR;
	  break;
	case NotifyVirtual:
	  event->crossing.detail = BDK_NOTIFY_VIRTUAL;
	  break;
	case NotifyNonlinear:
	  event->crossing.detail = BDK_NOTIFY_NONLINEAR;
	  break;
	case NotifyNonlinearVirtual:
	  event->crossing.detail = BDK_NOTIFY_NONLINEAR_VIRTUAL;
	  break;
	default:
	  event->crossing.detail = BDK_NOTIFY_UNKNOWN;
	  break;
	}
      
      event->crossing.focus = xevent->xcrossing.focus;
      event->crossing.state = xevent->xcrossing.state;
      
      break;
      
      /* We only care about focus events that indicate that _this_
       * window (not a ancestor or child) got or lost the focus
       */
    case FocusIn:
      BDK_NOTE (EVENTS,
		g_message ("focus in:\t\twindow: %ld, detail: %s, mode: %s",
			   xevent->xfocus.window,
			   notify_details[xevent->xfocus.detail],
			   notify_modes[xevent->xfocus.mode]));
      
      if (toplevel)
	{
	  gboolean had_focus = HAS_FOCUS (toplevel);
	  
	  switch (xevent->xfocus.detail)
	    {
	    case NotifyAncestor:
	    case NotifyVirtual:
	      /* When the focus moves from an ancestor of the window to
	       * the window or a descendent of the window, *and* the
	       * pointer is inside the window, then we were previously
	       * receiving keystroke events in the has_pointer_focus
	       * case and are now receiving them in the
	       * has_focus_window case.
	       */
	      if (toplevel->has_pointer &&
		  xevent->xfocus.mode != NotifyGrab &&
		  xevent->xfocus.mode != NotifyUngrab)
		toplevel->has_pointer_focus = FALSE;
	      
	      /* fall through */
	    case NotifyNonlinear:
	    case NotifyNonlinearVirtual:
	      if (xevent->xfocus.mode != NotifyGrab &&
		  xevent->xfocus.mode != NotifyUngrab)
		toplevel->has_focus_window = TRUE;
	      /* We pretend that the focus moves to the grab
	       * window, so we pay attention to NotifyGrab
	       * NotifyUngrab, and ignore NotifyWhileGrabbed
	       */
	      if (xevent->xfocus.mode != NotifyWhileGrabbed)
		toplevel->has_focus = TRUE;
	      break;
	    case NotifyPointer:
	      /* The X server sends NotifyPointer/NotifyGrab,
	       * but the pointer focus is ignored while a
	       * grab is in effect
	       */
	      if (xevent->xfocus.mode != NotifyGrab &&
		  xevent->xfocus.mode != NotifyUngrab)
		toplevel->has_pointer_focus = TRUE;
	      break;
	    case NotifyInferior:
	    case NotifyPointerRoot:
	    case NotifyDetailNone:
	      break;
	    }

	  if (HAS_FOCUS (toplevel) != had_focus)
	    generate_focus_event (window, TRUE);
	}
      break;
    case FocusOut:
      BDK_NOTE (EVENTS,
		g_message ("focus out:\t\twindow: %ld, detail: %s, mode: %s",
			   xevent->xfocus.window,
			   notify_details[xevent->xfocus.detail],
			   notify_modes[xevent->xfocus.mode]));
      
      if (toplevel)
	{
	  gboolean had_focus = HAS_FOCUS (toplevel);
	    
	  switch (xevent->xfocus.detail)
	    {
	    case NotifyAncestor:
	    case NotifyVirtual:
	      /* When the focus moves from the window or a descendent
	       * of the window to an ancestor of the window, *and* the
	       * pointer is inside the window, then we were previously
	       * receiving keystroke events in the has_focus_window
	       * case and are now receiving them in the
	       * has_pointer_focus case.
	       */
	      if (toplevel->has_pointer &&
		  xevent->xfocus.mode != NotifyGrab &&
		  xevent->xfocus.mode != NotifyUngrab)
		toplevel->has_pointer_focus = TRUE;

	      /* fall through */
	    case NotifyNonlinear:
	    case NotifyNonlinearVirtual:
	      if (xevent->xfocus.mode != NotifyGrab &&
		  xevent->xfocus.mode != NotifyUngrab)
		toplevel->has_focus_window = FALSE;
	      if (xevent->xfocus.mode != NotifyWhileGrabbed)
		toplevel->has_focus = FALSE;
	      break;
	    case NotifyPointer:
	      if (xevent->xfocus.mode != NotifyGrab &&
		  xevent->xfocus.mode != NotifyUngrab)
		toplevel->has_pointer_focus = FALSE;
	    break;
	    case NotifyInferior:
	    case NotifyPointerRoot:
	    case NotifyDetailNone:
	      break;
	    }

	  if (HAS_FOCUS (toplevel) != had_focus)
	    generate_focus_event (window, FALSE);
	}
      break;

#if 0      
 	  /* bdk_keyboard_grab() causes following events. These events confuse
 	   * the XIM focus, so ignore them.
 	   */
 	  if (xevent->xfocus.mode == NotifyGrab ||
 	      xevent->xfocus.mode == NotifyUngrab)
 	    break;
#endif

    case KeymapNotify:
      BDK_NOTE (EVENTS,
		g_message ("keymap notify"));

      /* Not currently handled */
      return_val = FALSE;
      break;
      
    case Expose:
      BDK_NOTE (EVENTS,
		g_message ("expose:\t\twindow: %ld  %d	x,y: %d %d  w,h: %d %d%s",
			   xevent->xexpose.window, xevent->xexpose.count,
			   xevent->xexpose.x, xevent->xexpose.y,
			   xevent->xexpose.width, xevent->xexpose.height,
			   event->any.send_event ? " (send)" : ""));
     
      if (window_private == NULL)
        {
          return_val = FALSE;
          break;
        }

      {
	BdkRectangle expose_rect;

	expose_rect.x = xevent->xexpose.x;
	expose_rect.y = xevent->xexpose.y;
	expose_rect.width = xevent->xexpose.width;
	expose_rect.height = xevent->xexpose.height;

	_bdk_window_process_expose (window, xevent->xexpose.serial, &expose_rect);
	 return_val = FALSE;
      }

      break;
      
    case GraphicsExpose:
      {
	BdkRectangle expose_rect;

        BDK_NOTE (EVENTS,
		  g_message ("graphics expose:\tdrawable: %ld",
			     xevent->xgraphicsexpose.drawable));
 
        if (window_private == NULL)
          {
            return_val = FALSE;
            break;
          }
        
	expose_rect.x = xevent->xgraphicsexpose.x;
	expose_rect.y = xevent->xgraphicsexpose.y;
	expose_rect.width = xevent->xgraphicsexpose.width;
	expose_rect.height = xevent->xgraphicsexpose.height;
	    
	if (return_exposes)
	  {
	    event->expose.type = BDK_EXPOSE;
	    event->expose.area = expose_rect;
	    event->expose.rebunnyion = bdk_rebunnyion_rectangle (&expose_rect);
	    event->expose.window = window;
	    event->expose.count = xevent->xgraphicsexpose.count;

	    return_val = TRUE;
	  }
	else
	  {
	    _bdk_window_process_expose (window, xevent->xgraphicsexpose.serial, &expose_rect);
	    
	    return_val = FALSE;
	  }
	
      }
      break;
      
    case NoExpose:
      BDK_NOTE (EVENTS,
		g_message ("no expose:\t\tdrawable: %ld",
			   xevent->xnoexpose.drawable));
      
      event->no_expose.type = BDK_NO_EXPOSE;
      event->no_expose.window = window;
      
      break;
      
    case VisibilityNotify:
#ifdef G_ENABLE_DEBUG
      if (_bdk_debug_flags & BDK_DEBUG_EVENTS)
	switch (xevent->xvisibility.state)
	  {
	  case VisibilityFullyObscured:
	    g_message ("visibility notify:\twindow: %ld	 none",
		       xevent->xvisibility.window);
	    break;
	  case VisibilityPartiallyObscured:
	    g_message ("visibility notify:\twindow: %ld	 partial",
		       xevent->xvisibility.window);
	    break;
	  case VisibilityUnobscured:
	    g_message ("visibility notify:\twindow: %ld	 full",
		       xevent->xvisibility.window);
	    break;
	  }
#endif /* G_ENABLE_DEBUG */
      
      if (window_private == NULL)
        {
          return_val = FALSE;
          break;
        }
      
      event->visibility.type = BDK_VISIBILITY_NOTIFY;
      event->visibility.window = window;
      
      switch (xevent->xvisibility.state)
	{
	case VisibilityFullyObscured:
	  event->visibility.state = BDK_VISIBILITY_FULLY_OBSCURED;
	  break;
	  
	case VisibilityPartiallyObscured:
	  event->visibility.state = BDK_VISIBILITY_PARTIAL;
	  break;
	  
	case VisibilityUnobscured:
	  event->visibility.state = BDK_VISIBILITY_UNOBSCURED;
	  break;
	}
      
      break;
      
    case CreateNotify:
      BDK_NOTE (EVENTS,
		g_message ("create notify:\twindow: %ld  x,y: %d %d	w,h: %d %d  b-w: %d  parent: %ld	 ovr: %d",
			   xevent->xcreatewindow.window,
			   xevent->xcreatewindow.x,
			   xevent->xcreatewindow.y,
			   xevent->xcreatewindow.width,
			   xevent->xcreatewindow.height,
			   xevent->xcreatewindow.border_width,
			   xevent->xcreatewindow.parent,
			   xevent->xcreatewindow.override_redirect));
      /* not really handled */
      break;
      
    case DestroyNotify:
      BDK_NOTE (EVENTS,
		g_message ("destroy notify:\twindow: %ld",
			   xevent->xdestroywindow.window));

      /* Ignore DestroyNotify from SubstructureNotifyMask */
      if (xevent->xdestroywindow.window == xevent->xdestroywindow.event)
	{
	  event->any.type = BDK_DESTROY;
	  event->any.window = window;
	  
	  return_val = window_private && !BDK_WINDOW_DESTROYED (window);
	  
	  if (window && BDK_WINDOW_XID (window) != screen_x11->xroot_window)
	    bdk_window_destroy_notify (window);
	}
      else
	return_val = FALSE;
      
      break;
      
    case UnmapNotify:
      BDK_NOTE (EVENTS,
		g_message ("unmap notify:\t\twindow: %ld",
			   xevent->xmap.window));
      
      event->any.type = BDK_UNMAP;
      event->any.window = window;      

      /* If the WM supports the _NET_WM_STATE_HIDDEN hint, we do not want to
       * interpret UnmapNotify events as implying iconic state.
       * http://bugzilla.bunny.org/show_bug.cgi?id=590726.
       */
      if (screen &&
          !bdk_x11_screen_supports_net_wm_hint (screen,
                                                bdk_atom_intern_static_string ("_NET_WM_STATE_HIDDEN")))
        {
          /* If we are shown (not withdrawn) and get an unmap, it means we were
           * iconified in the X sense. If we are withdrawn, and get an unmap, it
           * means we hid the window ourselves, so we will have already flipped
           * the iconified bit off.
           */
          if (window && BDK_WINDOW_IS_MAPPED (window))
            bdk_synthesize_window_state (window,
                                         0,
                                         BDK_WINDOW_STATE_ICONIFIED);
        }

      if (window)
        _bdk_xgrab_check_unmap (window, xevent->xany.serial);

      break;
      
    case MapNotify:
      BDK_NOTE (EVENTS,
		g_message ("map notify:\t\twindow: %ld",
			   xevent->xmap.window));
      
      event->any.type = BDK_MAP;
      event->any.window = window;

      /* Unset iconified if it was set */
      if (window && (((BdkWindowObject*)window)->state & BDK_WINDOW_STATE_ICONIFIED))
        bdk_synthesize_window_state (window,
                                     BDK_WINDOW_STATE_ICONIFIED,
                                     0);
      
      break;
      
    case ReparentNotify:
      BDK_NOTE (EVENTS,
		g_message ("reparent notify:\twindow: %ld  x,y: %d %d  parent: %ld	ovr: %d",
			   xevent->xreparent.window,
			   xevent->xreparent.x,
			   xevent->xreparent.y,
			   xevent->xreparent.parent,
			   xevent->xreparent.override_redirect));

      /* Not currently handled */
      return_val = FALSE;
      break;
      
    case ConfigureNotify:
      BDK_NOTE (EVENTS,
		g_message ("configure notify:\twindow: %ld  x,y: %d %d	w,h: %d %d  b-w: %d  above: %ld	 ovr: %d%s",
			   xevent->xconfigure.window,
			   xevent->xconfigure.x,
			   xevent->xconfigure.y,
			   xevent->xconfigure.width,
			   xevent->xconfigure.height,
			   xevent->xconfigure.border_width,
			   xevent->xconfigure.above,
			   xevent->xconfigure.override_redirect,
			   !window
			   ? " (discarding)"
			   : BDK_WINDOW_TYPE (window) == BDK_WINDOW_CHILD
			   ? " (discarding child)"
			   : xevent->xconfigure.event != xevent->xconfigure.window
			   ? " (discarding substructure)"
			   : ""));
      if (window && BDK_WINDOW_TYPE (window) == BDK_WINDOW_ROOT)
        { 
	  window_private->width = xevent->xconfigure.width;
	  window_private->height = xevent->xconfigure.height;

	  _bdk_window_update_size (window);
	  _bdk_x11_drawable_update_size (window_private->impl);
	  _bdk_x11_screen_size_changed (screen, xevent);
        }

      if (window &&
	  xevent->xconfigure.event == xevent->xconfigure.window &&
	  !BDK_WINDOW_DESTROYED (window) &&
	  window_private->input_window != NULL)
	_bdk_input_configure_event (&xevent->xconfigure, window);
      
#ifdef HAVE_XSYNC
      if (toplevel && display_x11->use_sync && !XSyncValueIsZero (toplevel->pending_counter_value))
	{
	  toplevel->current_counter_value = toplevel->pending_counter_value;
	  XSyncIntToValue (&toplevel->pending_counter_value, 0);
	}
#endif

      if (!window ||
	  xevent->xconfigure.event != xevent->xconfigure.window ||
          BDK_WINDOW_TYPE (window) == BDK_WINDOW_CHILD ||
          BDK_WINDOW_TYPE (window) == BDK_WINDOW_ROOT)
	return_val = FALSE;
      else
	{
	  event->configure.type = BDK_CONFIGURE;
	  event->configure.window = window;
	  event->configure.width = xevent->xconfigure.width;
	  event->configure.height = xevent->xconfigure.height;
	  
	  if (!xevent->xconfigure.send_event &&
	      !xevent->xconfigure.override_redirect &&
	      !BDK_WINDOW_DESTROYED (window))
	    {
	      gint tx = 0;
	      gint ty = 0;
	      Window child_window = 0;

	      bdk_error_trap_push ();
	      if (XTranslateCoordinates (BDK_DRAWABLE_XDISPLAY (window),
					 BDK_DRAWABLE_XID (window),
					 screen_x11->xroot_window,
					 0, 0,
					 &tx, &ty,
					 &child_window))
		{
		  event->configure.x = tx;
		  event->configure.y = ty;
		}
	      bdk_error_trap_pop ();
	    }
	  else
	    {
	      event->configure.x = xevent->xconfigure.x;
	      event->configure.y = xevent->xconfigure.y;
	    }
	  window_private->x = event->configure.x;
	  window_private->y = event->configure.y;
	  window_private->width = xevent->xconfigure.width;
	  window_private->height = xevent->xconfigure.height;
	  
	  _bdk_window_update_size (window);
	  _bdk_x11_drawable_update_size (window_private->impl);
	  
	  if (window_private->resize_count >= 1)
	    {
	      window_private->resize_count -= 1;

	      if (window_private->resize_count == 0)
		_bdk_moveresize_configure_done (display, window);
	    }
	}
      break;
      
    case PropertyNotify:
      BDK_NOTE (EVENTS,
		g_message ("property notify:\twindow: %ld, atom(%ld): %s%s%s",
			   xevent->xproperty.window,
			   xevent->xproperty.atom,
			   "\"",
			   bdk_x11_get_xatom_name_for_display (display, xevent->xproperty.atom),
			   "\""));

      if (window_private == NULL)
        {
	  return_val = FALSE;
          break;
        }

      /* We compare with the serial of the last time we mapped the
       * window to avoid refetching properties that we set ourselves
       */
      if (toplevel &&
	  xevent->xproperty.serial >= toplevel->map_serial)
	{
	  if (xevent->xproperty.atom == bdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_STATE"))
	    bdk_check_wm_state_changed (window);
	  
	  if (xevent->xproperty.atom == bdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_DESKTOP"))
	    bdk_check_wm_desktop_changed (window);
	}
      
      if (window_private->event_mask & BDK_PROPERTY_CHANGE_MASK) 
	{
	  event->property.type = BDK_PROPERTY_NOTIFY;
	  event->property.window = window;
	  event->property.atom = bdk_x11_xatom_to_atom_for_display (display, xevent->xproperty.atom);
	  event->property.time = xevent->xproperty.time;
	  event->property.state = xevent->xproperty.state;
	}
      else
	return_val = FALSE;

      break;
      
    case SelectionClear:
      BDK_NOTE (EVENTS,
		g_message ("selection clear:\twindow: %ld",
			   xevent->xproperty.window));

      if (_bdk_selection_filter_clear_event (&xevent->xselectionclear))
	{
	  event->selection.type = BDK_SELECTION_CLEAR;
	  event->selection.window = window;
	  event->selection.selection = bdk_x11_xatom_to_atom_for_display (display, xevent->xselectionclear.selection);
	  event->selection.time = xevent->xselectionclear.time;
	}
      else
	return_val = FALSE;
	  
      break;
      
    case SelectionRequest:
      BDK_NOTE (EVENTS,
		g_message ("selection request:\twindow: %ld",
			   xevent->xproperty.window));
      
      event->selection.type = BDK_SELECTION_REQUEST;
      event->selection.window = window;
      event->selection.selection = bdk_x11_xatom_to_atom_for_display (display, xevent->xselectionrequest.selection);
      event->selection.target = bdk_x11_xatom_to_atom_for_display (display, xevent->xselectionrequest.target);
      event->selection.property = bdk_x11_xatom_to_atom_for_display (display, xevent->xselectionrequest.property);
      event->selection.requestor = xevent->xselectionrequest.requestor;
      event->selection.time = xevent->xselectionrequest.time;
      
      break;
      
    case SelectionNotify:
      BDK_NOTE (EVENTS,
		g_message ("selection notify:\twindow: %ld",
			   xevent->xproperty.window));
      
      
      event->selection.type = BDK_SELECTION_NOTIFY;
      event->selection.window = window;
      event->selection.selection = bdk_x11_xatom_to_atom_for_display (display, xevent->xselection.selection);
      event->selection.target = bdk_x11_xatom_to_atom_for_display (display, xevent->xselection.target);
      if (xevent->xselection.property == None)
        event->selection.property = BDK_NONE;
      else
        event->selection.property = bdk_x11_xatom_to_atom_for_display (display, xevent->xselection.property);
      event->selection.time = xevent->xselection.time;
      
      break;
      
    case ColormapNotify:
      BDK_NOTE (EVENTS,
		g_message ("colormap notify:\twindow: %ld",
			   xevent->xcolormap.window));
      
      /* Not currently handled */
      return_val = FALSE;
      break;
      
    case ClientMessage:
      {
	GList *tmp_list;
	BdkFilterReturn result = BDK_FILTER_CONTINUE;
	BdkAtom message_type = bdk_x11_xatom_to_atom_for_display (display, xevent->xclient.message_type);

	BDK_NOTE (EVENTS,
		  g_message ("client message:\twindow: %ld",
			     xevent->xclient.window));
	
	tmp_list = display_x11->client_filters;
	while (tmp_list)
	  {
	    BdkClientFilter *filter = tmp_list->data;
	    tmp_list = tmp_list->next;
	    
	    if (filter->type == message_type)
	      {
		result = (*filter->function) (xevent, event, filter->data);
		if (result != BDK_FILTER_CONTINUE)
		  break;
	      }
	  }

	switch (result)
	  {
	  case BDK_FILTER_REMOVE:
	    return_val = FALSE;
	    break;
	  case BDK_FILTER_TRANSLATE:
	    return_val = TRUE;
	    break;
	  case BDK_FILTER_CONTINUE:
	    /* Send unknown ClientMessage's on to Btk for it to use */
            if (window_private == NULL)
              {
                return_val = FALSE;
              }
            else
              {
                event->client.type = BDK_CLIENT_EVENT;
                event->client.window = window;
                event->client.message_type = message_type;
                event->client.data_format = xevent->xclient.format;
                memcpy(&event->client.data, &xevent->xclient.data,
                       sizeof(event->client.data));
              }
            break;
          }
      }
      
      break;
      
    case MappingNotify:
      BDK_NOTE (EVENTS,
		g_message ("mapping notify"));
      
      /* Let XLib know that there is a new keyboard mapping.
       */
      XRefreshKeyboardMapping (&xevent->xmapping);
      _bdk_keymap_keys_changed (display);
      return_val = FALSE;
      break;

    default:
#ifdef HAVE_XKB
      if (xevent->type == display_x11->xkb_event_type)
	{
	  XkbEvent *xkb_event = (XkbEvent *)xevent;

	  switch (xkb_event->any.xkb_type)
	    {
	    case XkbNewKeyboardNotify:
	    case XkbMapNotify:
	      _bdk_keymap_keys_changed (display);

	      return_val = FALSE;
	      break;
	      
	    case XkbStateNotify:
	      _bdk_keymap_state_changed (display, xevent);
	      break;
	    }
	}
      else
#endif
#ifdef HAVE_XFIXES
      if (xevent->type - display_x11->xfixes_event_base == XFixesSelectionNotify)
	{
	  XFixesSelectionNotifyEvent *selection_notify = (XFixesSelectionNotifyEvent *)xevent;

	  _bdk_x11_screen_process_owner_change (screen, xevent);
	  
	  event->owner_change.type = BDK_OWNER_CHANGE;
	  event->owner_change.window = window;
	  event->owner_change.owner = selection_notify->owner;
	  event->owner_change.reason = selection_notify->subtype;
	  event->owner_change.selection = 
	    bdk_x11_xatom_to_atom_for_display (display, 
					       selection_notify->selection);
	  event->owner_change.time = selection_notify->timestamp;
	  event->owner_change.selection_time = selection_notify->selection_timestamp;
	  
	  return_val = TRUE;
	}
      else
#endif
#ifdef HAVE_RANDR
      if (xevent->type - display_x11->xrandr_event_base == RRScreenChangeNotify ||
          xevent->type - display_x11->xrandr_event_base == RRNotify)
	{
          if (screen)
            _bdk_x11_screen_size_changed (screen, xevent);
	}
      else
#endif
#if defined(HAVE_XCOMPOSITE) && defined (HAVE_XDAMAGE) && defined (HAVE_XFIXES)
      if (display_x11->have_xdamage && window_private && window_private->composited &&
	  xevent->type == display_x11->xdamage_event_base + XDamageNotify &&
	  ((XDamageNotifyEvent *) xevent)->damage == window_impl->damage)
	{
	  XDamageNotifyEvent *damage_event = (XDamageNotifyEvent *) xevent;
	  XserverRebunnyion repair;
	  BdkRectangle rect;

	  rect.x = window_private->x + damage_event->area.x;
	  rect.y = window_private->y + damage_event->area.y;
	  rect.width = damage_event->area.width;
	  rect.height = damage_event->area.height;

	  repair = XFixesCreateRebunnyion (display_x11->xdisplay,
				       &damage_event->area, 1);
	  XDamageSubtract (display_x11->xdisplay,
			   window_impl->damage,
			   repair, None);
	  XFixesDestroyRebunnyion (display_x11->xdisplay, repair);

	  if (window_private->parent != NULL)
	    _bdk_window_process_expose (BDK_WINDOW (window_private->parent),
					damage_event->serial, &rect);

	  return_val = TRUE;
	}
      else
#endif
	{
	  /* something else - (e.g., a Xinput event) */
	  
	  if (window_private &&
	      !BDK_WINDOW_DESTROYED (window_private) &&
	      window_private->input_window)
	    return_val = _bdk_input_other_event (event, xevent, window);
	  else
	    return_val = FALSE;
	  
	  break;
	}
    }

 done:
  if (return_val)
    {
      if (event->any.window)
	g_object_ref (event->any.window);
      if (((event->any.type == BDK_ENTER_NOTIFY) ||
	   (event->any.type == BDK_LEAVE_NOTIFY)) &&
	  (event->crossing.subwindow != NULL))
	g_object_ref (event->crossing.subwindow);
    }
  else
    {
      /* Mark this event as having no resources to be freed */
      event->any.window = NULL;
      event->any.type = BDK_NOTHING;
    }
  
  if (window)
    g_object_unref (window);

  return return_val;
}

static BdkFilterReturn
bdk_wm_protocols_filter (BdkXEvent *xev,
			 BdkEvent  *event,
			 gpointer data)
{
  XEvent *xevent = (XEvent *)xev;
  BdkWindow *win = event->any.window;
  BdkDisplay *display;
  Atom atom;

  if (!win)
      return BDK_FILTER_REMOVE;    

  display = BDK_WINDOW_DISPLAY (win);
  atom = (Atom)xevent->xclient.data.l[0];

  if (atom == bdk_x11_get_xatom_by_name_for_display (display, "WM_DELETE_WINDOW"))
    {
  /* The delete window request specifies a window
   *  to delete. We don't actually destroy the
   *  window because "it is only a request". (The
   *  window might contain vital data that the
   *  program does not want destroyed). Instead
   *  the event is passed along to the program,
   *  which should then destroy the window.
   */
      BDK_NOTE (EVENTS,
		g_message ("delete window:\t\twindow: %ld",
			   xevent->xclient.window));
      
      event->any.type = BDK_DELETE;

      bdk_x11_window_set_user_time (win, xevent->xclient.data.l[1]);

      return BDK_FILTER_TRANSLATE;
    }
  else if (atom == bdk_x11_get_xatom_by_name_for_display (display, "WM_TAKE_FOCUS"))
    {
      BdkToplevelX11 *toplevel = _bdk_x11_window_get_toplevel (event->any.window);
      BdkWindowObject *private = (BdkWindowObject *)win;

      /* There is no way of knowing reliably whether we are viewable;
       * _bdk_x11_set_input_focus_safe() traps errors asynchronously.
       */
      if (toplevel && private->accept_focus)
	_bdk_x11_set_input_focus_safe (display, toplevel->focus_window,
				       RevertToParent,
				       xevent->xclient.data.l[1]);

      return BDK_FILTER_REMOVE;
    }
  else if (atom == bdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_PING") &&
	   !_bdk_x11_display_is_root_window (display,
					     xevent->xclient.window))
    {
      XClientMessageEvent xclient = xevent->xclient;
      
      xclient.window = BDK_WINDOW_XROOTWIN (win);
      XSendEvent (BDK_WINDOW_XDISPLAY (win), 
		  xclient.window,
		  False, 
		  SubstructureRedirectMask | SubstructureNotifyMask, (XEvent *)&xclient);

      return BDK_FILTER_REMOVE;
    }
  else if (atom == bdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_SYNC_REQUEST") &&
	   BDK_DISPLAY_X11 (display)->use_sync)
    {
      BdkToplevelX11 *toplevel = _bdk_x11_window_get_toplevel (event->any.window);
      if (toplevel)
	{
#ifdef HAVE_XSYNC
	  XSyncIntsToValue (&toplevel->pending_counter_value, 
			    xevent->xclient.data.l[2], 
			    xevent->xclient.data.l[3]);
#endif
	}
      return BDK_FILTER_REMOVE;
    }
  
  return BDK_FILTER_CONTINUE;
}

void
_bdk_events_queue (BdkDisplay *display)
{
  GList *node;
  BdkEvent *event;
  XEvent xevent;
  Display *xdisplay = BDK_DISPLAY_XDISPLAY (display);

  while (!_bdk_event_queue_find_first(display) && XPending (xdisplay))
    {
      XNextEvent (xdisplay, &xevent);

      switch (xevent.type)
	{
	case KeyPress:
	case KeyRelease:
	  break;
	default:
	  if (XFilterEvent (&xevent, None))
	    continue;
	}
      
      event = bdk_event_new (BDK_NOTHING);
      
      event->any.window = NULL;
      event->any.send_event = xevent.xany.send_event ? TRUE : FALSE;

      ((BdkEventPrivate *)event)->flags |= BDK_EVENT_PENDING;

      node = _bdk_event_queue_append (display, event);

      if (bdk_event_translate (display, event, &xevent, FALSE))
	{
	  ((BdkEventPrivate *)event)->flags &= ~BDK_EVENT_PENDING;
          _bdk_windowing_got_event (display, node, event, xevent.xany.serial);
	}
      else
	{
	  _bdk_event_queue_remove_link (display, node);
	  g_list_free_1 (node);
	  bdk_event_free (event);
	}
    }
}

static gboolean  
bdk_event_prepare (GSource  *source,
		   gint     *timeout)
{
  BdkDisplay *display = ((BdkDisplaySource*)source)->display;
  gboolean retval;
  
  BDK_THREADS_ENTER ();

  *timeout = -1;
  retval = (_bdk_event_queue_find_first (display) != NULL || 
	    bdk_check_xpending (display));
  
  BDK_THREADS_LEAVE ();

  return retval;
}

static gboolean  
bdk_event_check (GSource *source) 
{
  BdkDisplaySource *display_source = (BdkDisplaySource*)source;
  gboolean retval;

  BDK_THREADS_ENTER ();

  if (display_source->event_poll_fd.revents & G_IO_IN)
    retval = (_bdk_event_queue_find_first (display_source->display) != NULL || 
	      bdk_check_xpending (display_source->display));
  else
    retval = FALSE;

  BDK_THREADS_LEAVE ();

  return retval;
}

static gboolean  
bdk_event_dispatch (GSource    *source,
		    GSourceFunc callback,
		    gpointer    user_data)
{
  BdkDisplay *display = ((BdkDisplaySource*)source)->display;
  BdkEvent *event;
 
  BDK_THREADS_ENTER ();

  _bdk_events_queue (display);
  event = _bdk_event_unqueue (display);

  if (event)
    {
      if (_bdk_event_func)
	(*_bdk_event_func) (event, _bdk_event_data);
      
      bdk_event_free (event);
    }
  
  BDK_THREADS_LEAVE ();

  return TRUE;
}

/**
 * bdk_event_send_client_message_for_display:
 * @display: the #BdkDisplay for the window where the message is to be sent.
 * @event: the #BdkEvent to send, which should be a #BdkEventClient.
 * @winid: the window to send the client message to.
 *
 * On X11, sends an X ClientMessage event to a given window. On
 * Windows, sends a message registered with the name
 * BDK_WIN32_CLIENT_MESSAGE.
 *
 * This could be used for communicating between different
 * applications, though the amount of data is limited to 20 bytes on
 * X11, and to just four bytes on Windows.
 *
 * Returns: non-zero on success.
 *
 * Since: 2.2
 */
gboolean
bdk_event_send_client_message_for_display (BdkDisplay     *display,
					   BdkEvent       *event,
					   BdkNativeWindow winid)
{
  XEvent sev;
  
  g_return_val_if_fail(event != NULL, FALSE);

  /* Set up our event to send, with the exception of its target window */
  sev.xclient.type = ClientMessage;
  sev.xclient.display = BDK_DISPLAY_XDISPLAY (display);
  sev.xclient.format = event->client.data_format;
  sev.xclient.window = winid;
  memcpy(&sev.xclient.data, &event->client.data, sizeof (sev.xclient.data));
  sev.xclient.message_type = bdk_x11_atom_to_xatom_for_display (display, event->client.message_type);
  
  return _bdk_send_xevent (display, winid, False, NoEventMask, &sev);
}



/* Sends a ClientMessage to all toplevel client windows */
static gboolean
bdk_event_send_client_message_to_all_recurse (BdkDisplay *display,
					      XEvent     *xev, 
					      guint32     xid,
					      guint       level)
{
  Atom type = None;
  int format;
  unsigned long nitems, after;
  unsigned char *data;
  Window *ret_children, ret_root, ret_parent;
  unsigned int ret_nchildren;
  gboolean send = FALSE;
  gboolean found = FALSE;
  gboolean result = FALSE;
  int i;
  
  bdk_error_trap_push ();
  
  if (XGetWindowProperty (BDK_DISPLAY_XDISPLAY (display), xid, 
			  bdk_x11_get_xatom_by_name_for_display (display, "WM_STATE"),
			  0, 0, False, AnyPropertyType,
			  &type, &format, &nitems, &after, &data) != Success)
    goto out;
  
  if (type)
    {
      send = TRUE;
      XFree (data);
    }
  else
    {
      /* OK, we're all set, now let's find some windows to send this to */
      if (!XQueryTree (BDK_DISPLAY_XDISPLAY (display), xid,
		      &ret_root, &ret_parent,
		      &ret_children, &ret_nchildren))	
	goto out;

      for(i = 0; i < ret_nchildren; i++)
	if (bdk_event_send_client_message_to_all_recurse (display, xev, ret_children[i], level + 1))
	  found = TRUE;

      XFree (ret_children);
    }

  if (send || (!found && (level == 1)))
    {
      xev->xclient.window = xid;
      _bdk_send_xevent (display, xid, False, NoEventMask, xev);
    }

  result = send || found;

 out:
  bdk_error_trap_pop ();

  return result;
}

/**
 * bdk_screen_broadcast_client_message:
 * @screen: the #BdkScreen where the event will be broadcasted.
 * @event: the #BdkEvent.
 *
 * On X11, sends an X ClientMessage event to all toplevel windows on
 * @screen. 
 *
 * Toplevel windows are determined by checking for the WM_STATE property, 
 * as described in the Inter-Client Communication Conventions Manual (ICCCM).
 * If no windows are found with the WM_STATE property set, the message is 
 * sent to all children of the root window.
 *
 * On Windows, broadcasts a message registered with the name
 * BDK_WIN32_CLIENT_MESSAGE to all top-level windows. The amount of
 * data is limited to one long, i.e. four bytes.
 *
 * Since: 2.2
 */

void
bdk_screen_broadcast_client_message (BdkScreen *screen, 
				     BdkEvent  *event)
{
  XEvent sev;
  BdkWindow *root_window;

  g_return_if_fail (event != NULL);
  
  root_window = bdk_screen_get_root_window (screen);
  
  /* Set up our event to send, with the exception of its target window */
  sev.xclient.type = ClientMessage;
  sev.xclient.display = BDK_WINDOW_XDISPLAY (root_window);
  sev.xclient.format = event->client.data_format;
  memcpy(&sev.xclient.data, &event->client.data, sizeof (sev.xclient.data));
  sev.xclient.message_type = 
    bdk_x11_atom_to_xatom_for_display (BDK_WINDOW_DISPLAY (root_window),
				       event->client.message_type);

  bdk_event_send_client_message_to_all_recurse (bdk_screen_get_display (screen),
						&sev, 
						BDK_WINDOW_XID (root_window), 
						0);
}

/*
 *--------------------------------------------------------------
 * bdk_flush
 *
 *   Flushes the Xlib output buffer and then waits
 *   until all requests have been received and processed
 *   by the X server. The only real use for this function
 *   is in dealing with XShm.
 *
 * Arguments:
 *
 * Results:
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

void
bdk_flush (void)
{
  GSList *tmp_list = _bdk_displays;
  
  while (tmp_list)
    {
      XSync (BDK_DISPLAY_XDISPLAY (tmp_list->data), False);
      tmp_list = tmp_list->next;
    }
}

static Bool
timestamp_predicate (Display *display,
		     XEvent  *xevent,
		     XPointer arg)
{
  Window xwindow = GPOINTER_TO_UINT (arg);
  BdkDisplay *bdk_display = bdk_x11_lookup_xdisplay (display);

  if (xevent->type == PropertyNotify &&
      xevent->xproperty.window == xwindow &&
      xevent->xproperty.atom == bdk_x11_get_xatom_by_name_for_display (bdk_display,
								       "BDK_TIMESTAMP_PROP"))
    return True;

  return False;
}

/**
 * bdk_x11_get_server_time:
 * @window: a #BdkWindow, used for communication with the server.
 *          The window must have BDK_PROPERTY_CHANGE_MASK in its
 *          events mask or a hang will result.
 * 
 * Routine to get the current X server time stamp. 
 * 
 * Return value: the time stamp.
 **/
guint32
bdk_x11_get_server_time (BdkWindow *window)
{
  Display *xdisplay;
  Window   xwindow;
  guchar c = 'a';
  XEvent xevent;
  Atom timestamp_prop_atom;

  g_return_val_if_fail (BDK_IS_WINDOW (window), 0);
  g_return_val_if_fail (!BDK_WINDOW_DESTROYED (window), 0);

  xdisplay = BDK_WINDOW_XDISPLAY (window);
  xwindow = BDK_WINDOW_XWINDOW (window);
  timestamp_prop_atom = 
    bdk_x11_get_xatom_by_name_for_display (BDK_WINDOW_DISPLAY (window),
					   "BDK_TIMESTAMP_PROP");
  
  XChangeProperty (xdisplay, xwindow, timestamp_prop_atom,
		   timestamp_prop_atom,
		   8, PropModeReplace, &c, 1);

  XIfEvent (xdisplay, &xevent,
	    timestamp_predicate, GUINT_TO_POINTER(xwindow));

  return xevent.xproperty.time;
}

static void
fetch_net_wm_check_window (BdkScreen *screen)
{
  BdkScreenX11 *screen_x11;
  BdkDisplay *display;
  Atom type;
  gint format;
  gulong n_items;
  gulong bytes_after;
  guchar *data;
  Window *xwindow;
  GTimeVal tv;
  gint error;

  screen_x11 = BDK_SCREEN_X11 (screen);
  display = screen_x11->display;

  g_return_if_fail (BDK_DISPLAY_X11 (display)->trusted_client);
  
  g_get_current_time (&tv);

  if (ABS  (tv.tv_sec - screen_x11->last_wmspec_check_time) < 15)
    return; /* we've checked recently */

  screen_x11->last_wmspec_check_time = tv.tv_sec;

  data = NULL;
  XGetWindowProperty (screen_x11->xdisplay, screen_x11->xroot_window,
		      bdk_x11_get_xatom_by_name_for_display (display, "_NET_SUPPORTING_WM_CHECK"),
		      0, G_MAXLONG, False, XA_WINDOW, &type, &format,
		      &n_items, &bytes_after, &data);
  
  if (type != XA_WINDOW)
    {
      if (data)
        XFree (data);
      return;
    }

  xwindow = (Window *)data;

  if (screen_x11->wmspec_check_window == *xwindow)
    {
      XFree (xwindow);
      return;
    }

  bdk_error_trap_push ();

  /* Find out if this WM goes away, so we can reset everything. */
  XSelectInput (screen_x11->xdisplay, *xwindow, StructureNotifyMask);
  bdk_display_sync (display);

  error = bdk_error_trap_pop ();
  if (!error)
    {
      screen_x11->wmspec_check_window = *xwindow;
      screen_x11->need_refetch_net_supported = TRUE;
      screen_x11->need_refetch_wm_name = TRUE;

      /* Careful, reentrancy */
      _bdk_x11_screen_window_manager_changed (BDK_SCREEN (screen_x11));
    }
  else if (error == BadWindow)
    {
      /* Leftover property, try again immediately, new wm may be starting up */
      screen_x11->last_wmspec_check_time = 0;
    }

  XFree (xwindow);
}

/**
 * bdk_x11_screen_get_window_manager_name:
 * @screen: a #BdkScreen 
 * 
 * Returns the name of the window manager for @screen. 
 * 
 * Return value: the name of the window manager screen @screen, or 
 * "unknown" if the window manager is unknown. The string is owned by BDK
 * and should not be freed.
 *
 * Since: 2.2
 **/
const char*
bdk_x11_screen_get_window_manager_name (BdkScreen *screen)
{
  BdkScreenX11 *screen_x11;

  screen_x11 = BDK_SCREEN_X11 (screen);
  
  if (!G_LIKELY (BDK_DISPLAY_X11 (screen_x11->display)->trusted_client))
    return screen_x11->window_manager_name;

  fetch_net_wm_check_window (screen);

  if (screen_x11->need_refetch_wm_name)
    {
      /* Get the name of the window manager */
      screen_x11->need_refetch_wm_name = FALSE;

      g_free (screen_x11->window_manager_name);
      screen_x11->window_manager_name = g_strdup ("unknown");
      
      if (screen_x11->wmspec_check_window != None)
        {
          Atom type;
          gint format;
          gulong n_items;
          gulong bytes_after;
          gchar *name;
          
          name = NULL;

	  bdk_error_trap_push ();
          
          XGetWindowProperty (BDK_DISPLAY_XDISPLAY (screen_x11->display),
                              screen_x11->wmspec_check_window,
                              bdk_x11_get_xatom_by_name_for_display (screen_x11->display,
                                                                     "_NET_WM_NAME"),
                              0, G_MAXLONG, False,
                              bdk_x11_get_xatom_by_name_for_display (screen_x11->display,
                                                                     "UTF8_STRING"),
                              &type, &format, 
                              &n_items, &bytes_after,
                              (guchar **)&name);
          
          bdk_display_sync (screen_x11->display);
          
          bdk_error_trap_pop ();
          
          if (name != NULL)
            {
              g_free (screen_x11->window_manager_name);
              screen_x11->window_manager_name = g_strdup (name);
              XFree (name);
            }
        }
    }
  
  return BDK_SCREEN_X11 (screen)->window_manager_name;
}

typedef struct _NetWmSupportedAtoms NetWmSupportedAtoms;

struct _NetWmSupportedAtoms
{
  Atom *atoms;
  gulong n_atoms;
};

static void
cleanup_atoms(gpointer data)
{
  NetWmSupportedAtoms *supported_atoms = data;
  if (supported_atoms->atoms)
      XFree (supported_atoms->atoms);
  g_free (supported_atoms);
}

/**
 * bdk_x11_screen_supports_net_wm_hint:
 * @screen: the relevant #BdkScreen.
 * @property: a property atom.
 * 
 * This function is specific to the X11 backend of BDK, and indicates
 * whether the window manager supports a certain hint from the
 * Extended Window Manager Hints Specification. You can find this
 * specification on 
 * <ulink url="http://www.freedesktop.org">http://www.freedesktop.org</ulink>.
 *
 * When using this function, keep in mind that the window manager
 * can change over time; so you shouldn't use this function in
 * a way that impacts persistent application state. A common bug
 * is that your application can start up before the window manager
 * does when the user logs in, and before the window manager starts
 * bdk_x11_screen_supports_net_wm_hint() will return %FALSE for every property.
 * You can monitor the window_manager_changed signal on #BdkScreen to detect
 * a window manager change.
 * 
 * Return value: %TRUE if the window manager supports @property
 *
 * Since: 2.2
 **/
gboolean
bdk_x11_screen_supports_net_wm_hint (BdkScreen *screen,
				     BdkAtom    property)
{
  gulong i;
  BdkScreenX11 *screen_x11;
  NetWmSupportedAtoms *supported_atoms;
  BdkDisplay *display;

  g_return_val_if_fail (BDK_IS_SCREEN (screen), FALSE);
  
  screen_x11 = BDK_SCREEN_X11 (screen);
  display = screen_x11->display;

  if (!G_LIKELY (BDK_DISPLAY_X11 (display)->trusted_client))
    return FALSE;

  supported_atoms = g_object_get_data (G_OBJECT (screen), "bdk-net-wm-supported-atoms");
  if (!supported_atoms)
    {
      supported_atoms = g_new0 (NetWmSupportedAtoms, 1);
      g_object_set_data_full (G_OBJECT (screen), "bdk-net-wm-supported-atoms", supported_atoms, cleanup_atoms);
    }

  fetch_net_wm_check_window (screen);

  if (screen_x11->wmspec_check_window == None)
    return FALSE;
  
  if (screen_x11->need_refetch_net_supported)
    {
      /* WM has changed since we last got the supported list,
       * refetch it.
       */
      Atom type;
      gint format;
      gulong bytes_after;

      screen_x11->need_refetch_net_supported = FALSE;
      
      if (supported_atoms->atoms)
        XFree (supported_atoms->atoms);
      
      supported_atoms->atoms = NULL;
      supported_atoms->n_atoms = 0;
      
      XGetWindowProperty (BDK_DISPLAY_XDISPLAY (display), screen_x11->xroot_window,
                          bdk_x11_get_xatom_by_name_for_display (display, "_NET_SUPPORTED"),
                          0, G_MAXLONG, False, XA_ATOM, &type, &format, 
                          &supported_atoms->n_atoms, &bytes_after,
                          (guchar **)&supported_atoms->atoms);
      
      if (type != XA_ATOM)
        return FALSE;
    }
  
  if (supported_atoms->atoms == NULL)
    return FALSE;
  
  i = 0;
  while (i < supported_atoms->n_atoms)
    {
      if (supported_atoms->atoms[i] == bdk_x11_atom_to_xatom_for_display (display, property))
        return TRUE;
      
      ++i;
    }
  
  return FALSE;
}

/**
 * bdk_net_wm_supports:
 * @property: a property atom.
 * 
 * This function is specific to the X11 backend of BDK, and indicates
 * whether the window manager for the default screen supports a certain
 * hint from the Extended Window Manager Hints Specification. See
 * bdk_x11_screen_supports_net_wm_hint() for complete details.
 * 
 * Return value: %TRUE if the window manager supports @property
 *
 * Deprecated:2.24: Use bdk_x11_screen_supports_net_wm_hint() instead
 **/
gboolean
bdk_net_wm_supports (BdkAtom property)
{
  return bdk_x11_screen_supports_net_wm_hint (bdk_screen_get_default (), property);
}


static void
bdk_xsettings_notify_cb (const char       *name,
			 XSettingsAction   action,
			 XSettingsSetting *setting,
			 void             *data)
{
  BdkEvent new_event;
  BdkScreen *screen = data;
  BdkScreenX11 *screen_x11 = data;
  int i;

  if (screen_x11->xsettings_in_init)
    return;
  
  new_event.type = BDK_SETTING;
  new_event.setting.window = bdk_screen_get_root_window (screen);
  new_event.setting.send_event = FALSE;
  new_event.setting.name = NULL;

  for (i = 0; i < BDK_SETTINGS_N_ELEMENTS() ; i++)
    if (strcmp (BDK_SETTINGS_X_NAME (i), name) == 0)
      {
	new_event.setting.name = (char*) BDK_SETTINGS_BDK_NAME (i);
	break;
      }
  
  if (!new_event.setting.name)
    return;
  
  switch (action)
    {
    case XSETTINGS_ACTION_NEW:
      new_event.setting.action = BDK_SETTING_ACTION_NEW;
      break;
    case XSETTINGS_ACTION_CHANGED:
      new_event.setting.action = BDK_SETTING_ACTION_CHANGED;
      break;
    case XSETTINGS_ACTION_DELETED:
      new_event.setting.action = BDK_SETTING_ACTION_DELETED;
      break;
    }

  bdk_event_put (&new_event);
}

static gboolean
check_transform (const gchar *xsettings_name,
		 GType        src_type,
		 GType        dest_type)
{
  if (!g_value_type_transformable (src_type, dest_type))
    {
      g_warning ("Cannot transform xsetting %s of type %s to type %s\n",
		 xsettings_name,
		 g_type_name (src_type),
		 g_type_name (dest_type));
      return FALSE;
    }
  else
    return TRUE;
}

/**
 * bdk_screen_get_setting:
 * @screen: the #BdkScreen where the setting is located
 * @name: the name of the setting
 * @value: location to store the value of the setting
 *
 * Retrieves a desktop-wide setting such as double-click time
 * for the #BdkScreen @screen. 
 *
 * FIXME needs a list of valid settings here, or a link to 
 * more information.
 * 
 * Returns: %TRUE if the setting existed and a value was stored
 *   in @value, %FALSE otherwise.
 *
 * Since: 2.2
 **/
gboolean
bdk_screen_get_setting (BdkScreen   *screen,
			const gchar *name,
			GValue      *value)
{

  const char *xsettings_name = NULL;
  XSettingsResult result;
  XSettingsSetting *setting = NULL;
  BdkScreenX11 *screen_x11;
  gboolean success = FALSE;
  gint i;
  GValue tmp_val = { 0, };
  
  g_return_val_if_fail (BDK_IS_SCREEN (screen), FALSE);
  
  screen_x11 = BDK_SCREEN_X11 (screen);

  for (i = 0; i < BDK_SETTINGS_N_ELEMENTS(); i++)
    if (strcmp (BDK_SETTINGS_BDK_NAME (i), name) == 0)
      {
	xsettings_name = BDK_SETTINGS_X_NAME (i);
	break;
      }

  if (!xsettings_name)
    goto out;

  result = xsettings_client_get_setting (screen_x11->xsettings_client, 
					 xsettings_name, &setting);
  if (result != XSETTINGS_SUCCESS)
    goto out;

  switch (setting->type)
    {
    case XSETTINGS_TYPE_INT:
      if (check_transform (xsettings_name, G_TYPE_INT, G_VALUE_TYPE (value)))
	{
	  g_value_init (&tmp_val, G_TYPE_INT);
	  g_value_set_int (&tmp_val, setting->data.v_int);
	  g_value_transform (&tmp_val, value);

	  success = TRUE;
	}
      break;
    case XSETTINGS_TYPE_STRING:
      if (check_transform (xsettings_name, G_TYPE_STRING, G_VALUE_TYPE (value)))
	{
	  g_value_init (&tmp_val, G_TYPE_STRING);
	  g_value_set_string (&tmp_val, setting->data.v_string);
	  g_value_transform (&tmp_val, value);

	  success = TRUE;
	}
      break;
    case XSETTINGS_TYPE_COLOR:
      if (!check_transform (xsettings_name, BDK_TYPE_COLOR, G_VALUE_TYPE (value)))
	{
	  BdkColor color;
	  
	  g_value_init (&tmp_val, BDK_TYPE_COLOR);

	  color.pixel = 0;
	  color.red = setting->data.v_color.red;
	  color.green = setting->data.v_color.green;
	  color.blue = setting->data.v_color.blue;
	  
	  g_value_set_boxed (&tmp_val, &color);
	  
	  g_value_transform (&tmp_val, value);
	  
	  success = TRUE;
	}
      break;
    }
  
  g_value_unset (&tmp_val);

 out:
  if (setting)
    xsettings_setting_free (setting);

  if (success)
    return TRUE;
  else
    return _bdk_x11_get_xft_setting (screen, name, value);
}

static BdkFilterReturn 
bdk_xsettings_client_event_filter (BdkXEvent *xevent,
				   BdkEvent  *event,
				   gpointer   data)
{
  BdkScreenX11 *screen = data;
  
  if (xsettings_client_process_event (screen->xsettings_client, (XEvent *)xevent))
    return BDK_FILTER_REMOVE;
  else
    return BDK_FILTER_CONTINUE;
}

static Bool
bdk_xsettings_watch_cb (Window   window,
			Bool	 is_start,
			long     mask,
			void    *cb_data)
{
  BdkWindow *bdkwin;
  BdkScreen *screen = cb_data;

  bdkwin = bdk_window_lookup_for_display (bdk_screen_get_display (screen), window);

  if (is_start)
    {
      if (bdkwin)
	g_object_ref (bdkwin);
      else
	{
	  bdkwin = bdk_window_foreign_new_for_display (bdk_screen_get_display (screen), window);
	  
	  /* bdk_window_foreign_new_for_display() can fail and return NULL if the
	   * window has already been destroyed.
	   */
	  if (!bdkwin)
	    return False;
	}

      bdk_window_add_filter (bdkwin, bdk_xsettings_client_event_filter, screen);
    }
  else
    {
      if (!bdkwin)
	{
	  /* bdkwin should not be NULL here, since if starting the watch succeeded
	   * we have a reference on the window. It might mean that the caller didn't
	   * remove the watch when it got a DestroyNotify event. Or maybe the
	   * caller ignored the return value when starting the watch failed.
	   */
	  g_warning ("bdk_xsettings_watch_cb(): Couldn't find window to unwatch");
	  return False;
	}
      
      bdk_window_remove_filter (bdkwin, bdk_xsettings_client_event_filter, screen);
      g_object_unref (bdkwin);
    }

  return True;
}

void
_bdk_windowing_event_data_copy (const BdkEvent *src,
                                BdkEvent       *dst)
{
}

void
_bdk_windowing_event_data_free (BdkEvent *event)
{
}

#define __BDK_EVENTS_X11_C__
#include "bdkaliasdef.c"
