/* BDK - The GIMP Drawing Kit
 * Copyright (C) 2000 Red Hat, Inc.
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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "bdk.h"

#include "bdkprivate-win32.h"
#include "bdkinternals.h"
#include "bdkkeysyms.h"
#include "bdkkeys.h"
#include "bdkwin32keys.h"

enum _BdkWin32KeyLevelState
{
  BDK_WIN32_LEVEL_NONE = 0,
  BDK_WIN32_LEVEL_SHIFT,
  BDK_WIN32_LEVEL_CAPSLOCK,
  BDK_WIN32_LEVEL_SHIFT_CAPSLOCK,
  BDK_WIN32_LEVEL_ALTGR,
  BDK_WIN32_LEVEL_SHIFT_ALTGR,
  BDK_WIN32_LEVEL_CAPSLOCK_ALTGR,
  BDK_WIN32_LEVEL_SHIFT_CAPSLOCK_ALTGR,
  BDK_WIN32_LEVEL_COUNT
};

typedef enum _BdkWin32KeyLevelState BdkWin32KeyLevelState;

struct _BdkWin32KeyNode
{
  /* Non-spacing version of the dead key */
  guint                  undead_bdk_keycode;

  /* Virtual key code */
  guint8                 vk;

  /* Level for which this virtual key code produces this bdk_keycode */
  BdkWin32KeyLevelState  level;

  /* BDK (X11) code for this key */
  guint                  bdk_keycode;

  /* Array of BdkWin32KeyNode should be sorted by bdk_keycode, then by level */
  GArray                *combinations;
};

typedef struct _BdkWin32KeyNode BdkWin32KeyNode;

/*
Example:
  BdkWin32KeyNode
  {
    undead_bdk_keycode = 0x0b4 BDK_KEY_acute (')
    vk = 0xde VK_OEM_7
    level = BDK_WIN32_LEVEL_NONE
    bdk_keycode = 0xfe51 BDK_KEY_dead_acute
    combinations = 
    {
      BdkWin32KeyNode
      {
        undead_bdk_keycode = 0x061 BDK_KEY_a (a)
        level = BDK_WIN32_LEVEL_NONE
        vk = 0x41 VK_A
        bdk_keycode = 0xe1 BDK_KEY_aacute á
        combinations = NULL
      },
      BdkWin32KeyNode
      {
        unicode_char = 0x041 BDK_KEY_A (A)
        level = BDK_WIN32_LEVEL_SHIFT
        vk = 0x41 VK_A
        bdk_keycode = 0x0c1 BDK_KEY_Aacute Á
        combinations = NULL
      },
      { ... }
    }
  }

Thus:

BDK_KEY_dead_acute + BDK_KEY_a
= BDK_KEY_aacute

BDK_KEY_dead_acute + BDK_KEY_A
= BDK_KEY_Aacute

BDK_KEY_dead_acute + BDK_KEY_s
matches partially
(BDK_KEY_dead_acute is a known dead key, but does not combine with BDK_KEY_s)
and resolves into:
BDK_KEY_acute + BDK_KEY_s

BDK_KEY_dead_somethingelse + BDK_KEY_anything
does not match at all
(W32 API did not provide any deadkey info for BDK_KEY_dead_somethingelse)
and the caller will try other matching mechanisms for compose_buffer
*/

struct _BdkWin32KeyGroupOptions
{
  /* character that should be used as the decimal separator */
  wchar_t         decimal_mark;

  /* Scancode for the VK_RSHIFT */
  guint           scancode_rshift;

  /* TRUE if Ctrl+Alt emulates AltGr */
  gboolean        has_altgr;

  GArray         *dead_keys;
};

typedef struct _BdkWin32KeyGroupOptions BdkWin32KeyGroupOptions;

struct _BdkWin32KeymapClass
{
  BdkKeymapClass parent_class;
};

struct _BdkWin32Keymap
{
  BdkKeymap parent_instance;

  /* length = what GetKeyboardLayoutList() returns, type = HKL.
   * When it changes, recreate the keymap and repopulate the options.
   */
  GArray *layout_handles;

  /* VirtualKeyCode -> bdk_keyval table
   * length = 256 * length(layout_handles) * 2 * 4
   * 256 is the number of virtual key codes,
   * 2x4 is the number of Shift/AltGr/CapsLock combinations (level),
   * length(layout_handles) is the number of layout handles (group).
   */
  guint  *keysym_tab;

  /* length = length(layout_handles), type =  BdkWin32KeyGroupOptions
   * Kept separate from layout_handles because layout_handles is
   * populated by W32 API.
   */
  GArray *options;

  /* Index of a handle in layout_handles,
   * at any point it should be the same handle as GetKeyboardLayout(0) returns,
   * but BDK caches it to avoid calling GetKeyboardLayout (0) every time.
   */
  guint8 active_layout;
};

G_DEFINE_TYPE (BdkWin32Keymap, bdk_win32_keymap, BDK_TYPE_KEYMAP)

guint _bdk_keymap_serial = 0;

static BdkKeymap *default_keymap = NULL;

#define KEY_STATE_SIZE 256

static void update_keymap (BdkKeymap *bdk_keymap);

static void
bdk_win32_key_group_options_clear (BdkWin32KeyGroupOptions *options)
{
  g_clear_pointer (&options->dead_keys, g_array_unref);
}

static void
bdk_win32_key_node_clear (BdkWin32KeyNode *node)
{
  g_clear_pointer (&node->combinations, g_array_unref);
}

static void
bdk_win32_keymap_init (BdkWin32Keymap *keymap)
{
  keymap->layout_handles = g_array_new (FALSE, FALSE, sizeof (HKL));
  keymap->options = g_array_new (FALSE, FALSE, sizeof (BdkWin32KeyGroupOptions));
  g_array_set_clear_func (keymap->options, (GDestroyNotify) bdk_win32_key_group_options_clear);
  keymap->keysym_tab = NULL;
  keymap->active_layout = 0;
  update_keymap (BDK_KEYMAP (keymap));
}

static void
bdk_win32_keymap_finalize (GObject *object)
{
  BdkWin32Keymap *keymap = BDK_WIN32_KEYMAP (object);

  g_clear_pointer (&keymap->keysym_tab, g_free);
  g_clear_pointer (&keymap->layout_handles, g_array_unref);
  g_clear_pointer (&keymap->options, g_array_unref);

  G_OBJECT_CLASS (bdk_win32_keymap_parent_class)->finalize (object);
}

#ifdef G_ENABLE_DEBUG
static void
print_keysym_tab (BdkWin32Keymap *keymap)
{
  gint                      li;
  BdkWin32KeyGroupOptions  *options;
  gint                      vk;
  BdkWin32KeyLevelState     level;
  gint                      group_size = keymap->layout_handles->len;

  for (li = 0; li < group_size; li++)
    {
      options = &g_array_index (keymap->options, BdkWin32KeyGroupOptions, li);
      g_print ("keymap %d (0x%p):%s\n",
               li, g_array_index (keymap->layout_handles, HKL, li),
               options->has_altgr ? " (uses AltGr)" : "");

      for (vk = 0; vk < KEY_STATE_SIZE; vk++)
        {
          g_print ("%#.02x: ", vk);

          for (level = 0; level < BDK_WIN32_LEVEL_COUNT; level++)
            {
              gchar *name = bdk_keyval_name (keymap->keysym_tab[vk * group_size * BDK_WIN32_LEVEL_COUNT + level]);

              g_print ("%s ", name ? name : "(none)");
            }

          g_print ("\n");
        }
     }
}
#endif

static void
handle_special (guint  vk,
		guint *ksymp,
		gint   shift)

