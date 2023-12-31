/* bdkkeys-quartz.c
 *
 * Copyright (C) 2000 Red Hat, Inc.
 * Copyright (C) 2005 Imendio AB
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
/* Some parts of this code come from quartzKeyboard.c,
 * from the Apple X11 Server.
 *
 * Copyright (c) 2003 Apple Computer, Inc. All rights reserved.
 *
 *  Permission is hereby granted, free of charge, to any person
 *  obtaining a copy of this software and associated documentation files
 *  (the "Software"), to deal in the Software without restriction,
 *  including without limitation the rights to use, copy, modify, merge,
 *  publish, distribute, sublicense, and/or sell copies of the Software,
 *  and to permit persons to whom the Software is furnished to do so,
 *  subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT.  IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT
 *  HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 *  Except as contained in this notice, the name(s) of the above
 *  copyright holders shall not be used in advertising or otherwise to
 *  promote the sale, use or other dealings in this Software without
 *  prior written authorization.
 */

#include "config.h"

#include <Carbon/Carbon.h>
#include <AppKit/NSEvent.h>
#include "bdk.h"
#include "bdkkeysyms.h"

#define NUM_KEYCODES 128
#define KEYVALS_PER_KEYCODE 4

static BdkKeymap *default_keymap = NULL;

/* This is a table of all keyvals. Each keycode gets KEYVALS_PER_KEYCODE entries.
 * TThere is 1 keyval per modifier (Nothing, Shift, Alt, Shift+Alt);
 */
static buint *keyval_array = NULL;

static inline UniChar
macroman2ucs (unsigned char c)
{
  /* Precalculated table mapping MacRoman-128 to Unicode. Generated
     by creating single element CFStringRefs then extracting the
     first character. */
  
  static const unsigned short table[128] = {
    0xc4, 0xc5, 0xc7, 0xc9, 0xd1, 0xd6, 0xdc, 0xe1,
    0xe0, 0xe2, 0xe4, 0xe3, 0xe5, 0xe7, 0xe9, 0xe8,
    0xea, 0xeb, 0xed, 0xec, 0xee, 0xef, 0xf1, 0xf3,
    0xf2, 0xf4, 0xf6, 0xf5, 0xfa, 0xf9, 0xfb, 0xfc,
    0x2020, 0xb0, 0xa2, 0xa3, 0xa7, 0x2022, 0xb6, 0xdf,
    0xae, 0xa9, 0x2122, 0xb4, 0xa8, 0x2260, 0xc6, 0xd8,
    0x221e, 0xb1, 0x2264, 0x2265, 0xa5, 0xb5, 0x2202, 0x2211,
    0x220f, 0x3c0, 0x222b, 0xaa, 0xba, 0x3a9, 0xe6, 0xf8,
    0xbf, 0xa1, 0xac, 0x221a, 0x192, 0x2248, 0x2206, 0xab,
    0xbb, 0x2026, 0xa0, 0xc0, 0xc3, 0xd5, 0x152, 0x153,
    0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0xf7, 0x25ca,
    0xff, 0x178, 0x2044, 0x20ac, 0x2039, 0x203a, 0xfb01, 0xfb02,
    0x2021, 0xb7, 0x201a, 0x201e, 0x2030, 0xc2, 0xca, 0xc1,
    0xcb, 0xc8, 0xcd, 0xce, 0xcf, 0xcc, 0xd3, 0xd4,
    0xf8ff, 0xd2, 0xda, 0xdb, 0xd9, 0x131, 0x2c6, 0x2dc,
    0xaf, 0x2d8, 0x2d9, 0x2da, 0xb8, 0x2dd, 0x2db, 0x2c7
  };

  if (c < 128)
    return c;
  else
    return table[c - 128];
}

