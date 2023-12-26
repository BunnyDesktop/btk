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
#include <bdk/bdktestutils.h>
#include <bdk/bdkkeysyms.h>
#include <x11/bdkx.h>
#include "bdkalias.h"

#include <X11/Xlib.h>

/**
 * bdk_test_render_sync:
 * @window: a mapped #BdkWindow
 *
 * This function retrieves a pixel from @window to force the windowing
 * system to carry out any pending rendering commands.
 * This function is intended to be used to syncronize with rendering
 * pipelines, to benchmark windowing system rendering operations.
 *
 * Since: 2.14
 **/
void
bdk_test_render_sync (BdkWindow *window)
{
  static BdkImage *p1image = NULL;
  /* syncronize to X drawing queue, see:
   * http://mail.bunny.org/archives/btk-devel-list/2006-October/msg00103.html
   */
  p1image = bdk_drawable_copy_to_image (window, p1image, 0, 0, 0, 0, 1, 1);
}

/**
 * bdk_test_simulate_key
 * @window: a #BdkWindow to simulate a key event for.
 * @x:      x coordinate within @window for the key event.
 * @y:      y coordinate within @window for the key event.
 * @keyval: A BDK keyboard value.
 * @modifiers: Keyboard modifiers the event is setup with.
 * @key_pressrelease: either %BDK_KEY_PRESS or %BDK_KEY_RELEASE
 *
 * This function is intended to be used in BTK+ test programs.
 * If (@x,@y) are > (-1,-1), it will warp the mouse pointer to
 * the given (@x,@y) corrdinates within @window and simulate a
 * key press or release event.
 *
 * When the mouse pointer is warped to the target location, use
 * of this function outside of test programs that run in their
 * own virtual windowing system (e.g. Xvfb) is not recommended.
 * If (@x,@y) are passed as (-1,-1), the mouse pointer will not
 * be warped and @window origin will be used as mouse pointer
 * location for the event.
 *
 * Also, btk_test_simulate_key() is a fairly low level function,
 * for most testing purposes, btk_test_widget_send_key() is the
 * right function to call which will generate a key press event
 * followed by its accompanying key release event.
 *
 * Returns: whether all actions neccessary for a key event simulation 
 *     were carried out successfully.
 *
 * Since: 2.14
 **/
bboolean
bdk_test_simulate_key (BdkWindow      *window,
                       bint            x,
                       bint            y,
                       buint           keyval,
                       BdkModifierType modifiers,
                       BdkEventType    key_pressrelease)
{
  BdkScreen *screen;
  BdkKeymapKey *keys = NULL;
  BdkWindowObject *priv;
  bboolean success;
  bint n_keys = 0;
  XKeyEvent xev = {
    0,  /* type */
    0,  /* serial */
    1,  /* send_event */
  };
  g_return_val_if_fail (key_pressrelease == BDK_KEY_PRESS || key_pressrelease == BDK_KEY_RELEASE, FALSE);
  g_return_val_if_fail (window != NULL, FALSE);
  if (!BDK_WINDOW_IS_MAPPED (window))
    return FALSE;
  screen = bdk_colormap_get_screen (bdk_drawable_get_colormap (window));
  if (x < 0 && y < 0)
    {
      bdk_drawable_get_size (window, &x, &y);
      x /= 2;
      y /= 2;
    }

  priv = (BdkWindowObject *)window;
  /* Convert to impl coordinates */
  x = x + priv->abs_x;
  y = y + priv->abs_y;

  xev.type = key_pressrelease == BDK_KEY_PRESS ? KeyPress : KeyRelease;
  xev.display = BDK_DRAWABLE_XDISPLAY (window);
  xev.window = BDK_WINDOW_XID (window);
  xev.root = RootWindow (xev.display, BDK_SCREEN_XNUMBER (screen));
  xev.subwindow = 0;
  xev.time = 0;
  xev.x = MAX (x, 0);
  xev.y = MAX (y, 0);
  xev.x_root = 0;
  xev.y_root = 0;
  xev.state = modifiers;
  xev.keycode = 0;
  success = bdk_keymap_get_entries_for_keyval (bdk_keymap_get_for_display (bdk_drawable_get_display (window)), keyval, &keys, &n_keys);
  success &= n_keys > 0;
  if (success)
    {
      bint i;
      for (i = 0; i < n_keys; i++)
        if (keys[i].group == 0 && (keys[i].level == 0 || keys[i].level == 1))
          {
            xev.keycode = keys[i].keycode;
            if (keys[i].level == 1)
              {
                /* Assume shift takes us to level 1 */
                xev.state |= BDK_SHIFT_MASK;
              }
            break;
          }
      if (i >= n_keys) /* no match for group==0 and level==0 or 1 */
        xev.keycode = keys[0].keycode;
    }
  g_free (keys);
  if (!success)
    return FALSE;
  bdk_error_trap_push ();
  xev.same_screen = XTranslateCoordinates (xev.display, xev.window, xev.root,
                                           xev.x, xev.y, &xev.x_root, &xev.y_root,
                                           &xev.subwindow);
  if (!xev.subwindow)
    xev.subwindow = xev.window;
  success &= xev.same_screen;
  if (x >= 0 && y >= 0)
    success &= 0 != XWarpPointer (xev.display, None, xev.window, 0, 0, 0, 0, xev.x, xev.y);
  success &= 0 != XSendEvent (xev.display, xev.window, True, key_pressrelease == BDK_KEY_PRESS ? KeyPressMask : KeyReleaseMask, (XEvent*) &xev);
  XSync (xev.display, False);
  success &= 0 == bdk_error_trap_pop();
  return success;
}

