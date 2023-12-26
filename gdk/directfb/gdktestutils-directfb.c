/* Btk+ testing utilities
 * Copyright (C) 2007 Imendio AB
 * Authors: Tim Janik
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
 * BTK+ DirectFB backend
 * Copyright (C) 2001-2002  convergence integrated media GmbH
 * Copyright (C) 2002-2004  convergence GmbH
 * Written by Denis Oliver Kropp <dok@convergence.de> and
 *            Sven Neumann <sven@convergence.de>
 */
#include "config.h"

#include <unistd.h>

#include "bdk.h"
#include "bdkdirectfb.h"
#include "bdkprivate-directfb.h"

#include <bdk/bdktestutils.h>
#include <bdk/bdkkeysyms.h>
#include "bdkalias.h"


static DFBInputDeviceKeySymbol
_bdk_keyval_to_directfb (guint keyval)
{
  switch (keyval) {
  case 0 ... 127:
    return DFB_KEY (UNICODE, keyval);
  case BDK_F1 ... BDK_F12:
    return keyval - BDK_F1 + DIKS_F1;
  case BDK_BackSpace:
    return DIKS_BACKSPACE;
  case BDK_Tab:
    return DIKS_TAB;
  case BDK_Return:
    return DIKS_RETURN;
  case BDK_Escape:
    return DIKS_ESCAPE;
  case BDK_Delete:
    return DIKS_DELETE;
  case BDK_Left:
    return DIKS_CURSOR_LEFT;
  case BDK_Up:
    return DIKS_CURSOR_UP;
  case BDK_Right:
    return DIKS_CURSOR_RIGHT;
  case BDK_Down:
    return DIKS_CURSOR_DOWN;
  case BDK_Insert:
    return DIKS_INSERT;
  case BDK_Home:
    return DIKS_HOME;
  case BDK_End:
    return DIKS_END;
  case BDK_Page_Up:
    return DIKS_PAGE_UP;
  case BDK_Page_Down:
    return DIKS_PAGE_DOWN;
  case BDK_Print:
    return DIKS_PRINT;
  case BDK_Pause:
    return DIKS_PAUSE;
  case BDK_Clear:
    return DIKS_CLEAR;
  case BDK_Cancel:
    return DIKS_CANCEL;
    /* TODO: handle them all */
  default:
    break;
  }

  return DIKS_NULL;
}

static DFBInputDeviceModifierMask
_bdk_modifiers_to_directfb (BdkModifierType modifiers)
{
  DFBInputDeviceModifierMask dfb_modifiers = 0;

  if (modifiers & BDK_MOD1_MASK)
    dfb_modifiers |= DIMM_ALT;
  if (modifiers & BDK_MOD2_MASK)
    dfb_modifiers |= DIMM_ALTGR;
  if (modifiers & BDK_CONTROL_MASK)
    dfb_modifiers |= DIMM_CONTROL;
  if (modifiers & BDK_SHIFT_MASK)
    dfb_modifiers |= DIMM_SHIFT;

  return dfb_modifiers;
}

/**
 * bdk_test_render_sync
 * @window: a mapped BdkWindow
 *
 * This function retrives a pixel from @window to force the windowing
 * system to carry out any pending rendering commands.
 * This function is intended to be used to syncronize with rendering
 * pipelines, to benchmark windowing system rendering operations.
 **/
void
bdk_test_render_sync (BdkWindow *window)
{
  _bdk_display->directfb->WaitIdle (_bdk_display->directfb);
}

/**
 * bdk_test_simulate_key
 * @window: Bdk window to simulate a key event for.
 * @x:      x coordinate within @window for the key event.
 * @y:      y coordinate within @window for the key event.
 * @keyval: A Bdk keyboard value.
 * @modifiers: Keyboard modifiers the event is setup with.
 * @key_pressrelease: either %BDK_KEY_PRESS or %BDK_KEY_RELEASE
 *
 * This function is intended to be used in Btk+ test programs.
 * If (@x,@y) are > (-1,-1), it will warp the mouse pointer to
 * the given (@x,@y) corrdinates within @window and simulate a
 * key press or release event.
 * When the mouse pointer is warped to the target location, use
 * of this function outside of test programs that run in their
 * own virtual windowing system (e.g. Xvfb) is not recommended.
 * If (@x,@y) are passed as (-1,-1), the mouse pointer will not
 * be warped and @window origin will be used as mouse pointer
 * location for the event.
 * Also, btk_test_simulate_key() is a fairly low level function,
 * for most testing purposes, btk_test_widget_send_key() is the
 * right function to call which will generate a key press event
 * followed by its accompanying key release event.
 *
 * Returns: wether all actions neccessary for a key event simulation were carried out successfully.
 **/