{
  switch (vk)
    {
    case VK_CANCEL:
      *ksymp = BDK_KEY_Cancel; break;
    case VK_BACK:
      *ksymp = BDK_KEY_BackSpace; break;
    case VK_TAB:
      if (shift & 0x1)
	*ksymp = BDK_KEY_ISO_Left_Tab;
      else
	*ksymp = BDK_KEY_Tab;
      break;
    case VK_CLEAR:
      *ksymp = BDK_KEY_Clear; break;
    case VK_RETURN:
      *ksymp = BDK_KEY_Return; break;
    case VK_SHIFT:
    case VK_LSHIFT:
      *ksymp = BDK_KEY_Shift_L; break;
    case VK_CONTROL:
    case VK_LCONTROL:
      *ksymp = BDK_KEY_Control_L; break;
    case VK_MENU:
    case VK_LMENU:
      *ksymp = BDK_KEY_Alt_L; break;
    case VK_PAUSE:
      *ksymp = BDK_KEY_Pause; break;
    case VK_ESCAPE:
      *ksymp = BDK_KEY_Escape; break;
    case VK_PRIOR:
      *ksymp = BDK_KEY_Prior; break;
    case VK_NEXT:
      *ksymp = BDK_KEY_Next; break;
    case VK_END:
      *ksymp = BDK_KEY_End; break;
    case VK_HOME:
      *ksymp = BDK_KEY_Home; break;
    case VK_LEFT:
      *ksymp = BDK_KEY_Left; break;
    case VK_UP:
      *ksymp = BDK_KEY_Up; break;
    case VK_RIGHT:
      *ksymp = BDK_KEY_Right; break;
    case VK_DOWN:
      *ksymp = BDK_KEY_Down; break;
    case VK_SELECT:
      *ksymp = BDK_KEY_Select; break;
    case VK_PRINT:
      *ksymp = BDK_KEY_Print; break;
    case VK_SNAPSHOT:
      *ksymp = BDK_KEY_Print; break;
    case VK_EXECUTE:
      *ksymp = BDK_KEY_Execute; break;
    case VK_INSERT:
      *ksymp = BDK_KEY_Insert; break;
    case VK_DELETE:
      *ksymp = BDK_KEY_Delete; break;
    case VK_HELP:
      *ksymp = BDK_KEY_Help; break;
    case VK_LWIN:
      *ksymp = BDK_KEY_Meta_L; break;
    case VK_RWIN:
      *ksymp = BDK_KEY_Meta_R; break;
    case VK_APPS:
      *ksymp = BDK_KEY_Menu; break;
    case VK_DECIMAL:
      *ksymp = BDK_KEY_KP_Decimal; break;
    case VK_MULTIPLY:
      *ksymp = BDK_KEY_KP_Multiply; break;
    case VK_ADD:
      *ksymp = BDK_KEY_KP_Add; break;
    case VK_SEPARATOR:
      *ksymp = BDK_KEY_KP_Separator; break;
    case VK_SUBTRACT:
      *ksymp = BDK_KEY_KP_Subtract; break;
    case VK_DIVIDE:
      *ksymp = BDK_KEY_KP_Divide; break;
    case VK_NUMPAD0:
      *ksymp = BDK_KEY_KP_0; break;
    case VK_NUMPAD1:
      *ksymp = BDK_KEY_KP_1; break;
    case VK_NUMPAD2:
      *ksymp = BDK_KEY_KP_2; break;
    case VK_NUMPAD3:
      *ksymp = BDK_KEY_KP_3; break;
    case VK_NUMPAD4:
      *ksymp = BDK_KEY_KP_4; break;
    case VK_NUMPAD5:
      *ksymp = BDK_KEY_KP_5; break;
    case VK_NUMPAD6:
      *ksymp = BDK_KEY_KP_6; break;
    case VK_NUMPAD7:
      *ksymp = BDK_KEY_KP_7; break;
    case VK_NUMPAD8:
      *ksymp = BDK_KEY_KP_8; break;
    case VK_NUMPAD9:
      *ksymp = BDK_KEY_KP_9; break;
    case VK_F1:
      *ksymp = BDK_KEY_F1; break;
    case VK_F2:
      *ksymp = BDK_KEY_F2; break;
    case VK_F3:
      *ksymp = BDK_KEY_F3; break;
    case VK_F4:
      *ksymp = BDK_KEY_F4; break;
    case VK_F5:
      *ksymp = BDK_KEY_F5; break;
    case VK_F6:
      *ksymp = BDK_KEY_F6; break;
    case VK_F7:
      *ksymp = BDK_KEY_F7; break;
    case VK_F8:
      *ksymp = BDK_KEY_F8; break;
    case VK_F9:
      *ksymp = BDK_KEY_F9; break;
    case VK_F10:
      *ksymp = BDK_KEY_F10; break;
    case VK_F11:
      *ksymp = BDK_KEY_F11; break;
    case VK_F12:
      *ksymp = BDK_KEY_F12; break;
    case VK_F13:
      *ksymp = BDK_KEY_F13; break;
    case VK_F14:
      *ksymp = BDK_KEY_F14; break;
    case VK_F15:
      *ksymp = BDK_KEY_F15; break;
    case VK_F16:
      *ksymp = BDK_KEY_F16; break;
    case VK_F17:
      *ksymp = BDK_KEY_F17; break;
    case VK_F18:
      *ksymp = BDK_KEY_F18; break;
    case VK_F19:
      *ksymp = BDK_KEY_F19; break;
    case VK_F20:
      *ksymp = BDK_KEY_F20; break;
    case VK_F21:
      *ksymp = BDK_KEY_F21; break;
    case VK_F22:
      *ksymp = BDK_KEY_F22; break;
    case VK_F23:
      *ksymp = BDK_KEY_F23; break;
    case VK_F24:
      *ksymp = BDK_KEY_F24; break;
    case VK_NUMLOCK:
      *ksymp = BDK_KEY_Num_Lock; break;
    case VK_SCROLL:
      *ksymp = BDK_KEY_Scroll_Lock; break;
    case VK_RSHIFT:
      *ksymp = BDK_KEY_Shift_R; break;
    case VK_RCONTROL:
      *ksymp = BDK_KEY_Control_R; break;
    case VK_RMENU:
      *ksymp = BDK_KEY_Alt_R; break;
    }
}

static void
set_level_vks (guchar               *key_state,
	       BdkWin32KeyLevelState level)
{
  switch (level)
    {
    case BDK_WIN32_LEVEL_NONE:
      key_state[VK_SHIFT] = 0;
      key_state[VK_CAPITAL] = 0;
      key_state[VK_CONTROL] = key_state[VK_MENU] = 0;
      break;
    case BDK_WIN32_LEVEL_SHIFT:
      key_state[VK_SHIFT] = 0x80;
      key_state[VK_CAPITAL] = 0;
      key_state[VK_CONTROL] = key_state[VK_MENU] = 0;
      break;
    case BDK_WIN32_LEVEL_CAPSLOCK:
      key_state[VK_SHIFT] = 0;
      key_state[VK_CAPITAL] = 0x01;
      key_state[VK_CONTROL] = key_state[VK_MENU] = 0;
      break;
    case BDK_WIN32_LEVEL_SHIFT_CAPSLOCK:
      key_state[VK_SHIFT] = 0x80;
      key_state[VK_CAPITAL] = 0x01;
      key_state[VK_CONTROL] = key_state[VK_MENU] = 0;
      break;
    case BDK_WIN32_LEVEL_ALTGR:
      key_state[VK_SHIFT] = 0;
      key_state[VK_CAPITAL] = 0;
      key_state[VK_CONTROL] = key_state[VK_MENU] = 0x80;
      break;
    case BDK_WIN32_LEVEL_SHIFT_ALTGR:
      key_state[VK_SHIFT] = 0x80;
      key_state[VK_CAPITAL] = 0;
      key_state[VK_CONTROL] = key_state[VK_MENU] = 0x80;
      break;
    case BDK_WIN32_LEVEL_CAPSLOCK_ALTGR:
      key_state[VK_SHIFT] = 0;
      key_state[VK_CAPITAL] = 0x01;
      key_state[VK_CONTROL] = key_state[VK_MENU] = 0x80;
      break;
    case BDK_WIN32_LEVEL_SHIFT_CAPSLOCK_ALTGR:
      key_state[VK_SHIFT] = 0x80;
      key_state[VK_CAPITAL] = 0x01;
      key_state[VK_CONTROL] = key_state[VK_MENU] = 0x80;
      break;
    case BDK_WIN32_LEVEL_COUNT:
      g_assert_not_reached ();
      break;
    }
}

