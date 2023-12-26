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

#include "config.h"

#include <bunnylib.h>
#include "bdk.h"
#include "bdkdirectfb.h"
#include "bdkprivate-directfb.h"
#include "bdkscreen.h"
#include "bdkdisplaymanager.h"
#include "bdkalias.h"


extern void _bdk_visual_init            (void);
extern void _bdk_events_init            (void);
extern void _bdk_input_init             (void);
extern void _bdk_dnd_init               (void);
extern void _bdk_windowing_window_init  (BdkScreen *screen);
extern void _bdk_windowing_image_init   (void);
extern void _bdk_directfb_keyboard_init (void);

static bboolean   bdk_directfb_argb_font           = FALSE;
static bint       bdk_directfb_glyph_surface_cache = 8;


const GOptionEntry _bdk_windowing_args[] =
  {
    { "disable-aa-fonts",0,0,G_OPTION_ARG_INT,&bdk_directfb_monochrome_fonts,NULL,NULL    },
    { "argb-font",0,0, G_OPTION_ARG_INT, &bdk_directfb_argb_font,NULL,NULL},
    { "transparent-unfocused",0,0, G_OPTION_ARG_INT, &bdk_directfb_apply_focus_opacity,NULL,NULL },
    { "glyph-surface-cache",0,0,G_OPTION_ARG_INT,&bdk_directfb_glyph_surface_cache,NULL,NULL },
    { "enable-color-keying",0,0,G_OPTION_ARG_INT,&bdk_directfb_enable_color_keying,NULL,NULL },
    { NULL }
  };

/* Main entry point for bdk in 2.6 args are parsed
 */
BdkDisplay *
bdk_display_open (const bchar *display_name)
{
  IDirectFB             *directfb;
  IDirectFBDisplayLayer *layer;
  IDirectFBInputDevice  *keyboard;
  DFBResult              ret;

  if (_bdk_display)
    {
      return BDK_DISPLAY_OBJECT (_bdk_display); /* single display only */
    }

  ret = DirectFBInit (NULL, NULL);
  if (ret != DFB_OK)
    {
      DirectFBError ("bdk_display_open: DirectFBInit", ret);
      return NULL;
    }

  ret = DirectFBCreate (&directfb);
  if (ret != DFB_OK)
    {
      DirectFBError ("bdk_display_open: DirectFBCreate", ret);
      return NULL;
    }

  _bdk_display = g_object_new (BDK_TYPE_DISPLAY_DFB, NULL);
  _bdk_display->directfb = directfb;

  ret = directfb->GetDisplayLayer (directfb, DLID_PRIMARY, &layer);
  if (ret != DFB_OK)
    {
      DirectFBError ("bdk_display_open: GetDisplayLayer", ret);
      directfb->Release (directfb);
      _bdk_display->directfb = NULL;
      return NULL;
    }


  ret = directfb->GetInputDevice (directfb, DIDID_KEYBOARD, &keyboard);
  if (ret != DFB_OK)
    {
      DirectFBError ("bdk_display_open: GetInputDevice", ret);
      directfb->Release (directfb);
      _bdk_display->directfb = NULL;
      return NULL;
    }

  _bdk_display->layer    = layer;
  _bdk_display->keyboard = keyboard;

  _bdk_directfb_keyboard_init ();

  _bdk_screen = g_object_new (BDK_TYPE_SCREEN, NULL);

  _bdk_visual_init ();
  _bdk_windowing_window_init (_bdk_screen);

  bdk_screen_set_default_colormap (_bdk_screen,
                                   bdk_screen_get_system_colormap (_bdk_screen));
  _bdk_windowing_image_init ();

  _bdk_events_init ();
  _bdk_input_init ();
  _bdk_dnd_init ();

  layer->EnableCursor (layer, 1);

  g_signal_emit_by_name (bdk_display_manager_get (),
			 "display_opened", _bdk_display);

  return BDK_DISPLAY_OBJECT (_bdk_display);
}

GType
bdk_display_dfb_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      const GTypeInfo object_info =
        {
          sizeof (BdkDisplayDFBClass),
          (GBaseInitFunc) NULL,
          (GBaseFinalizeFunc) NULL,
          (GClassInitFunc) NULL,
          NULL,                 /* class_finalize */
          NULL,                 /* class_data */
          sizeof (BdkDisplayDFB),
          0,                    /* n_preallocs */
          (GInstanceInitFunc) NULL,
        };

      object_type = g_type_register_static (BDK_TYPE_DISPLAY,
                                            "BdkDisplayDFB",
                                            &object_info, 0);
    }

  return object_type;
}

