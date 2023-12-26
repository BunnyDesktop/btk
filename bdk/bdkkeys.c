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

#include "bdkdisplay.h"
#include "bdkkeys.h"
#include "bdkalias.h"

enum {
  DIRECTION_CHANGED,
  KEYS_CHANGED,
  STATE_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BdkKeymap, bdk_keymap, G_TYPE_OBJECT)

static void
bdk_keymap_class_init (BdkKeymapClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  /**
   * BdkKeymap::direction-changed:
   * @keymap: the object on which the signal is emitted
   * 
   * The ::direction-changed signal gets emitted when the direction of
   * the keymap changes. 
   *
   * Since: 2.0
   */
  signals[DIRECTION_CHANGED] =
    g_signal_new ("direction-changed",
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BdkKeymapClass, direction_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE,
		  0);
  /**
   * BdkKeymap::keys-changed:
   * @keymap: the object on which the signal is emitted
   *
   * The ::keys-changed signal is emitted when the mapping represented by
   * @keymap changes.
   *
   * Since: 2.2
   */
  signals[KEYS_CHANGED] =
    g_signal_new ("keys-changed",
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BdkKeymapClass, keys_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE,
		  0);

  /**
   * BdkKeymap::state-changed:
   * @keymap: the object on which the signal is emitted
   *
   * The ::state-changed signal is emitted when the state of the
   * keyboard changes, e.g when Caps Lock is turned on or off.
   * See bdk_keymap_get_caps_lock_state().
   *
   * Since: 2.16
   */
  signals[STATE_CHANGED] =
    g_signal_new ("state_changed",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BdkKeymapClass, state_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 
                  0);
}

static void
bdk_keymap_init (BdkKeymap *keymap)
{
}

/* Other key-handling stuff
 */

#ifndef HAVE_XCONVERTCASE
#include "bdkkeysyms.h"

/* compatibility function from X11R6.3, since XConvertCase is not
 * supplied by X11R5.
 */
/**
 * bdk_keyval_convert_case:
 * @symbol: a keyval
 * @lower: (out): return location for lowercase version of @symbol
 * @upper: (out): return location for uppercase version of @symbol
 *
 * Obtains the upper- and lower-case versions of the keyval @symbol.
 * Examples of keyvals are #BDK_a, #BDK_Enter, #BDK_F1, etc.
 *
 **/