gboolean
bdk_test_simulate_key (BdkWindow      *window,
                       gint            x,
                       gint            y,
                       guint           keyval,
                       BdkModifierType modifiers,
                       BdkEventType    key_pressrelease)
{
  BdkWindowObject       *private;
  BdkWindowImplDirectFB *impl;
  DFBWindowEvent         evt;

  g_return_val_if_fail (BDK_IS_WINDOW(window), FALSE);
  g_return_val_if_fail (key_pressrelease == BDK_KEY_PRESS ||
                        key_pressrelease == BDK_KEY_RELEASE, FALSE);

  private = BDK_WINDOW_OBJECT (window);
  impl = BDK_WINDOW_IMPL_DIRECTFB (private->impl);

  if (x >= 0 && y >= 0) {
    int win_x, win_y;
    impl->window->GetPosition (impl->window, &win_x, &win_y);
    if (_bdk_display->layer->WarpCursor (_bdk_display->layer, win_x+x, win_y+y))
      return FALSE;
  }

  evt.clazz      = DFEC_WINDOW;
  evt.type       = (key_pressrelease == BDK_KEY_PRESS) ? DWET_KEYDOWN : DWET_KEYUP;
#if ((DIRECTFB_MAJOR_VERSION > 1) || (DIRECTFB_MINOR_VERSION >= 2))
  evt.flags      = DWEF_NONE;
#endif
  evt.window_id  = impl->dfb_id;
  evt.x          = MAX(x, 0);
  evt.y          = MAX(y, 0);
  _bdk_display->layer->GetCursorPosition (_bdk_display->layer, &evt.cx, &evt.cy);
  evt.key_code   = -1;
  evt.key_symbol = _bdk_keyval_to_directfb (keyval);
  evt.modifiers  = _bdk_modifiers_to_directfb (modifiers);
  evt.locks      = (modifiers & BDK_LOCK_MASK) ? DILS_CAPS : 0;
  gettimeofday (&evt.timestamp, NULL);

  _bdk_display->buffer->PostEvent (_bdk_display->buffer, DFB_EVENT(&evt));

  return TRUE;
}

/**
 * bdk_test_simulate_button
 * @window: Bdk window to simulate a button event for.
 * @x:      x coordinate within @window for the button event.
 * @y:      y coordinate within @window for the button event.
 * @button: Number of the pointer button for the event, usually 1, 2 or 3.
 * @modifiers: Keyboard modifiers the event is setup with.
 * @button_pressrelease: either %BDK_BUTTON_PRESS or %BDK_BUTTON_RELEASE
 *
 * This function is intended to be used in Btk+ test programs.
 * It will warp the mouse pointer to the given (@x,@y) corrdinates
 * within @window and simulate a button press or release event.
 * Because the mouse pointer needs to be warped to the target
 * location, use of this function outside of test programs that
 * run in their own virtual windowing system (e.g. Xvfb) is not
 * recommended.
 * Also, btk_test_simulate_button() is a fairly low level function,
 * for most testing purposes, btk_test_widget_click() is the right
 * function to call which will generate a button press event followed
 * by its accompanying button release event.
 *
 * Returns: wether all actions neccessary for a button event simulation were carried out successfully.
 **/
gboolean
bdk_test_simulate_button (BdkWindow      *window,
                          gint            x,
                          gint            y,
                          guint           button, /*1..3*/
                          BdkModifierType modifiers,
                          BdkEventType    button_pressrelease)
{
  BdkWindowObject       *private;
  BdkWindowImplDirectFB *impl;
  DFBWindowEvent         evt;

  g_return_val_if_fail (BDK_IS_WINDOW (window), FALSE);
  g_return_val_if_fail (button_pressrelease == BDK_BUTTON_PRESS ||
                        button_pressrelease == BDK_BUTTON_RELEASE, FALSE);

  private = BDK_WINDOW_OBJECT (window);
  impl = BDK_WINDOW_IMPL_DIRECTFB (private->impl);

  if (x >= 0 && y >= 0) {
    int win_x, win_y;
    impl->window->GetPosition (impl->window, &win_x, &win_y);
    if (_bdk_display->layer->WarpCursor (_bdk_display->layer, win_x+x, win_y+y))
      return FALSE;
  }

  evt.clazz      = DFEC_WINDOW;
  evt.type       = (button_pressrelease == BDK_BUTTON_PRESS) ? DWET_BUTTONDOWN : DWET_BUTTONUP;
#if ((DIRECTFB_MAJOR_VERSION > 1) || (DIRECTFB_MINOR_VERSION >= 2))
  evt.flags      = DWEF_NONE;
#endif
  evt.window_id  = impl->dfb_id;
  evt.x          = MAX (x, 0);
  evt.y          = MAX (y, 0);
  _bdk_display->layer->GetCursorPosition (_bdk_display->layer, &evt.cx, &evt.cy);
  evt.modifiers  = _bdk_modifiers_to_directfb (modifiers);
  evt.locks      = (modifiers & BDK_LOCK_MASK) ? DILS_CAPS : 0;
  evt.button     = button;
  evt.buttons    = 0;
  gettimeofday (&evt.timestamp, NULL);

  _bdk_display->buffer->PostEvent (_bdk_display->buffer, DFB_EVENT(&evt));

  return TRUE;
}

#define __BDK_TEST_UTILS_X11_C__
#include "bdkaliasdef.c"