IDirectFBSurface *
bdk_display_dfb_create_surface (BdkDisplayDFB *display, int format, int width, int height)
{
  DFBResult              ret;
  IDirectFBSurface      *temp;
  DFBSurfaceDescription  dsc;
  dsc.flags       = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
  dsc.width       = width;
  dsc.height      = height;
  dsc.pixelformat = format;
  ret             = display->directfb->CreateSurface (display->directfb, &dsc, &temp);
  if (ret)
    {
      DirectFBError ("bdk_display_dfb_create_surface ", ret);
      return NULL;
    }
  return temp;

}


/*************************************************************************************************
 * Displays and Screens
 */

void
_bdk_windowing_set_default_display (BdkDisplay *display)
{
  _bdk_display = BDK_DISPLAY_DFB (display);
}

const bchar *
bdk_display_get_name (BdkDisplay *display)
{
  return bdk_get_display_arg_name ();
}

int
bdk_display_get_n_screens (BdkDisplay *display)
{
  return 1;
}

BdkScreen *
bdk_display_get_screen (BdkDisplay *display,
			bint        screen_num)
{
  return _bdk_screen;
}

BdkScreen *
bdk_display_get_default_screen (BdkDisplay *display)
{
  return _bdk_screen;
}

bboolean
bdk_display_supports_shapes (BdkDisplay *display)
{
  return FALSE;
}

bboolean
bdk_display_supports_input_shapes (BdkDisplay *display)
{
  return FALSE;
}


BdkWindow *bdk_display_get_default_group (BdkDisplay *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);
  return  _bdk_parent_root;
}


/*************************************************************************************************
 * Selection and Clipboard
 */

bboolean
bdk_display_supports_selection_notification (BdkDisplay *display)
{
  return FALSE;
}

bboolean bdk_display_request_selection_notification  (BdkDisplay *display,
                                                      BdkAtom     selection)

{
  g_warning("bdk_display_request_selection_notification Unimplemented function \n");
  return FALSE;
}

bboolean
bdk_display_supports_clipboard_persistence (BdkDisplay *display)
{
  g_warning("bdk_display_supports_clipboard_persistence Unimplemented function \n");
  return FALSE;
}

void
bdk_display_store_clipboard (BdkDisplay    *display,
                             BdkWindow     *clipboard_window,
                             buint32        time_,
                             const BdkAtom *targets,
                             bint           n_targets)
{

  g_warning("bdk_display_store_clipboard Unimplemented function \n");

}


/*************************************************************************************************
 * Pointer
 */

static bboolean _bdk_directfb_pointer_implicit_grab = FALSE;

BdkGrabStatus
bdk_directfb_pointer_grab (BdkWindow    *window,
                           bint          owner_events,
                           BdkEventMask  event_mask,
                           BdkWindow    *confine_to,
                           BdkCursor    *cursor,
                           buint32       time,
                           bboolean      implicit_grab)
{
  BdkWindow             *toplevel;
  BdkWindowImplDirectFB *impl;

  if (_bdk_directfb_pointer_grab_window)
    {
      if (implicit_grab && !_bdk_directfb_pointer_implicit_grab)
        return BDK_GRAB_ALREADY_GRABBED;

      bdk_pointer_ungrab (time);
    }

  toplevel = bdk_directfb_window_find_toplevel (window);
  impl = BDK_WINDOW_IMPL_DIRECTFB (BDK_WINDOW_OBJECT (toplevel)->impl);

  if (impl->window)
    {
      if (impl->window->GrabPointer (impl->window) == DFB_LOCKED)
        return BDK_GRAB_ALREADY_GRABBED;
    }

  if (event_mask & BDK_BUTTON_MOTION_MASK)
    event_mask |= (BDK_BUTTON1_MOTION_MASK |
                   BDK_BUTTON2_MOTION_MASK |
                   BDK_BUTTON3_MOTION_MASK);

  _bdk_directfb_pointer_implicit_grab     = implicit_grab;
  _bdk_directfb_pointer_grab_window       = g_object_ref (window);
  _bdk_directfb_pointer_grab_owner_events = owner_events;

  _bdk_directfb_pointer_grab_confine      = (confine_to ?
                                             g_object_ref (confine_to) : NULL);
  _bdk_directfb_pointer_grab_events       = event_mask;
  _bdk_directfb_pointer_grab_cursor       = (cursor ?
                                             bdk_cursor_ref (cursor) : NULL);


  bdk_directfb_window_send_crossing_events (NULL,
                                            window,
                                            BDK_CROSSING_GRAB);

  return BDK_GRAB_SUCCESS;
}