const static struct {
  buint keycode;
  buint keyval;
  unsigned int modmask; /* So we can tell when a mod key is pressed/released */
} modifier_keys[] = {
  {  54, BDK_Meta_R,    NSCommandKeyMask },
  {  55, BDK_Meta_L,    NSCommandKeyMask },
  {  56, BDK_Shift_L,   NSShiftKeyMask },
  {  57, BDK_Caps_Lock, NSAlphaShiftKeyMask },
  {  58, BDK_Alt_L,     NSAlternateKeyMask },
  {  59, BDK_Control_L, NSControlKeyMask },
  {  60, BDK_Shift_R,   NSShiftKeyMask },
  {  61, BDK_Alt_R,     NSAlternateKeyMask },
  {  62, BDK_Control_R, NSControlKeyMask }
};

const static struct {
  buint keycode;
  buint keyval;
} function_keys[] = {
  { 122, BDK_F1 },
  { 120, BDK_F2 },
  {  99, BDK_F3 },
  { 118, BDK_F4 },
  {  96, BDK_F5 },
  {  97, BDK_F6 },
  {  98, BDK_F7 },
  { 100, BDK_F8 },
  { 101, BDK_F9 },
  { 109, BDK_F10 },
  { 103, BDK_F11 },
  { 111, BDK_F12 },
  { 105, BDK_F13 },
  { 107, BDK_F14 },
  { 113, BDK_F15 },
  { 106, BDK_F16 }
};

const static struct {
  buint keycode;
  buint normal_keyval, keypad_keyval;
} known_numeric_keys[] = {
  { 65, BDK_period, BDK_KP_Decimal },
  { 67, BDK_asterisk, BDK_KP_Multiply },
  { 69, BDK_plus, BDK_KP_Add },
  { 75, BDK_slash, BDK_KP_Divide },
  { 76, BDK_Return, BDK_KP_Enter },
  { 78, BDK_minus, BDK_KP_Subtract },
  { 81, BDK_equal, BDK_KP_Equal },
  { 82, BDK_0, BDK_KP_0 },
  { 83, BDK_1, BDK_KP_1 },
  { 84, BDK_2, BDK_KP_2 },
  { 85, BDK_3, BDK_KP_3 },
  { 86, BDK_4, BDK_KP_4 },
  { 87, BDK_5, BDK_KP_5 },
  { 88, BDK_6, BDK_KP_6 },
  { 89, BDK_7, BDK_KP_7 },
  { 91, BDK_8, BDK_KP_8 },
  { 92, BDK_9, BDK_KP_9 }
};

