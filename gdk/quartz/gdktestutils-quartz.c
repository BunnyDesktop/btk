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
#include "config.h"
#include <bdk/bdktestutils.h>
#include <bdk/bdkkeysyms.h>
#include <bdk/bdkinternals.h>

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
  /* FIXME: Find out if there is a way to implement this on quartz. */
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
  g_return_val_if_fail (key_pressrelease == BDK_KEY_PRESS || key_pressrelease == BDK_KEY_RELEASE, FALSE);
  g_return_val_if_fail (window != NULL, FALSE);

  if (!BDK_WINDOW_IS_MAPPED (window))
    return FALSE;

  /* FIXME: Implement. */

  return FALSE;
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
  g_return_val_if_fail (button_pressrelease == BDK_BUTTON_PRESS || button_pressrelease == BDK_BUTTON_RELEASE, FALSE);
  g_return_val_if_fail (window != NULL, FALSE);

  if (!BDK_WINDOW_IS_MAPPED (window))
    return FALSE;

  /* FIXME: Implement. */

  return FALSE;
}