static void
reset_after_dead (guchar key_state[KEY_STATE_SIZE],
                  HKL    handle)
{
  guchar  temp_key_state[KEY_STATE_SIZE];
  wchar_t wcs[2];

  memmove (temp_key_state, key_state, KEY_STATE_SIZE);

  temp_key_state[VK_SHIFT] =
    temp_key_state[VK_CONTROL] =
    temp_key_state[VK_CAPITAL] =
    temp_key_state[VK_MENU] = 0;

  ToUnicodeEx (VK_SPACE, MapVirtualKey (VK_SPACE, 0),
	       temp_key_state, wcs, G_N_ELEMENTS (wcs),
	       0, handle);
}

static void
handle_dead (guint  keysym,
	     guint *ksymp)
{
  switch (keysym)
    {
    case '"': /* 0x022 */
      *ksymp = BDK_KEY_dead_diaeresis; break;
    case '\'': /* 0x027 */
      *ksymp = BDK_KEY_dead_acute; break;
    case BDK_KEY_asciicircum: /* 0x05e */
      *ksymp = BDK_KEY_dead_circumflex; break;
    case BDK_KEY_grave:	/* 0x060 */
      *ksymp = BDK_KEY_dead_grave; break;
    case BDK_KEY_asciitilde: /* 0x07e */
      *ksymp = BDK_KEY_dead_tilde; break;
    case BDK_KEY_diaeresis: /* 0x0a8 */
      *ksymp = BDK_KEY_dead_diaeresis; break;
    case BDK_KEY_degree: /* 0x0b0 */
      *ksymp = BDK_KEY_dead_abovering; break;
    case BDK_KEY_acute:	/* 0x0b4 */
      *ksymp = BDK_KEY_dead_acute; break;
    case BDK_KEY_periodcentered: /* 0x0b7 */
      *ksymp = BDK_KEY_dead_abovedot; break;
    case BDK_KEY_cedilla: /* 0x0b8 */
      *ksymp = BDK_KEY_dead_cedilla; break;
    case BDK_KEY_breve:	/* 0x1a2 */
      *ksymp = BDK_KEY_dead_breve; break;
    case BDK_KEY_ogonek: /* 0x1b2 */
      *ksymp = BDK_KEY_dead_ogonek; break;
    case BDK_KEY_caron:	/* 0x1b7 */
      *ksymp = BDK_KEY_dead_caron; break;
    case BDK_KEY_doubleacute: /* 0x1bd */
      *ksymp = BDK_KEY_dead_doubleacute; break;
    case BDK_KEY_abovedot: /* 0x1ff */
      *ksymp = BDK_KEY_dead_abovedot; break;
    case 0x1000384: /* Greek tonos */
      *ksymp = BDK_KEY_dead_acute; break;
    case BDK_KEY_Greek_accentdieresis: /* 0x7ae */
      *ksymp = BDK_KEY_Greek_accentdieresis; break;
    default:
      /* By default use the keysym as such. This takes care of for
       * instance the dead U+09CD (BENGALI VIRAMA) on the ekushey
       * Bengali layout.
       */
      *ksymp = keysym; break;
    }
}

/* keypad decimal mark depends on active keyboard layout
   return current decimal mark as unicode character
   */
guint32
_bdk_win32_keymap_get_decimal_mark (BdkWin32Keymap *keymap)
{
  if (keymap != NULL &&
      keymap->layout_handles->len > 0 &&
      g_array_index (keymap->options, BdkWin32KeyGroupOptions, keymap->active_layout).decimal_mark)
    return g_array_index (keymap->options, BdkWin32KeyGroupOptions, keymap->active_layout).decimal_mark;

  return (guint32) '.';
}

static gboolean
layouts_are_the_same (GArray *array, HKL *hkls, gint hkls_len)
{
  gint i;

  if (hkls_len != array->len)
    return FALSE;

  for (i = 0; i < hkls_len; i++)
    if (hkls[i] != g_array_index (array, HKL, i))
      return FALSE;

  return TRUE;
}

static void
check_that_active_layout_is_in_sync (BdkWin32Keymap *keymap)
{
  HKL     hkl;
  HKL     cached_hkl;
  wchar_t hkl_name[KL_NAMELENGTH];

  if (keymap->layout_handles->len <= 0)
    return;

  hkl = GetKeyboardLayout (0);
  cached_hkl = g_array_index (keymap->layout_handles, HKL, keymap->active_layout);

  if (hkl != cached_hkl)
    {
      if (!GetKeyboardLayoutNameW (hkl_name))
        wcsncpy (hkl_name, L"(NULL)", KL_NAMELENGTH);

      g_warning ("Cached active layout #%d (0x%p) does not match actual layout %S, 0x%p",
                 keymap->active_layout, cached_hkl, hkl_name, hkl);
    }
}

static gint
sort_key_nodes_by_bdk_keyval (gconstpointer a,
                              gconstpointer b)
{
  const BdkWin32KeyNode *one = a;
  const BdkWin32KeyNode *two = b;

  if (one->bdk_keycode < two->bdk_keycode)
    return -1;
  else if (one->bdk_keycode > two->bdk_keycode)
    return 1;

  if (one->level < two->level)
    return -1;
  else if (one->level > two->level)
    return 1;

  return 0;
}