/* These values aren't covered by bdk_unicode_to_keyval */
const static struct {
  gunichar ucs_value;
  buint keyval;
} special_ucs_table [] = {
  { 0x0001, BDK_Home },
  { 0x0003, BDK_Return },
  { 0x0004, BDK_End },
  { 0x0008, BDK_BackSpace },
  { 0x0009, BDK_Tab },
  { 0x000b, BDK_Page_Up },
  { 0x000c, BDK_Page_Down },
  { 0x000d, BDK_Return },
  { 0x001b, BDK_Escape },
  { 0x001c, BDK_Left },
  { 0x001d, BDK_Right },
  { 0x001e, BDK_Up },
  { 0x001f, BDK_Down },
  { 0x007f, BDK_Delete },
  { 0xf027, BDK_dead_acute },
  { 0xf060, BDK_dead_grave },
  { 0xf300, BDK_dead_grave },
  { 0xf0b4, BDK_dead_acute },
  { 0xf301, BDK_dead_acute },
  { 0xf385, BDK_dead_acute },
  { 0xf05e, BDK_dead_circumflex },
  { 0xf2c6, BDK_dead_circumflex },
  { 0xf302, BDK_dead_circumflex },
  { 0xf07e, BDK_dead_tilde },
  { 0xf2dc, BDK_dead_tilde },
  { 0xf303, BDK_dead_tilde },
  { 0xf342, BDK_dead_perispomeni },
  { 0xf0af, BDK_dead_macron },
  { 0xf304, BDK_dead_macron },
  { 0xf2d8, BDK_dead_breve },
  { 0xf306, BDK_dead_breve },
  { 0xf2d9, BDK_dead_abovedot },
  { 0xf307, BDK_dead_abovedot },
  { 0xf0a8, BDK_dead_diaeresis },
  { 0xf308, BDK_dead_diaeresis },
  { 0xf2da, BDK_dead_abovering },
  { 0xf30A, BDK_dead_abovering },
  { 0xf022, BDK_dead_doubleacute },
  { 0xf2dd, BDK_dead_doubleacute },
  { 0xf30B, BDK_dead_doubleacute },
  { 0xf2c7, BDK_dead_caron },
  { 0xf30C, BDK_dead_caron },
  { 0xf0be, BDK_dead_cedilla },
  { 0xf327, BDK_dead_cedilla },
  { 0xf2db, BDK_dead_ogonek },
  { 0xf328, BDK_dead_ogonek },
  { 0xfe5d, BDK_dead_iota },
  { 0xf323, BDK_dead_belowdot },
  { 0xf309, BDK_dead_hook },
  { 0xf31B, BDK_dead_horn },
  { 0xf02d, BDK_dead_stroke },
  { 0xf335, BDK_dead_stroke },
  { 0xf336, BDK_dead_stroke },
  { 0xf313, BDK_dead_abovecomma },
  /*  { 0xf313, BDK_dead_psili }, */
  { 0xf314, BDK_dead_abovereversedcomma },
  /*  { 0xf314, BDK_dead_dasia }, */
  { 0xf30F, BDK_dead_doublegrave },
  { 0xf325, BDK_dead_belowring },
  { 0xf2cd, BDK_dead_belowmacron },
  { 0xf331, BDK_dead_belowmacron },
  { 0xf32D, BDK_dead_belowcircumflex },
  { 0xf330, BDK_dead_belowtilde },
  { 0xf32E, BDK_dead_belowbreve },
  { 0xf324, BDK_dead_belowdiaeresis },
  { 0xf311, BDK_dead_invertedbreve },
  { 0xf02c, BDK_dead_belowcomma },
  { 0xf326, BDK_dead_belowcomma }
};

