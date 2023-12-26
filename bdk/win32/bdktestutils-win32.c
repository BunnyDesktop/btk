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
#include <win32/bdkwin32.h>
#include "bdkalias.h"

void
bdk_test_render_sync (BdkWindow *window)
{
}

gboolean
bdk_test_simulate_key (BdkWindow      *window,
                       gint            x,
                       gint            y,
                       guint           keyval,
                       BdkModifierType modifiers,
                       BdkEventType    key_pressrelease)
{
  gboolean      success = FALSE;
  BdkKeymapKey *keys    = NULL;
  gint          n_keys  = 0;
  INPUT         ip;
  gint          i;

  g_return_val_if_fail (key_pressrelease == BDK_KEY_PRESS || key_pressrelease == BDK_KEY_RELEASE, FALSE);
  g_return_val_if_fail (window != NULL, FALSE);

  ip.type = INPUT_KEYBOARD;
  ip.ki.wScan = 0;
  ip.ki.time = 0;
  ip.ki.dwExtraInfo = 0;

  switch (key_pressrelease)
    {
    case BDK_KEY_PRESS:
      ip.ki.dwFlags = 0;
      break;
    case BDK_KEY_RELEASE:
      ip.ki.dwFlags = KEYEVENTF_KEYUP;
      break;
    default:
      /* Not a key event. */
      return FALSE;
    }
  if (bdk_keymap_get_entries_for_keyval (bdk_keymap_get_default (), keyval, &keys, &n_keys))
    {
      for (i = 0; i < n_keys; i++)
        {
          if (key_pressrelease == BDK_KEY_PRESS)
            {
              /* AltGr press. */
              if (keys[i].group)
                {
                  /* According to some virtualbox code I found, AltGr is
                   * simulated on win32 with LCtrl+RAlt */
                  ip.ki.wVk = VK_CONTROL;
                  SendInput(1, &ip, sizeof(INPUT));
                  ip.ki.wVk = VK_MENU;
                  SendInput(1, &ip, sizeof(INPUT));
                }
              /* Shift press. */
              if (keys[i].level || (modifiers & BDK_SHIFT_MASK))
                {
                  ip.ki.wVk = VK_SHIFT;
                  SendInput(1, &ip, sizeof(INPUT));
                }
            }

          /* Key pressed/released. */
          ip.ki.wVk = keys[i].keycode;
          SendInput(1, &ip, sizeof(INPUT));

          if (key_pressrelease == BDK_KEY_RELEASE)
            {
              /* Shift release. */
              if (keys[i].level || (modifiers & BDK_SHIFT_MASK))
                {
                  ip.ki.wVk = VK_SHIFT;
                  SendInput(1, &ip, sizeof(INPUT));
                }
              /* AltrGr release. */
              if (keys[i].group)
                {
                  ip.ki.wVk = VK_MENU;
                  SendInput(1, &ip, sizeof(INPUT));
                  ip.ki.wVk = VK_CONTROL;
                  SendInput(1, &ip, sizeof(INPUT));
                }
            }

          /* No need to loop for alternative keycodes. We want only one
           * key generated. */
          success = TRUE;
          break;
        }
      g_free (keys);
    }
  return success;
}

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

  return FALSE;
}