static void
update_keymap (BdkKeymap *bdk_keymap)
{
  int                      hkls_len;
  static int               hkls_size = 0;
  static HKL              *hkls = NULL;
  gboolean                 no_list;
  static guint             current_serial = 0;
  gint                     i, group;
  BdkWin32KeyLevelState    level;
  BdkWin32KeyGroupOptions *options;
  BdkWin32Keymap          *keymap = BDK_WIN32_KEYMAP (bdk_keymap);
  gint                     keysym_tab_size;

  guchar                   key_state[KEY_STATE_SIZE];
  guint                    scancode;
  guint                    vk;
  guint                   *keygroup;

  if (keymap->keysym_tab != NULL &&
      current_serial == _bdk_keymap_serial)
    return;

  no_list = FALSE;
  hkls_len = GetKeyboardLayoutList (0, NULL);

  if (hkls_len <= 0)
    {
      hkls_len = 1;
      no_list = TRUE;
    }
  else if (hkls_len > 255)
    {
      hkls_len = 255;
    }

  if (hkls_size < hkls_len)
    {
      hkls = g_renew (HKL, hkls, hkls_len);
      hkls_size = hkls_len;
    }

  if (hkls_len != GetKeyboardLayoutList (hkls_len, hkls))
    {
      if (!no_list)
        return;

      hkls[0] = GetKeyboardLayout (0);
      hkls_len = 1;
    }

  if (layouts_are_the_same (keymap->layout_handles, hkls, hkls_len))
    {
      check_that_active_layout_is_in_sync (keymap);
      current_serial = _bdk_keymap_serial;

      return;
    }

  BDK_NOTE (EVENTS, g_print ("\nHave %d keyboard layouts:", hkls_len));

  for (i = 0; i < hkls_len; i++)
    {
      BDK_NOTE (EVENTS, g_print (" 0x%p", hkls[i]));

      if (GetKeyboardLayout (0) == hkls[i])
        {
          wchar_t hkl_name[KL_NAMELENGTH];

          if (!GetKeyboardLayoutNameW (hkl_name))
            wcsncpy (hkl_name, L"(NULL)", KL_NAMELENGTH);

          BDK_NOTE (EVENTS, g_print ("(active, %S)", hkl_name));
        }
    }

  BDK_NOTE (EVENTS, g_print ("\n"));

  keysym_tab_size = hkls_len * 256 * 2 * 4;

  if (hkls_len != keymap->layout_handles->len)
    keymap->keysym_tab = g_renew (guint, keymap->keysym_tab, keysym_tab_size);

  memset (keymap->keysym_tab, 0, keysym_tab_size);
  g_array_set_size (keymap->layout_handles, hkls_len);
  g_array_set_size (keymap->options, hkls_len);

  for (i = 0; i < hkls_len; i++)
    {
      options = &g_array_index (keymap->options, BdkWin32KeyGroupOptions, i);

      options->decimal_mark = 0;
      options->scancode_rshift = 0;
      options->has_altgr = FALSE;
      options->dead_keys = g_array_new (FALSE, FALSE, sizeof (BdkWin32KeyNode));
      g_array_set_clear_func (options->dead_keys, (GDestroyNotify) bdk_win32_key_node_clear);

      g_array_index (keymap->layout_handles, HKL, i) = hkls[i];

      if (hkls[i] == _bdk_input_locale)
        keymap->active_layout = i;
    }

  for (vk = 0; vk < KEY_STATE_SIZE; vk++)
    {
      for (group = 0; group < hkls_len; group++)
        {
          options = &g_array_index (keymap->options, BdkWin32KeyGroupOptions, group);
          scancode = MapVirtualKeyEx (vk, 0, hkls[group]);
          keygroup = &keymap->keysym_tab[(vk * hkls_len + group) * BDK_WIN32_LEVEL_COUNT];

          /* MapVirtualKeyEx() fails to produce a scancode for VK_DIVIDE and VK_PAUSE.
           * Ignore that, handle_special() will figure out a Bdk keyval for these
           * without needing a scancode.
           */
          if (scancode == 0 &&
              vk != VK_DIVIDE &&
              vk != VK_PAUSE)
            {
              for (level = BDK_WIN32_LEVEL_NONE; level < BDK_WIN32_LEVEL_COUNT; level++)
                keygroup[level] = BDK_KEY_VoidSymbol;

              continue;
            }

          if (vk == VK_RSHIFT)
            options->scancode_rshift = scancode;

          key_state[vk] = 0x80;

          for (level = BDK_WIN32_LEVEL_NONE; level < BDK_WIN32_LEVEL_COUNT; level++)
            {
              guint *ksymp = &keygroup[level];

              set_level_vks (key_state, level);

              *ksymp = 0;

              /* First, handle those virtual keys that we always want
               * as special BDK_* keysyms, even if ToAsciiEx might
               * turn some them into a ASCII character (like TAB and
               * ESC).
               */
              handle_special (vk, ksymp, level);

              if ((*ksymp == 0) ||
                  ((vk == VK_DECIMAL) && (level == BDK_WIN32_LEVEL_NONE)))
                {
                  wchar_t         wcs[10];
                  gint            k;
                  guint           keysym;
                  BdkWin32KeyNode dead_key;

                  wcs[0] = wcs[1] = 0;
                  k = ToUnicodeEx (vk, scancode, key_state,
                                   wcs, G_N_ELEMENTS (wcs),
                                   0, hkls[group]);
#if 0
                  g_print ("ToUnicodeEx(%#02x, %d: %d): %d, %04x %04x\n",
                           vk, scancode, level, k,
                           wcs[0], wcs[1]);
#endif
                  switch (k)
                    {
                    case 1:
                      if ((vk == VK_DECIMAL) && (level == BDK_WIN32_LEVEL_NONE))
                        options->decimal_mark = wcs[0];
                      else
                        *ksymp = bdk_unicode_to_keyval (wcs[0]);
                      break;
                    case -1:
                      keysym = bdk_unicode_to_keyval (wcs[0]);

                      /* It is a dead key, and it has been stored in
                       * the keyboard layout's state by
                       * ToAsciiEx()/ToUnicodeEx(). Yes, this is an
                       * incredibly silly API! Make the keyboard
                       * layout forget it by calling
                       * ToAsciiEx()/ToUnicodeEx() once more, with the
                       * virtual key code and scancode for the
                       * spacebar, without shift or AltGr. Otherwise
                       * the next call to ToAsciiEx() with a different
                       * key would try to combine with the dead key.
                       */
                      reset_after_dead (key_state, hkls[group]);

                      /* Use dead keysyms instead of "undead" ones */
                      handle_dead (keysym, ksymp);

                      dead_key.undead_bdk_keycode = keysym;
                      dead_key.vk = vk;
                      dead_key.level = level;
                      dead_key.bdk_keycode = *ksymp;
                      dead_key.combinations = NULL;
                      g_array_append_val (options->dead_keys, dead_key);
                      break;
                    case 0:
                      /* Seems to be necessary to "reset" the keyboard layout
                       * in this case, too. Otherwise problems on NT4.
                       */
                      reset_after_dead (key_state, hkls[group]);
                      break;
                    default:
#if 0
                      BDK_NOTE (EVENTS,
                                g_print ("ToUnicodeEx returns %d "
                                         "for vk:%02x, sc:%02x%s%s\n",
                                         k, vk, scancode,
                                         (shift&0x1 ? " shift" : ""),
                                         (shift&0x2 ? " altgr" : "")));
#endif
                      break;
                    }
                }

              if (*ksymp == 0)
                *ksymp = BDK_KEY_VoidSymbol;
            }

          key_state[vk] = 0;

          /* Check if keyboard has an AltGr key by checking if
           * the mapping with Control+Alt is different.
           * Don't test CapsLock here, as it does not seem to affect
           * dead keys themselves, only the results of dead key combinations.
           */
          if (!options->has_altgr)
            if ((keygroup[BDK_WIN32_LEVEL_ALTGR] != BDK_KEY_VoidSymbol &&
                 keygroup[BDK_WIN32_LEVEL_NONE] != keygroup[BDK_WIN32_LEVEL_ALTGR]) ||
                (keygroup[BDK_WIN32_LEVEL_SHIFT_ALTGR] != BDK_KEY_VoidSymbol &&
                 keygroup[BDK_WIN32_LEVEL_SHIFT] != keygroup[BDK_WIN32_LEVEL_SHIFT_ALTGR]))
              options->has_altgr = TRUE;
        }
    }

  scancode = 0x0;

  for (group = 0; group < hkls_len; group++)
    {
      options = &g_array_index (keymap->options, BdkWin32KeyGroupOptions, group);

      for (i = 0; i < options->dead_keys->len; i++)
        {
          wchar_t          wcs[10];
          gint             k;
          BdkWin32KeyNode *dead_key;
          BdkWin32KeyNode  combo;

          dead_key = &g_array_index (options->dead_keys, BdkWin32KeyNode, i);

          for (vk = 0; vk < KEY_STATE_SIZE; vk++)
            {
              for (level = BDK_WIN32_LEVEL_NONE; level < BDK_WIN32_LEVEL_COUNT; level++)
                {
                  /* Prime the ToUnicodeEx() internal state */
                  wcs[0] = wcs[1] = 0;
                  set_level_vks (key_state, dead_key->level);
                  k = ToUnicodeEx (dead_key->vk, scancode, key_state,
                                   wcs, G_N_ELEMENTS (wcs),
                                   0, hkls[group]);
                  switch (k)
                    {
                    case -1:
                      /* Okay */
                      break;
                    default:
                      /* Expected a dead key, got something else */
                      reset_after_dead (key_state, hkls[group]);
                      continue;
                    }

                  /* Check how it combines with vk */
                  wcs[0] = wcs[1] = 0;
                  set_level_vks (key_state, level);
                  k = ToUnicodeEx (vk, scancode, key_state,
                                   wcs, G_N_ELEMENTS (wcs),
                                   0, hkls[group]);

                  if (k == 0)
                    {
                      reset_after_dead (key_state, hkls[group]);
                    }
                  else if (k == -1)
                    {
                      /* Dead key chaining? TODO: support this (deeper tree?) */
                      reset_after_dead (key_state, hkls[group]);
                    }
                  else if (k == 1)
                    {
                      combo.vk = vk;
                      combo.level = level;
                      combo.bdk_keycode = bdk_unicode_to_keyval (wcs[0]);
                      combo.undead_bdk_keycode = combo.bdk_keycode;
                      combo.combinations = NULL;

                      if (dead_key->combinations == NULL)
                        {
                          dead_key->combinations = g_array_new (FALSE, FALSE, sizeof (BdkWin32KeyNode));
                          g_array_set_clear_func (dead_key->combinations, (GDestroyNotify) bdk_win32_key_node_clear);
                        }

#if 0
                      {
                        char *dead_key_undead_u8, *wcs_u8;
                        wchar_t t = bdk_keyval_to_unicode (dead_key->undead_bdk_keycode);
                        dead_key_undead_u8 = g_utf16_to_utf8 (&t, 1, NULL, NULL, NULL);
                        wcs_u8 = g_utf16_to_utf8 (wcs, 1, NULL, NULL, NULL);
                        g_fprintf (stdout, "%d %s%s%s0x%02x (%s) + %s%s%s0x%02x = 0x%04x (%s)\n", group,
                                 (dead_key->level == BDK_WIN32_LEVEL_SHIFT ||
                                  dead_key->level == BDK_WIN32_LEVEL_SHIFT_ALTGR ||
                                  dead_key->level == BDK_WIN32_LEVEL_SHIFT_CAPSLOCK ||
                                  dead_key->level == BDK_WIN32_LEVEL_SHIFT_CAPSLOCK_ALTGR) ? "SHIFT-" : "      ",
                                 (dead_key->level == BDK_WIN32_LEVEL_CAPSLOCK ||
                                  dead_key->level == BDK_WIN32_LEVEL_SHIFT_CAPSLOCK ||
                                  dead_key->level == BDK_WIN32_LEVEL_CAPSLOCK_ALTGR ||
                                  dead_key->level == BDK_WIN32_LEVEL_SHIFT_CAPSLOCK_ALTGR) ? "CAPSLOCK-" : "         ",
                                 (dead_key->level == BDK_WIN32_LEVEL_ALTGR ||
                                  dead_key->level == BDK_WIN32_LEVEL_SHIFT_ALTGR ||
                                  dead_key->level == BDK_WIN32_LEVEL_CAPSLOCK_ALTGR ||
                                  dead_key->level == BDK_WIN32_LEVEL_SHIFT_CAPSLOCK_ALTGR) ? "ALTGR-" : "      ",
                                 dead_key->vk,
                                 dead_key_undead_u8,
                                 (combo.level == BDK_WIN32_LEVEL_SHIFT ||
                                  combo.level == BDK_WIN32_LEVEL_SHIFT_ALTGR ||
                                  combo.level == BDK_WIN32_LEVEL_SHIFT_CAPSLOCK ||
                                  combo.level == BDK_WIN32_LEVEL_SHIFT_CAPSLOCK_ALTGR) ? "SHIFT-" : "      ",
                                 (combo.level == BDK_WIN32_LEVEL_CAPSLOCK ||
                                  combo.level == BDK_WIN32_LEVEL_SHIFT_CAPSLOCK ||
                                  combo.level == BDK_WIN32_LEVEL_CAPSLOCK_ALTGR ||
                                  combo.level == BDK_WIN32_LEVEL_SHIFT_CAPSLOCK_ALTGR) ? "CAPSLOCK-" : "         ",
                                 (combo.level == BDK_WIN32_LEVEL_ALTGR ||
                                  combo.level == BDK_WIN32_LEVEL_SHIFT_ALTGR ||
                                  combo.level == BDK_WIN32_LEVEL_CAPSLOCK_ALTGR ||
                                  combo.level == BDK_WIN32_LEVEL_SHIFT_CAPSLOCK_ALTGR) ? "ALTGR-" : "      ",
                                 vk,
                                 wcs[0],
                                 wcs_u8);
                        g_free (dead_key_undead_u8);
                        g_free (wcs_u8);
                      }
#endif

                      g_array_append_val (dead_key->combinations, combo);
                    }
                }
            }
        }

       g_array_sort (options->dead_keys, (GCompareFunc) sort_key_nodes_by_bdk_keyval);
    }

  BDK_NOTE (EVENTS, print_keysym_tab (keymap));

  check_that_active_layout_is_in_sync (keymap);
  current_serial = _bdk_keymap_serial;
}