static void
update_keymap (void)
{
  const void *chr_data = NULL;
  buint *p;
  int i;

  /* Note: we could check only if building against the 10.5 SDK instead, but
   * that would make non-xml layouts not work in 32-bit which would be a quite
   * bad regression. This way, old unsupported layouts will just not work in
   * 64-bit.
   */
#ifdef __LP64__
  TISInputSourceRef new_layout = TISCopyCurrentKeyboardLayoutInputSource ();
  CFDataRef layout_data_ref;

#else
  KeyboardLayoutRef new_layout;
  KeyboardLayoutKind layout_kind;

  KLGetCurrentKeyboardLayout (&new_layout);
#endif

  g_free (keyval_array);
  keyval_array = g_new0 (buint, NUM_KEYCODES * KEYVALS_PER_KEYCODE);

#ifdef __LP64__
  layout_data_ref = (CFDataRef) TISGetInputSourceProperty
    (new_layout, kTISPropertyUnicodeKeyLayoutData);

  if (layout_data_ref)
    chr_data = CFDataGetBytePtr (layout_data_ref);

  if (chr_data == NULL)
    {
      g_error ("cannot get keyboard layout data");
      return;
    }
#else
  /* Get the layout kind */
  KLGetKeyboardLayoutProperty (new_layout, kKLKind, (const void **)&layout_kind);

  /* 8-bit-only keyabord layout */
  if (layout_kind == kKLKCHRKind)
    {
      /* Get chr data */
      KLGetKeyboardLayoutProperty (new_layout, kKLKCHRData, (const void **)&chr_data);

      for (i = 0; i < NUM_KEYCODES; i++)
        {
          int j;
          UInt32 modifiers[] = {0, shiftKey, optionKey, shiftKey | optionKey};

          p = keyval_array + i * KEYVALS_PER_KEYCODE;

          for (j = 0; j < KEYVALS_PER_KEYCODE; j++)
            {
              UInt32 c, state = 0;
              UInt16 key_code;
              UniChar uc;

              key_code = modifiers[j] | i;
              c = KeyTranslate (chr_data, key_code, &state);

              if (state != 0)
                {
                  UInt32 state2 = 0;
                  c = KeyTranslate (chr_data, key_code | 128, &state2);
                }

              if (c != 0 && c != 0x10)
                {
                  int k;
                  bboolean found = FALSE;

                  /* FIXME: some keyboard layouts (e.g. Russian) use a
                   * different 8-bit character set. We should check
                   * for this. Not a serious problem, because most
                   * (all?) of these layouts also have a uchr version.
                   */
                  uc = macroman2ucs (c);

                  for (k = 0; k < G_N_ELEMENTS (special_ucs_table); k++)
                    {
                      if (special_ucs_table[k].ucs_value == uc)
                        {
                          p[j] = special_ucs_table[k].keyval;
                          found = TRUE;
                          break;
                        }
                    }

                  /* Special-case shift-tab since BTK+ expects
                   * BDK_ISO_Left_Tab for that.
                   */
                  if (found && p[j] == BDK_Tab && modifiers[j] == shiftKey)
                    p[j] = BDK_ISO_Left_Tab;

                  if (!found)
                    p[j] = bdk_unicode_to_keyval (uc);
                }
            }

          if (p[3] == p[2])
            p[3] = 0;
          if (p[2] == p[1])
            p[2] = 0;
          if (p[1] == p[0])
            p[1] = 0;
          if (p[0] == p[2] &&
              p[1] == p[3])
            p[2] = p[3] = 0;
        }
    }
  /* unicode keyboard layout */
  else if (layout_kind == kKLKCHRuchrKind || layout_kind == kKLuchrKind)
    {
      /* Get chr data */
      KLGetKeyboardLayoutProperty (new_layout, kKLuchrData, (const void **)&chr_data);
#endif

      for (i = 0; i < NUM_KEYCODES; i++)
        {
          int j;
          UInt32 modifiers[] = {0, shiftKey, optionKey, shiftKey | optionKey};
          UniChar chars[4];
          UniCharCount nChars;

          p = keyval_array + i * KEYVALS_PER_KEYCODE;

          for (j = 0; j < KEYVALS_PER_KEYCODE; j++)
            {
              UInt32 state = 0;
              OSStatus err;
              UInt16 key_code;
              UniChar uc;

              key_code = modifiers[j] | i;
              err = UCKeyTranslate (chr_data, i, kUCKeyActionDisplay,
                                    (modifiers[j] >> 8) & 0xFF,
                                    LMGetKbdType(),
                                    0,
                                    &state, 4, &nChars, chars);

              /* FIXME: Theoretically, we can get multiple UTF-16
               * values; we should convert them to proper unicode and
               * figure out whether there are really keyboard layouts
               * that give us more than one character for one
               * keypress.
               */
              if (err == noErr && nChars == 1)
                {
                  int k;
                  bboolean found = FALSE;

                  /* A few <Shift><Option>keys return two characters,
                   * the first of which is U+00a0, which isn't
                   * interesting; so we return the second. More
                   * sophisticated handling is the job of a
                   * BtkIMContext.
                   *
                   * If state isn't zero, it means that it's a dead
                   * key of some sort. Some of those are enumerated in
                   * the special_ucs_table with the high nibble set to
                   * f to push it into the private use range. Here we
                   * do the same.
                   */
                  if (state != 0)
                    chars[nChars - 1] |= 0xf000;
                  uc = chars[nChars - 1];

                  for (k = 0; k < G_N_ELEMENTS (special_ucs_table); k++)
                    {
                      if (special_ucs_table[k].ucs_value == uc)
                        {
                          p[j] = special_ucs_table[k].keyval;
                          found = TRUE;
                          break;
                        }
                    }

                  /* Special-case shift-tab since BTK+ expects
                   * BDK_ISO_Left_Tab for that.
                   */
                  if (found && p[j] == BDK_Tab && modifiers[j] == shiftKey)
                    p[j] = BDK_ISO_Left_Tab;

                  if (!found)
                    p[j] = bdk_unicode_to_keyval (uc);
                }
            }

          if (p[3] == p[2])
            p[3] = 0;
          if (p[2] == p[1])
            p[2] = 0;
          if (p[1] == p[0])
            p[1] = 0;
          if (p[0] == p[2] &&
              p[1] == p[3])
            p[2] = p[3] = 0;
        }
#ifndef __LP64__
    }
  else
    {
      g_error ("unknown type of keyboard layout (neither KCHR nor uchr)"
               " - not supported right now");
    }
#endif

  for (i = 0; i < G_N_ELEMENTS (modifier_keys); i++)
    {
      p = keyval_array + modifier_keys[i].keycode * KEYVALS_PER_KEYCODE;

      if (p[0] == 0 && p[1] == 0 &&
          p[2] == 0 && p[3] == 0)
        p[0] = modifier_keys[i].keyval;
    }

  for (i = 0; i < G_N_ELEMENTS (function_keys); i++)
    {
      p = keyval_array + function_keys[i].keycode * KEYVALS_PER_KEYCODE;

      p[0] = function_keys[i].keyval;
      p[1] = p[2] = p[3] = 0;
    }

  for (i = 0; i < G_N_ELEMENTS (known_numeric_keys); i++)
    {
      p = keyval_array + known_numeric_keys[i].keycode * KEYVALS_PER_KEYCODE;

      if (p[0] == known_numeric_keys[i].normal_keyval)
        p[0] = known_numeric_keys[i].keypad_keyval;
    }

  g_signal_emit_by_name (default_keymap, "keys-changed");
}