void
bdk_keyval_convert_case (guint symbol,
			 guint *lower,
			 guint *upper)
{
  guint xlower = symbol;
  guint xupper = symbol;

  /* Check for directly encoded 24-bit UCS characters: */
  if ((symbol & 0xff000000) == 0x01000000)
    {
      if (lower)
	*lower = bdk_unicode_to_keyval (g_unichar_tolower (symbol & 0x00ffffff));
      if (upper)
	*upper = bdk_unicode_to_keyval (g_unichar_toupper (symbol & 0x00ffffff));
      return;
    }

  switch (symbol >> 8)
    {
    case 0: /* Latin 1 */
      if ((symbol >= BDK_A) && (symbol <= BDK_Z))
	xlower += (BDK_a - BDK_A);
      else if ((symbol >= BDK_a) && (symbol <= BDK_z))
	xupper -= (BDK_a - BDK_A);
      else if ((symbol >= BDK_Agrave) && (symbol <= BDK_Odiaeresis))
	xlower += (BDK_agrave - BDK_Agrave);
      else if ((symbol >= BDK_agrave) && (symbol <= BDK_odiaeresis))
	xupper -= (BDK_agrave - BDK_Agrave);
      else if ((symbol >= BDK_Ooblique) && (symbol <= BDK_Thorn))
	xlower += (BDK_oslash - BDK_Ooblique);
      else if ((symbol >= BDK_oslash) && (symbol <= BDK_thorn))
	xupper -= (BDK_oslash - BDK_Ooblique);
      break;
      
    case 1: /* Latin 2 */
      /* Assume the KeySym is a legal value (ignore discontinuities) */
      if (symbol == BDK_Aogonek)
	xlower = BDK_aogonek;
      else if (symbol >= BDK_Lstroke && symbol <= BDK_Sacute)
	xlower += (BDK_lstroke - BDK_Lstroke);
      else if (symbol >= BDK_Scaron && symbol <= BDK_Zacute)
	xlower += (BDK_scaron - BDK_Scaron);
      else if (symbol >= BDK_Zcaron && symbol <= BDK_Zabovedot)
	xlower += (BDK_zcaron - BDK_Zcaron);
      else if (symbol == BDK_aogonek)
	xupper = BDK_Aogonek;
      else if (symbol >= BDK_lstroke && symbol <= BDK_sacute)
	xupper -= (BDK_lstroke - BDK_Lstroke);
      else if (symbol >= BDK_scaron && symbol <= BDK_zacute)
	xupper -= (BDK_scaron - BDK_Scaron);
      else if (symbol >= BDK_zcaron && symbol <= BDK_zabovedot)
	xupper -= (BDK_zcaron - BDK_Zcaron);
      else if (symbol >= BDK_Racute && symbol <= BDK_Tcedilla)
	xlower += (BDK_racute - BDK_Racute);
      else if (symbol >= BDK_racute && symbol <= BDK_tcedilla)
	xupper -= (BDK_racute - BDK_Racute);
      break;
      
    case 2: /* Latin 3 */
      /* Assume the KeySym is a legal value (ignore discontinuities) */
      if (symbol >= BDK_Hstroke && symbol <= BDK_Hcircumflex)
	xlower += (BDK_hstroke - BDK_Hstroke);
      else if (symbol >= BDK_Gbreve && symbol <= BDK_Jcircumflex)
	xlower += (BDK_gbreve - BDK_Gbreve);
      else if (symbol >= BDK_hstroke && symbol <= BDK_hcircumflex)
	xupper -= (BDK_hstroke - BDK_Hstroke);
      else if (symbol >= BDK_gbreve && symbol <= BDK_jcircumflex)
	xupper -= (BDK_gbreve - BDK_Gbreve);
      else if (symbol >= BDK_Cabovedot && symbol <= BDK_Scircumflex)
	xlower += (BDK_cabovedot - BDK_Cabovedot);
      else if (symbol >= BDK_cabovedot && symbol <= BDK_scircumflex)
	xupper -= (BDK_cabovedot - BDK_Cabovedot);
      break;
      
    case 3: /* Latin 4 */
      /* Assume the KeySym is a legal value (ignore discontinuities) */
      if (symbol >= BDK_Rcedilla && symbol <= BDK_Tslash)
	xlower += (BDK_rcedilla - BDK_Rcedilla);
      else if (symbol >= BDK_rcedilla && symbol <= BDK_tslash)
	xupper -= (BDK_rcedilla - BDK_Rcedilla);
      else if (symbol == BDK_ENG)
	xlower = BDK_eng;
      else if (symbol == BDK_eng)
	xupper = BDK_ENG;
      else if (symbol >= BDK_Amacron && symbol <= BDK_Umacron)
	xlower += (BDK_amacron - BDK_Amacron);
      else if (symbol >= BDK_amacron && symbol <= BDK_umacron)
	xupper -= (BDK_amacron - BDK_Amacron);
      break;
      
    case 6: /* Cyrillic */
      /* Assume the KeySym is a legal value (ignore discontinuities) */
      if (symbol >= BDK_Serbian_DJE && symbol <= BDK_Serbian_DZE)
	xlower -= (BDK_Serbian_DJE - BDK_Serbian_dje);
      else if (symbol >= BDK_Serbian_dje && symbol <= BDK_Serbian_dze)
	xupper += (BDK_Serbian_DJE - BDK_Serbian_dje);
      else if (symbol >= BDK_Cyrillic_YU && symbol <= BDK_Cyrillic_HARDSIGN)
	xlower -= (BDK_Cyrillic_YU - BDK_Cyrillic_yu);
      else if (symbol >= BDK_Cyrillic_yu && symbol <= BDK_Cyrillic_hardsign)
	xupper += (BDK_Cyrillic_YU - BDK_Cyrillic_yu);
      break;
      
    case 7: /* Greek */
      /* Assume the KeySym is a legal value (ignore discontinuities) */
      if (symbol >= BDK_Greek_ALPHAaccent && symbol <= BDK_Greek_OMEGAaccent)
	xlower += (BDK_Greek_alphaaccent - BDK_Greek_ALPHAaccent);
      else if (symbol >= BDK_Greek_alphaaccent && symbol <= BDK_Greek_omegaaccent &&
	       symbol != BDK_Greek_iotaaccentdieresis &&
	       symbol != BDK_Greek_upsilonaccentdieresis)
	xupper -= (BDK_Greek_alphaaccent - BDK_Greek_ALPHAaccent);
      else if (symbol >= BDK_Greek_ALPHA && symbol <= BDK_Greek_OMEGA)
	xlower += (BDK_Greek_alpha - BDK_Greek_ALPHA);
      else if (symbol >= BDK_Greek_alpha && symbol <= BDK_Greek_omega &&
	       symbol != BDK_Greek_finalsmallsigma)
	xupper -= (BDK_Greek_alpha - BDK_Greek_ALPHA);
      break;
    }

  if (lower)
    *lower = xlower;
  if (upper)
    *upper = xupper;
}
#endif

guint
bdk_keyval_to_upper (guint keyval)
{
  guint result;
  
  bdk_keyval_convert_case (keyval, NULL, &result);

  return result;
}

guint
bdk_keyval_to_lower (guint keyval)
{
  guint result;
  
  bdk_keyval_convert_case (keyval, &result, NULL);

  return result;
}

gboolean
bdk_keyval_is_upper (guint keyval)
{
  if (keyval)
    {
      guint upper_val = 0;
      
      bdk_keyval_convert_case (keyval, NULL, &upper_val);
      return upper_val == keyval;
    }
  return FALSE;
}

gboolean
bdk_keyval_is_lower (guint keyval)
{
  if (keyval)
    {
      guint lower_val = 0;
      
      bdk_keyval_convert_case (keyval, &lower_val, NULL);
      return lower_val == keyval;
    }
  return FALSE;
}

/** 
 * bdk_keymap_get_default:
 * @returns: the #BdkKeymap attached to the default display.
 *
 * Returns the #BdkKeymap attached to the default display.
 **/
BdkKeymap*
bdk_keymap_get_default (void)
{
  return bdk_keymap_get_for_display (bdk_display_get_default ());
}

#define __BDK_KEYS_C__
#include "bdkaliasdef.c"