static gboolean
find_deadkey_by_keyval (GArray  *dead_keys,
                        guint16  keyval,
                        gsize   *index)
{
  gsize deadkey_i;
  gsize deadkey_i_max;

  if (dead_keys->len == 0)
    return FALSE;

  deadkey_i = 0;
  deadkey_i_max = dead_keys->len - 1;

  while (deadkey_i != deadkey_i_max)
    {
      BdkWin32KeyNode *dead_key;
      gsize middle;

      if (g_array_index (dead_keys, BdkWin32KeyNode, deadkey_i).bdk_keycode == keyval)
        {
          break;
        }
      else if (g_array_index (dead_keys, BdkWin32KeyNode, deadkey_i_max).bdk_keycode == keyval)
        {
          deadkey_i = deadkey_i_max;
          break;
        }
      else if (deadkey_i + 1 == deadkey_i_max)
        {
          break;
        }

      middle = deadkey_i + (deadkey_i_max - deadkey_i) / 2;
      dead_key = &g_array_index (dead_keys, BdkWin32KeyNode, middle);

      if (dead_key->bdk_keycode < keyval)
        deadkey_i = middle;
      else if (dead_key->bdk_keycode > keyval)
        deadkey_i_max = middle;
      else
        deadkey_i = deadkey_i_max = middle;
    }

  if (g_array_index (dead_keys, BdkWin32KeyNode, deadkey_i).bdk_keycode == keyval)
    {
      *index = deadkey_i;

      return TRUE;
    }

  return FALSE;
}

BdkWin32KeymapMatch
bdk_win32_keymap_check_compose (BdkWin32Keymap *keymap,
                                guint          *compose_buffer,
                                gsize           compose_buffer_len,
                                guint16        *output,
                                gsize          *output_len)
{
  gint partial_match;
  guint8 active_group;
  gsize deadkey_i, node_i;
  BdkWin32KeyNode *dead_key;
  BdkWin32KeyGroupOptions *options;
  BdkWin32KeymapMatch match;
  gsize output_size;

  g_return_val_if_fail (output != NULL && output_len != NULL, BDK_WIN32_KEYMAP_MATCH_NONE);

  if (compose_buffer_len < 1)
    return BDK_WIN32_KEYMAP_MATCH_NONE;

  output_size = *output_len;

  active_group = _bdk_win32_keymap_get_active_group (keymap);
  options = &g_array_index (keymap->options, BdkWin32KeyGroupOptions, active_group);

  partial_match = -1;
  match = BDK_WIN32_KEYMAP_MATCH_NONE;

  if (find_deadkey_by_keyval (options->dead_keys, compose_buffer[0], &deadkey_i))
    {
      while (deadkey_i > 0 &&
             g_array_index (options->dead_keys, BdkWin32KeyNode, deadkey_i - 1).bdk_keycode == compose_buffer[0])
        deadkey_i--;

      /* Hardcoded 2-tier tree here (dead key + non dead key = character).
       * TODO: support trees with arbitrary depth for dead key chaining.
       */
      dead_key = &g_array_index (options->dead_keys, BdkWin32KeyNode, deadkey_i);

      /* "Partial match" means "matched the whole sequence except the last key"
       * (right now the sequence only has 2 keys, so this turns into "matched
       * at least the first key").
       * "last key" should be identified by having NULL further combinations.
       * As a heuristic, convert the buffer contents into keyvals and use
       * them as-is (normally there should be a separate unichar buffer for
       * each combination, but we do not store these).
       */
      partial_match = deadkey_i;

      if (compose_buffer_len < 2)
        match = BDK_WIN32_KEYMAP_MATCH_INCOMPLETE;

      for (node_i = 0;
           match != BDK_WIN32_KEYMAP_MATCH_INCOMPLETE &&
           node_i < dead_key->combinations->len;
           node_i++)
        {
          BdkWin32KeyNode *node;

          node = &g_array_index (dead_key->combinations, BdkWin32KeyNode, node_i);

          if (keymap->keysym_tab[(node->vk * keymap->layout_handles->len + active_group) * BDK_WIN32_LEVEL_COUNT + node->level] == compose_buffer[1])
            {
              match = BDK_WIN32_KEYMAP_MATCH_EXACT;
              *output_len = 0;

              if (*output_len < output_size && node->bdk_keycode != 0)
                output[(*output_len)++] = node->bdk_keycode;

              break;
            }
        }
    }

  if (match == BDK_WIN32_KEYMAP_MATCH_EXACT ||
      match == BDK_WIN32_KEYMAP_MATCH_INCOMPLETE)
    {
      return match;
    }

  if (partial_match >= 0)
    {
      if (compose_buffer_len == 2)
        {
          dead_key = &g_array_index (options->dead_keys, BdkWin32KeyNode, partial_match);
          *output_len = 0;

          if (output_size >= 1)
            output[(*output_len)++] = dead_key->undead_bdk_keycode;

          if (output_size >= 2)
            {
              gsize second_deadkey_i;

              /* Special case for "deadkey + deadkey = space-version-of-deadkey, space-version-of-deadkey" combinations.
               * Normally the result is a sequence of 2 unichars, but we do not store this.
               * For "deadkey + nondeadkey = space-version-of-deadkey, nondeadkey", we can use compose_buffer
               * contents as-is, but space version of a dead key need to be looked up separately.
               */
              if (find_deadkey_by_keyval (options->dead_keys, compose_buffer[1], &second_deadkey_i))
                output[(*output_len)++] = g_array_index (options->dead_keys, BdkWin32KeyNode, second_deadkey_i).undead_bdk_keycode;
              else
                output[(*output_len)++] = compose_buffer[1];
            }
        }

      return BDK_WIN32_KEYMAP_MATCH_PARTIAL;
    }

  return BDK_WIN32_KEYMAP_MATCH_NONE;
}