static void
input_sources_changed_notification (CFNotificationCenterRef  center,
                                    void                    *observer,
                                    CFStringRef              name,
                                    const void              *object,
                                    CFDictionaryRef          userInfo)
{
  update_keymap ();
}

BdkKeymap *
bdk_keymap_get_for_display (BdkDisplay *display)
{
  g_return_val_if_fail (display == bdk_display_get_default (), NULL);

  if (default_keymap == NULL)
    {
      default_keymap = g_object_new (bdk_keymap_get_type (), NULL);
      update_keymap ();
      CFNotificationCenterAddObserver (CFNotificationCenterGetDistributedCenter (),
                                       NULL,
                                       input_sources_changed_notification,
                                       CFSTR ("AppleSelectedInputSourcesChangedNotification"),
                                       NULL,
                                       CFNotificationSuspensionBehaviorDeliverImmediately);
    }

  return default_keymap;
}

BangoDirection
bdk_keymap_get_direction (BdkKeymap *keymap)
{
  return BANGO_DIRECTION_NEUTRAL;
}

bboolean
bdk_keymap_have_bidi_layouts (BdkKeymap *keymap)
{
  /* FIXME: Can we implement this? */
  return FALSE;
}

bboolean
bdk_keymap_get_caps_lock_state (BdkKeymap *keymap)
{
  /* FIXME: Implement this. */
  return FALSE;
}

bboolean
bdk_keymap_get_entries_for_keyval (BdkKeymap     *keymap,
                                   buint          keyval,
                                   BdkKeymapKey **keys,
                                   bint          *n_keys)
{
  GArray *keys_array;
  int i;

  g_return_val_if_fail (keymap == NULL || BDK_IS_KEYMAP (keymap), FALSE);
  g_return_val_if_fail (keys != NULL, FALSE);
  g_return_val_if_fail (n_keys != NULL, FALSE);
  g_return_val_if_fail (keyval != 0, FALSE);

  if (!keymap)
    keymap = bdk_keymap_get_for_display (bdk_display_get_default ());

  *n_keys = 0;
  keys_array = g_array_new (FALSE, FALSE, sizeof (BdkKeymapKey));

  for (i = 0; i < NUM_KEYCODES * KEYVALS_PER_KEYCODE; i++)
    {
      BdkKeymapKey key;

      if (keyval_array[i] != keyval)
	continue;

      (*n_keys)++;

      key.keycode = i / KEYVALS_PER_KEYCODE;
      key.group = (i % KEYVALS_PER_KEYCODE) >= 2;
      key.level = i % 2;

      g_array_append_val (keys_array, key);
    }

  *keys = (BdkKeymapKey *)g_array_free (keys_array, FALSE);
  
  return *n_keys > 0;;
}