void
bdk_directfb_pointer_ungrab (buint32  time,
                             bboolean implicit_grab)
{
  BdkWindow             *toplevel;
  BdkWindow             *mousewin;
  BdkWindow             *old_grab_window;
  BdkWindowImplDirectFB *impl;

  if (implicit_grab && !_bdk_directfb_pointer_implicit_grab)
    return;

  if (!_bdk_directfb_pointer_grab_window)
    return;

  toplevel =
    bdk_directfb_window_find_toplevel (_bdk_directfb_pointer_grab_window);
  impl = BDK_WINDOW_IMPL_DIRECTFB (BDK_WINDOW_OBJECT (toplevel)->impl);

  if (impl->window)
    impl->window->UngrabPointer (impl->window);

  if (_bdk_directfb_pointer_grab_confine)
    {
      g_object_unref (_bdk_directfb_pointer_grab_confine);
      _bdk_directfb_pointer_grab_confine = NULL;
    }

  if (_bdk_directfb_pointer_grab_cursor)
    {
      bdk_cursor_unref (_bdk_directfb_pointer_grab_cursor);
      _bdk_directfb_pointer_grab_cursor = NULL;
    }

  old_grab_window = _bdk_directfb_pointer_grab_window;

  _bdk_directfb_pointer_grab_window   = NULL;
  _bdk_directfb_pointer_implicit_grab = FALSE;

  mousewin = bdk_window_at_pointer (NULL, NULL);
  bdk_directfb_window_send_crossing_events (old_grab_window,
                                            mousewin,
                                            BDK_CROSSING_UNGRAB);
  g_object_unref (old_grab_window);
}

void
bdk_display_pointer_ungrab (BdkDisplay *display,
                            buint32 time)
{
  BdkPointerGrabInfo *grab = _bdk_display_get_last_pointer_grab (display);

  if (grab)
    {
      grab->serial_end = 0;
    }

  _bdk_display_pointer_grab_update (display, 0);
}


/*************************************************************************************************
 * Keyboard
 */

BdkGrabStatus
bdk_directfb_keyboard_grab (BdkDisplay *display,
                            BdkWindow  *window,
                            bint        owner_events,
                            buint32     time)
{
  BdkWindow             *toplevel;
  BdkWindowImplDirectFB *impl;

  g_return_val_if_fail (BDK_IS_WINDOW (window), 0);

  if (_bdk_directfb_keyboard_grab_window)
    bdk_keyboard_ungrab (time);

  toplevel = bdk_directfb_window_find_toplevel (window);
  impl = BDK_WINDOW_IMPL_DIRECTFB (BDK_WINDOW_OBJECT (toplevel)->impl);

  if (impl->window)
    {
      if (impl->window->GrabKeyboard (impl->window) == DFB_LOCKED)
        return BDK_GRAB_ALREADY_GRABBED;
    }

  _bdk_directfb_keyboard_grab_window = g_object_ref (window);
  _bdk_directfb_keyboard_grab_owner_events = owner_events;
  return BDK_GRAB_SUCCESS;
}

void
bdk_directfb_keyboard_ungrab (BdkDisplay *display,
                              buint32     time)
{
  BdkWindow             *toplevel;
  BdkWindowImplDirectFB *impl;

  if (!_bdk_directfb_keyboard_grab_window)
    return;

  toplevel = bdk_directfb_window_find_toplevel (_bdk_directfb_keyboard_grab_window);
  impl = BDK_WINDOW_IMPL_DIRECTFB (BDK_WINDOW_OBJECT (toplevel)->impl);

  if (impl->window)
    impl->window->UngrabKeyboard (impl->window);

  g_object_unref (_bdk_directfb_keyboard_grab_window);
  _bdk_directfb_keyboard_grab_window = NULL;
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
bdk_display_keyboard_grab (BdkDisplay *display,
                           BdkWindow  *window,
                           bint        owner_events,
                           buint32     time)
{
  return bdk_directfb_keyboard_grab (display, window, owner_events, time);
}

void
bdk_display_keyboard_ungrab (BdkDisplay *display,
                             buint32     time)
{
  return bdk_directfb_keyboard_ungrab (display, time);
}


/*************************************************************************************************
 * Misc Stuff
 */

void
bdk_display_beep (BdkDisplay *display)
{
}

void
bdk_display_sync (BdkDisplay *display)
{
}

void
bdk_display_flush (BdkDisplay *display)
{
}



/*************************************************************************************************
 * Notifications
 */

void
bdk_notify_startup_complete (void)
{
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
bdk_notify_startup_complete_with_id (const bchar* startup_id)
{
}


/*************************************************************************************************
 * Compositing
 */

bboolean
bdk_display_supports_composite (BdkDisplay *display)
{
  return FALSE;
}


#define __BDK_DISPLAY_X11_C__
#include "bdkaliasdef.c"