guint8
_bdk_win32_keymap_get_rshift_scancode (BdkWin32Keymap *keymap)
{
  if (keymap != NULL &&
      keymap->layout_handles->len > 0)
    return g_array_index (keymap->options, BdkWin32KeyGroupOptions, keymap->active_layout).scancode_rshift;

  return 0;
}

void
_bdk_win32_keymap_set_active_layout (BdkWin32Keymap *keymap,
                                     HKL             hkl)
{
  if (keymap != NULL &&
      keymap->layout_handles->len > 0)
    {
      gint group;

      for (group = 0; group < keymap->layout_handles->len; group++)
        if (g_array_index (keymap->layout_handles, HKL, group) == hkl)
          keymap->active_layout = group;
    }
}

gboolean
_bdk_win32_keymap_has_altgr (BdkWin32Keymap *keymap)
{
  if (keymap != NULL &&
      keymap->layout_handles->len > 0)
    return g_array_index (keymap->options, BdkWin32KeyGroupOptions, keymap->active_layout).has_altgr;

  return FALSE;
}

guint8
_bdk_win32_keymap_get_active_group (BdkWin32Keymap *keymap)
{
  if (keymap != NULL &&
      keymap->layout_handles->len > 0)
    return keymap->active_layout;

  return 0;
}

BdkKeymap*
bdk_keymap_get_for_display (BdkDisplay *display)
{
  g_return_val_if_fail (display == bdk_display_get_default (), NULL);

  if (default_keymap == NULL)
    default_keymap = g_object_new (bdk_win32_keymap_get_type (), NULL);

  return default_keymap;
}

static BangoDirection
get_hkl_direction (HKL hkl)
{
  switch (PRIMARYLANGID (LOWORD ((DWORD) (gintptr) hkl)))
    {
    case LANG_HEBREW:
    case LANG_ARABIC:
#ifdef LANG_URDU
    case LANG_URDU:
#endif
    case LANG_FARSI:
      /* Others? */
      return BANGO_DIRECTION_RTL;

    default:
      return BANGO_DIRECTION_LTR;
    }
}

BangoDirection
bdk_keymap_get_direction (BdkKeymap *bdk_keymap)
{
  HKL active_hkl;
  BdkWin32Keymap *keymap;

  if (bdk_keymap == NULL || bdk_keymap != bdk_keymap_get_default ())
    keymap = BDK_WIN32_KEYMAP (bdk_keymap_get_default ());
  else
    keymap = BDK_WIN32_KEYMAP (bdk_keymap);

  update_keymap (BDK_KEYMAP (keymap));

  if (keymap->layout_handles->len <= 0)
    active_hkl = GetKeyboardLayout (0);
  else
    active_hkl = g_array_index (keymap->layout_handles, HKL, keymap->active_layout);

  return get_hkl_direction (active_hkl);
}

gboolean
bdk_keymap_have_bidi_layouts (BdkKeymap *bdk_keymap)
{
  BdkWin32Keymap *keymap;
  gboolean        have_rtl = FALSE;
  gboolean        have_ltr = FALSE;
  gint            group;

  if (bdk_keymap == NULL || bdk_keymap != bdk_keymap_get_default ())
    keymap = BDK_WIN32_KEYMAP (bdk_keymap_get_default ());
  else
    keymap = BDK_WIN32_KEYMAP (bdk_keymap);

  update_keymap (BDK_KEYMAP (keymap));

  for (group = 0; group < keymap->layout_handles->len; group++)
    {
      if (get_hkl_direction (g_array_index (keymap->layout_handles, HKL, group)) == BANGO_DIRECTION_RTL)
        have_rtl = TRUE;
      else
        have_ltr = TRUE;
    }

  return have_ltr && have_rtl;
}

gboolean
bdk_keymap_get_caps_lock_state (BdkKeymap *keymap)
{
  (void) keymap;

  return ((GetKeyState (VK_CAPITAL) & 1) != 0);
}

gboolean
bdk_keymap_get_entries_for_keyval (BdkKeymap     *bdk_keymap,
                                   guint          keyval,
                                   BdkKeymapKey **keys,
                                   gint          *n_keys)
{
  GArray *retval;

  g_return_val_if_fail (bdk_keymap == NULL || BDK_IS_KEYMAP (bdk_keymap), FALSE);
  g_return_val_if_fail (keys != NULL, FALSE);
  g_return_val_if_fail (n_keys != NULL, FALSE);
  g_return_val_if_fail (keyval != 0, FALSE);

  retval = g_array_new (FALSE, FALSE, sizeof (BdkKeymapKey));

  /* Accept only the default keymap */
  if (bdk_keymap == NULL || bdk_keymap == bdk_keymap_get_default ())
    {
      gint vk;
      BdkWin32Keymap *keymap;

      if (bdk_keymap == NULL)
        keymap = BDK_WIN32_KEYMAP (bdk_keymap_get_default ());
      else
        keymap = BDK_WIN32_KEYMAP (bdk_keymap);

      update_keymap (bdk_keymap);

      for (vk = 0; vk < KEY_STATE_SIZE; vk++)
        {
          gint group;

          for (group = 0; group < keymap->layout_handles->len; group++)
            {
              BdkWin32KeyLevelState    level;

              for (level = BDK_WIN32_LEVEL_NONE; level < BDK_WIN32_LEVEL_COUNT; level++)
                {
                  guint *keygroup;

                  keygroup = &keymap->keysym_tab[(vk * keymap->layout_handles->len + group) * BDK_WIN32_LEVEL_COUNT];

                  if (keygroup[level] == keyval)
                    {
                      BdkKeymapKey key;

                      key.keycode = vk;
                      key.group = group;
                      key.level = level;
                      g_array_append_val (retval, key);
                    }
                }
            }
        }
    }

#ifdef G_ENABLE_DEBUG
  if (_bdk_debug_flags & BDK_DEBUG_EVENTS)
    {
      guint i;

      g_print ("bdk_keymap_get_entries_for_keyval: %#.04x (%s):",
               keyval, bdk_keyval_name (keyval));
      for (i = 0; i < retval->len; i++)
        {
          BdkKeymapKey *entry = (BdkKeymapKey *) retval->data + i;
          g_print ("  %#.02x %d %d", entry->keycode, entry->group, entry->level);
        }
      g_print ("\n");
    }
#endif

  if (retval->len > 0)
    {
      *keys = (BdkKeymapKey*) retval->data;
      *n_keys = retval->len;
    }
  else
    {
      *keys = NULL;
      *n_keys = 0;
    }

  g_array_free (retval, retval->len > 0 ? FALSE : TRUE);

  return *n_keys > 0;
}