bboolean
bdk_keymap_get_entries_for_keycode (BdkKeymap     *keymap,
                                    buint          hardware_keycode,
                                    BdkKeymapKey **keys,
                                    buint        **keyvals,
                                    bint          *n_entries)
{
  GArray *keys_array, *keyvals_array;
  int i;
  buint *p;

  g_return_val_if_fail (keymap == NULL || BDK_IS_KEYMAP (keymap), FALSE);
  g_return_val_if_fail (n_entries != NULL, FALSE);

  if (!keymap)
    keymap = bdk_keymap_get_for_display (bdk_display_get_default ());

  *n_entries = 0;

  if (hardware_keycode > NUM_KEYCODES)
    return FALSE;

  if (keys)
    keys_array = g_array_new (FALSE, FALSE, sizeof (BdkKeymapKey));
  else
    keys_array = NULL;

  if (keyvals)
    keyvals_array = g_array_new (FALSE, FALSE, sizeof (buint));
  else
    keyvals_array = NULL;

  p = keyval_array + hardware_keycode * KEYVALS_PER_KEYCODE;
  
  for (i = 0; i < KEYVALS_PER_KEYCODE; i++)
    {
      if (!p[i])
	continue;

      (*n_entries)++;
      
      if (keyvals_array)
	g_array_append_val (keyvals_array, p[i]);

      if (keys_array)
	{
	  BdkKeymapKey key;

	  key.keycode = hardware_keycode;
	  key.group = i >= 2;
	  key.level = i % 2;

	  g_array_append_val (keys_array, key);
	}
    }
  
  if (keys)
    *keys = (BdkKeymapKey *)g_array_free (keys_array, FALSE);

  if (keyvals)
    *keyvals = (buint *)g_array_free (keyvals_array, FALSE);

  return *n_entries > 0;
}

buint
bdk_keymap_lookup_key (BdkKeymap          *keymap,
                       const BdkKeymapKey *key)
{
  g_return_val_if_fail (keymap == NULL || BDK_IS_KEYMAP (keymap), 0);
  g_return_val_if_fail (key != NULL, 0);
  g_return_val_if_fail (key->group < 4, 0);

  if (!keymap)
    keymap = bdk_keymap_get_for_display (bdk_display_get_default ());

  /* FIXME: Implement */

  return 0;
}

#define GET_KEYVAL(keycode, group, level) (keyval_array[(keycode * KEYVALS_PER_KEYCODE + group * 2 + level)])

static buint
translate_keysym (buint           hardware_keycode,
		  bint            group,
		  BdkModifierType state,
		  bint           *effective_group,
		  bint           *effective_level)
{
  bint level;
  buint tmp_keyval;

  level = (state & BDK_SHIFT_MASK) ? 1 : 0;

  if (!(GET_KEYVAL (hardware_keycode, group, 0) || GET_KEYVAL (hardware_keycode, group, 1)) &&
      (GET_KEYVAL (hardware_keycode, 0, 0) || GET_KEYVAL (hardware_keycode, 0, 1)))
    group = 0;

  if (!GET_KEYVAL (hardware_keycode, group, level) &&
      GET_KEYVAL (hardware_keycode, group, 0))
    level = 0;

  tmp_keyval = GET_KEYVAL (hardware_keycode, group, level);

  if (state & BDK_LOCK_MASK)
    {
      buint upper = bdk_keyval_to_upper (tmp_keyval);
      if (upper != tmp_keyval)
        tmp_keyval = upper;
    }

  if (effective_group)
    *effective_group = group;
  if (effective_level)
    *effective_level = level;

  return tmp_keyval;
}