/**
 * bdk_test_simulate_button
 * @window: a #BdkWindow to simulate a button event for.
 * @x:      x coordinate within @window for the button event.
 * @y:      y coordinate within @window for the button event.
 * @button: Number of the pointer button for the event, usually 1, 2 or 3.
 * @modifiers: Keyboard modifiers the event is setup with.
 * @button_pressrelease: either %BDK_BUTTON_PRESS or %BDK_BUTTON_RELEASE
 *
 * This function is intended to be used in BTK+ test programs.
 * It will warp the mouse pointer to the given (@x,@y) corrdinates
 * within @window and simulate a button press or release event.
 * Because the mouse pointer needs to be warped to the target
 * location, use of this function outside of test programs that
 * run in their own virtual windowing system (e.g. Xvfb) is not
 * recommended.
 *
 * Also, btk_test_simulate_button() is a fairly low level function,
 * for most testing purposes, btk_test_widget_click() is the right
 * function to call which will generate a button press event followed
 * by its accompanying button release event.
 *
 * Returns: whether all actions neccessary for a button event simulation 
 *     were carried out successfully.
 *
 * Since: 2.14
 **/
bboolean
bdk_test_simulate_button (BdkWindow      *window,
                          bint            x,
                          bint            y,
                          buint           button, /*1..3*/
                          BdkModifierType modifiers,
                          BdkEventType    button_pressrelease)
{
  BdkScreen *screen;
  XButtonEvent xev = {
    0,  /* type */
    0,  /* serial */
    1,  /* send_event */
  };
  bboolean success;
  BdkWindowObject *priv;

  g_return_val_if_fail (button_pressrelease == BDK_BUTTON_PRESS || button_pressrelease == BDK_BUTTON_RELEASE, FALSE);
  g_return_val_if_fail (window != NULL, FALSE);

  if (!BDK_WINDOW_IS_MAPPED (window))
    return FALSE;
  screen = bdk_colormap_get_screen (bdk_drawable_get_colormap (window));
  if (x < 0 && y < 0)
    {
      bdk_drawable_get_size (window, &x, &y);
      x /= 2;
      y /= 2;
    }

  priv = (BdkWindowObject *)window;
  /* Convert to impl coordinates */
  x = x + priv->abs_x;
  y = y + priv->abs_y;

  xev.type = button_pressrelease == BDK_BUTTON_PRESS ? ButtonPress : ButtonRelease;
  xev.display = BDK_DRAWABLE_XDISPLAY (window);
  xev.window = BDK_WINDOW_XID (window);
  xev.root = RootWindow (xev.display, BDK_SCREEN_XNUMBER (screen));
  xev.subwindow = 0;
  xev.time = 0;
  xev.x = x;
  xev.y = y;
  xev.x_root = 0;
  xev.y_root = 0;
  xev.state = modifiers;
  xev.button = button;
  bdk_error_trap_push ();
  xev.same_screen = XTranslateCoordinates (xev.display, xev.window, xev.root,
                                           xev.x, xev.y, &xev.x_root, &xev.y_root,
                                           &xev.subwindow);
  if (!xev.subwindow)
    xev.subwindow = xev.window;
  success = xev.same_screen;
  success &= 0 != XWarpPointer (xev.display, None, xev.window, 0, 0, 0, 0, xev.x, xev.y);
  success &= 0 != XSendEvent (xev.display, xev.window, True, button_pressrelease == BDK_BUTTON_PRESS ? ButtonPressMask : ButtonReleaseMask, (XEvent*) &xev);
  XSync (xev.display, False);
  success &= 0 == bdk_error_trap_pop();
  return success;
}

#define __BDK_TEST_UTILS_X11_C__
#include "bdkaliasdef.c"