gboolean
bdk_keymap_get_entries_for_keycode (BdkKeymap     *bdk_keymap,
                                    guint          hardware_keycode,
                                    BdkKeymapKey **keys,
                                    guint        **keyvals,
                                    gint          *n_entries)
{
  GArray         *key_array;
  GArray         *keyval_array;
  gint            group;
  BdkWin32Keymap *keymap;

  g_return_val_if_fail (bdk_keymap == NULL || BDK_IS_KEYMAP (bdk_keymap), FALSE);
  g_return_val_if_fail (n_entries != NULL, FALSE);

  if (hardware_keycode <= 0 ||
      hardware_keycode >= KEY_STATE_SIZE ||
      (keys == NULL && keyvals == NULL) ||
      (bdk_keymap != NULL && bdk_keymap != bdk_keymap_get_default ()))
    {
      /* Wrong keycode or NULL output arrays or wrong keymap */
      if (keys)
        *keys = NULL;
      if (keyvals)
        *keyvals = NULL;

      *n_entries = 0;
      return FALSE;
    }

  if (keys)
    key_array = g_array_new (FALSE, FALSE, sizeof (BdkKeymapKey));
  else
    key_array = NULL;

  if (keyvals)
    keyval_array = g_array_new (FALSE, FALSE, sizeof (guint));
  else
    keyval_array = NULL;

  keymap = BDK_WIN32_KEYMAP (bdk_keymap_get_default ());
  update_keymap (BDK_KEYMAP (keymap));

  for (group = 0; group < keymap->layout_handles->len; group++)
    {
      BdkWin32KeyLevelState    level;

      for (level = BDK_WIN32_LEVEL_NONE; level < BDK_WIN32_LEVEL_COUNT; level++)
        {
          if (key_array)
            {
              BdkKeymapKey key;

              key.keycode = hardware_keycode;
              key.group = group;
              key.level = level;
              g_array_append_val (key_array, key);
            }

          if (keyval_array)
            {
              guint keyval = keymap->keysym_tab[(hardware_keycode * keymap->layout_handles->len + group) * BDK_WIN32_LEVEL_COUNT + level];

              g_array_append_val (keyval_array, keyval);
            }
        }
    }

  *n_entries = group * BDK_WIN32_LEVEL_COUNT;

  if ((key_array && key_array->len > 0) ||
      (keyval_array && keyval_array->len > 0))
    {
      if (keys)
        *keys = (BdkKeymapKey*) key_array->data;

      if (keyvals)
        *keyvals = (guint*) keyval_array->data;
    }
  else
    {
      if (keys)
        *keys = NULL;

      if (keyvals)
        *keyvals = NULL;
    }

  if (key_array)
    g_array_free (key_array, key_array->len > 0 ? FALSE : TRUE);
  if (keyval_array)
    g_array_free (keyval_array, keyval_array->len > 0 ? FALSE : TRUE);

  return *n_entries > 0;
}

guint
bdk_keymap_lookup_key (BdkKeymap          *bdk_keymap,
                       const BdkKeymapKey *key)
{
  guint sym;
  BdkWin32Keymap *keymap;

  g_return_val_if_fail (bdk_keymap == NULL || BDK_IS_KEYMAP (bdk_keymap), 0);
  g_return_val_if_fail (key != NULL, 0);

  /* Accept only the default keymap */
  if (bdk_keymap != NULL && bdk_keymap != bdk_keymap_get_default ())
    return 0;

  keymap = BDK_WIN32_KEYMAP (bdk_keymap_get_default ());
  update_keymap (BDK_KEYMAP (keymap));

  if (key->keycode >= KEY_STATE_SIZE ||
      key->group < 0 || key->group >= keymap->layout_handles->len ||
      key->level < 0 || key->level >= BDK_WIN32_LEVEL_COUNT)
    return 0;

  sym = keymap->keysym_tab[(key->keycode * keymap->layout_handles->len + key->group) * BDK_WIN32_LEVEL_COUNT + key->level];

  if (sym == BDK_KEY_VoidSymbol)
    return 0;
  else
    return sym;
}