bboolean
bdk_keymap_translate_keyboard_state (BdkKeymap       *keymap,
                                     buint            hardware_keycode,
                                     BdkModifierType  state,
                                     bint             group,
                                     buint           *keyval,
                                     bint            *effective_group,
                                     bint            *level,
                                     BdkModifierType *consumed_modifiers)
{
  buint tmp_keyval;
  BdkModifierType bit;
  buint tmp_modifiers = 0;

  g_return_val_if_fail (keymap == NULL || BDK_IS_KEYMAP (keymap), FALSE);
  g_return_val_if_fail (group >= 0 && group <= 1, FALSE);

  if (!keymap)
    keymap = bdk_keymap_get_for_display (bdk_display_get_default ());

  if (keyval)
    *keyval = 0;
  if (effective_group)
    *effective_group = 0;
  if (level)
    *level = 0;
  if (consumed_modifiers)
    *consumed_modifiers = 0;

  if (hardware_keycode < 0 || hardware_keycode >= NUM_KEYCODES)
    return FALSE;

  /* Check if modifiers modify the keyval */
  for (bit = BDK_SHIFT_MASK; bit < BDK_BUTTON1_MASK; bit <<= 1)
    {
      if (translate_keysym (hardware_keycode,
                            (bit == BDK_MOD1_MASK) ? 0 : group,
                            state & ~bit,
                            NULL, NULL) !=
	  translate_keysym (hardware_keycode,
                            (bit == BDK_MOD1_MASK) ? 1 : group,
                            state | bit,
                            NULL, NULL))
	tmp_modifiers |= bit;
    }

  tmp_keyval = translate_keysym (hardware_keycode, group, state, level, effective_group);

  if (consumed_modifiers)
    *consumed_modifiers = tmp_modifiers;

  if (keyval)
    *keyval = tmp_keyval; 

  return TRUE;
}

void
bdk_keymap_add_virtual_modifiers (BdkKeymap       *keymap,
                                  BdkModifierType *state)
{
  if (*state & BDK_MOD2_MASK)
    *state |= BDK_META_MASK;
}

bboolean
bdk_keymap_map_virtual_modifiers (BdkKeymap       *keymap,
                                  BdkModifierType *state)
{
  if (*state & BDK_META_MASK)
    *state |= BDK_MOD2_MASK;

  return TRUE;
}

/* What sort of key event is this? Returns one of
 * BDK_KEY_PRESS, BDK_KEY_RELEASE, BDK_NOTHING (should be ignored)
 */
BdkEventType
_bdk_quartz_keys_event_type (NSEvent *event)
{
  unsigned short keycode;
  unsigned int flags;
  int i;
  
  switch ([event type])
    {
    case NSKeyDown:
      return BDK_KEY_PRESS;
    case NSKeyUp:
      return BDK_KEY_RELEASE;
    case NSFlagsChanged:
      break;
    default:
      g_assert_not_reached ();
    }
  
  /* For flags-changed events, we have to find the special key that caused the
   * event, and see if it's in the modifier mask. */
  keycode = [event keyCode];
  flags = [event modifierFlags];
  
  for (i = 0; i < G_N_ELEMENTS (modifier_keys); i++)
    {
      if (modifier_keys[i].keycode == keycode)
	{
	  if (flags & modifier_keys[i].modmask)
	    return BDK_KEY_PRESS;
	  else
	    return BDK_KEY_RELEASE;
	}
    }
  
  /* Some keypresses (eg: Expose' activations) seem to trigger flags-changed
   * events for no good reason. Ignore them! */
  return BDK_NOTHING;
}

bboolean
_bdk_quartz_keys_is_modifier (buint keycode)
{
  bint i;
  
  for (i = 0; i < G_N_ELEMENTS (modifier_keys); i++)
    {
      if (modifier_keys[i].modmask == 0)
	break;

      if (modifier_keys[i].keycode == keycode)
	return TRUE;
    }

  return FALSE;
}