gboolean
bdk_keymap_translate_keyboard_state (BdkKeymap       *bdk_keymap,
                                     guint            hardware_keycode,
                                     BdkModifierType  state,
                                     gint             group,
                                     guint           *keyval,
                                     gint            *effective_group,
                                     gint            *level,
                                     BdkModifierType *consumed_modifiers)
{
  BdkWin32Keymap *keymap;
  guint tmp_keyval;
  guint *keygroup;
  BdkWin32KeyLevelState shift_level;
  BdkModifierType modifiers = BDK_SHIFT_MASK | BDK_LOCK_MASK | BDK_MOD2_MASK;

  g_return_val_if_fail (bdk_keymap == NULL || BDK_IS_KEYMAP (bdk_keymap), FALSE);

#if 0
  BDK_NOTE (EVENTS, g_print ("bdk_keymap_translate_keyboard_state: keycode=%#x state=%#x group=%d\n",
			     hardware_keycode, state, group));
#endif
  if (keyval)
    *keyval = 0;
  if (effective_group)
    *effective_group = 0;
  if (level)
    *level = 0;
  if (consumed_modifiers)
    *consumed_modifiers = 0;

  /* Accept only the default keymap */
  if (bdk_keymap != NULL && bdk_keymap != bdk_keymap_get_default ())
    return FALSE;

  if (hardware_keycode >= KEY_STATE_SIZE)
    return FALSE;

  keymap = BDK_WIN32_KEYMAP (bdk_keymap_get_default ());
  update_keymap (BDK_KEYMAP (keymap));

  if (group < 0 || group >= keymap->layout_handles->len)
    return FALSE;

  keygroup = &keymap->keysym_tab[(hardware_keycode * keymap->layout_handles->len + group) * BDK_WIN32_LEVEL_COUNT];

  if ((state & (BDK_SHIFT_MASK | BDK_LOCK_MASK)) == (BDK_SHIFT_MASK | BDK_LOCK_MASK))
    shift_level = BDK_WIN32_LEVEL_SHIFT_CAPSLOCK;
  else if (state & BDK_SHIFT_MASK)
    shift_level = BDK_WIN32_LEVEL_SHIFT;
  else if (state & BDK_LOCK_MASK)
    shift_level = BDK_WIN32_LEVEL_CAPSLOCK;
  else
    shift_level = BDK_WIN32_LEVEL_NONE;

  if (state & BDK_MOD2_MASK)
    {
      if (shift_level == BDK_WIN32_LEVEL_NONE)
        shift_level = BDK_WIN32_LEVEL_ALTGR;
      else if (shift_level == BDK_WIN32_LEVEL_SHIFT)
        shift_level = BDK_WIN32_LEVEL_SHIFT_ALTGR;
      else if (shift_level == BDK_WIN32_LEVEL_CAPSLOCK)
        shift_level = BDK_WIN32_LEVEL_CAPSLOCK_ALTGR;
      else
        shift_level = BDK_WIN32_LEVEL_SHIFT_CAPSLOCK_ALTGR;
     }

  /* Drop altgr, capslock and shift if there are no keysymbols on
   * the key for those.
   */
  if (keygroup[shift_level] == BDK_KEY_VoidSymbol)
    {
      switch (shift_level)
        {
         case BDK_WIN32_LEVEL_NONE:
         case BDK_WIN32_LEVEL_ALTGR:
         case BDK_WIN32_LEVEL_SHIFT:
         case BDK_WIN32_LEVEL_CAPSLOCK:
           if (keygroup[BDK_WIN32_LEVEL_NONE] != BDK_KEY_VoidSymbol)
             shift_level = BDK_WIN32_LEVEL_NONE;
           break;
         case BDK_WIN32_LEVEL_SHIFT_CAPSLOCK:
           if (keygroup[BDK_WIN32_LEVEL_CAPSLOCK] != BDK_KEY_VoidSymbol)
             shift_level = BDK_WIN32_LEVEL_CAPSLOCK;
           else if (keygroup[BDK_WIN32_LEVEL_SHIFT] != BDK_KEY_VoidSymbol)
             shift_level = BDK_WIN32_LEVEL_SHIFT;
           else if (keygroup[BDK_WIN32_LEVEL_NONE] != BDK_KEY_VoidSymbol)
             shift_level = BDK_WIN32_LEVEL_NONE;
           break;
         case BDK_WIN32_LEVEL_CAPSLOCK_ALTGR:
           if (keygroup[BDK_WIN32_LEVEL_ALTGR] != BDK_KEY_VoidSymbol)
             shift_level = BDK_WIN32_LEVEL_ALTGR;
           else if (keygroup[BDK_WIN32_LEVEL_CAPSLOCK] != BDK_KEY_VoidSymbol)
             shift_level = BDK_WIN32_LEVEL_CAPSLOCK;
           else if (keygroup[BDK_WIN32_LEVEL_NONE] != BDK_KEY_VoidSymbol)
             shift_level = BDK_WIN32_LEVEL_NONE;
           break;
         case BDK_WIN32_LEVEL_SHIFT_ALTGR:
           if (keygroup[BDK_WIN32_LEVEL_ALTGR] != BDK_KEY_VoidSymbol)
             shift_level = BDK_WIN32_LEVEL_ALTGR;
           else if (keygroup[BDK_WIN32_LEVEL_SHIFT] != BDK_KEY_VoidSymbol)
             shift_level = BDK_WIN32_LEVEL_SHIFT;
           else if (keygroup[BDK_WIN32_LEVEL_NONE] != BDK_KEY_VoidSymbol)
             shift_level = BDK_WIN32_LEVEL_NONE;
           break;
         case BDK_WIN32_LEVEL_SHIFT_CAPSLOCK_ALTGR:
           if (keygroup[BDK_WIN32_LEVEL_CAPSLOCK_ALTGR] != BDK_KEY_VoidSymbol)
             shift_level = BDK_WIN32_LEVEL_CAPSLOCK_ALTGR;
           else if (keygroup[BDK_WIN32_LEVEL_SHIFT_ALTGR] != BDK_KEY_VoidSymbol)
             shift_level = BDK_WIN32_LEVEL_SHIFT_ALTGR;
           else if (keygroup[BDK_WIN32_LEVEL_ALTGR] != BDK_KEY_VoidSymbol)
             shift_level = BDK_WIN32_LEVEL_ALTGR;
           else if (keygroup[BDK_WIN32_LEVEL_SHIFT_CAPSLOCK] != BDK_KEY_VoidSymbol)
             shift_level = BDK_WIN32_LEVEL_SHIFT_CAPSLOCK;
           else if (keygroup[BDK_WIN32_LEVEL_CAPSLOCK] != BDK_KEY_VoidSymbol)
             shift_level = BDK_WIN32_LEVEL_CAPSLOCK;
           else if (keygroup[BDK_WIN32_LEVEL_SHIFT] != BDK_KEY_VoidSymbol)
             shift_level = BDK_WIN32_LEVEL_SHIFT;
           else if (keygroup[BDK_WIN32_LEVEL_NONE] != BDK_KEY_VoidSymbol)
             shift_level = BDK_WIN32_LEVEL_NONE;
           break;
         case BDK_WIN32_LEVEL_COUNT:
           g_assert_not_reached ();
        }
    }

  /* See whether the shift level actually mattered
   * to know what to put in consumed_modifiers
   */
  if ((keygroup[BDK_WIN32_LEVEL_SHIFT] == BDK_KEY_VoidSymbol ||
       keygroup[BDK_WIN32_LEVEL_NONE] == keygroup[BDK_WIN32_LEVEL_SHIFT]) &&
      (keygroup[BDK_WIN32_LEVEL_SHIFT_ALTGR] == BDK_KEY_VoidSymbol ||
       keygroup[BDK_WIN32_LEVEL_ALTGR] == keygroup[BDK_WIN32_LEVEL_SHIFT_ALTGR]) &&
      (keygroup[BDK_WIN32_LEVEL_SHIFT_CAPSLOCK] == BDK_KEY_VoidSymbol ||
       keygroup[BDK_WIN32_LEVEL_CAPSLOCK] == keygroup[BDK_WIN32_LEVEL_SHIFT_CAPSLOCK]))
      modifiers &= ~BDK_SHIFT_MASK;

  if ((keygroup[BDK_WIN32_LEVEL_CAPSLOCK] == BDK_KEY_VoidSymbol ||
       keygroup[BDK_WIN32_LEVEL_NONE] == keygroup[BDK_WIN32_LEVEL_CAPSLOCK]) &&
      (keygroup[BDK_WIN32_LEVEL_CAPSLOCK_ALTGR] == BDK_KEY_VoidSymbol ||
       keygroup[BDK_WIN32_LEVEL_ALTGR] == keygroup[BDK_WIN32_LEVEL_CAPSLOCK_ALTGR]) &&
      (keygroup[BDK_WIN32_LEVEL_SHIFT_CAPSLOCK] == BDK_KEY_VoidSymbol ||
       keygroup[BDK_WIN32_LEVEL_SHIFT] == keygroup[BDK_WIN32_LEVEL_SHIFT_CAPSLOCK]))
      modifiers &= ~BDK_LOCK_MASK;

  if ((keygroup[BDK_WIN32_LEVEL_ALTGR] == BDK_KEY_VoidSymbol ||
       keygroup[BDK_WIN32_LEVEL_NONE] == keygroup[BDK_WIN32_LEVEL_ALTGR]) &&
      (keygroup[BDK_WIN32_LEVEL_SHIFT_ALTGR] == BDK_KEY_VoidSymbol ||
       keygroup[BDK_WIN32_LEVEL_SHIFT] == keygroup[BDK_WIN32_LEVEL_SHIFT_ALTGR]) &&
      (keygroup[BDK_WIN32_LEVEL_CAPSLOCK_ALTGR] == BDK_KEY_VoidSymbol ||
       keygroup[BDK_WIN32_LEVEL_CAPSLOCK] == keygroup[BDK_WIN32_LEVEL_CAPSLOCK_ALTGR]))
      modifiers &= ~BDK_MOD2_MASK;

  tmp_keyval = keygroup[shift_level];

  if (keyval)
    *keyval = tmp_keyval;

  if (effective_group)
    *effective_group = group;

  if (level)
    *level = shift_level;

  if (consumed_modifiers)
    *consumed_modifiers = modifiers;

#if 0
  BDK_NOTE (EVENTS, g_print ("... group=%d level=%d cmods=%#x keyval=%s\n",
			     group, shift_level, modifiers, bdk_keyval_name (tmp_keyval)));
#endif

  return tmp_keyval != BDK_KEY_VoidSymbol;
}

void
bdk_keymap_add_virtual_modifiers (BdkKeymap       *keymap,
                                  BdkModifierType *state)
{
}

gboolean
bdk_keymap_map_virtual_modifiers (BdkKeymap       *keymap,
                                  BdkModifierType *state)
{
  /* FIXME: Is this the right thing to do? */
  return TRUE;
}

static void
bdk_win32_keymap_class_init (BdkWin32KeymapClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bdk_win32_keymap_finalize;

}
