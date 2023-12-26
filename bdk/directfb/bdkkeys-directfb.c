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
 * file for a list of people on the BTK+ Team.
 */

/*
 * BTK+ DirectFB backend
 * Copyright (C) 2001-2002  convergence integrated media GmbH
 * Copyright (C) 2002       convergence GmbH
 * Written by Denis Oliver Kropp <dok@convergence.de> and
 *            Sven Neumann <sven@convergence.de>
 */

#include "config.h"
#include "bdk.h"

#include <stdlib.h>
#include <string.h>


#include "bdkdirectfb.h"
#include "bdkprivate-directfb.h"

#include "bdkkeysyms.h"
#include "bdkalias.h"

static struct bdk_key *bdk_keys_by_name = NULL;

BdkModifierType  _bdk_directfb_modifiers = 0;

static buint     *directfb_keymap        = NULL;
static bint       directfb_min_keycode   = 0;
static bint       directfb_max_keycode   = 0;


/*
 *  This array needs to be sorted by key values. It can be generated
 *  from bdkkeysyms.h using a few lines of Perl. This is a bit more
 *  complex as one would expect since BDK defines multiple names for a
 *  few key values. We throw the most akward of them away (BDK_Ln and
 *  BDK_Rn).  This code seems to do the trick:
 *
 *      while (<>) {
 *          if (/^\#define BDK\_(\w+) 0x([0-9|a-f|A-F|]+)/) {
 *	        push @{ $names{hex $2} }, $1;
 *          }
 *      }
 *      foreach $value (sort { $a <=> $b } keys %names) {
 *          for (@{ $names{$value} } ) {
 * 	        next if (/^[R|L]\d+/);
 *              print "  { BDK_$_, \"$_\" },\n";
 *          }
 *      }
 */
static const struct bdk_key
{
  buint        keyval;
  const bchar *name;
} bdk_keys_by_keyval[] = {
  { BDK_space, "space" },
  { BDK_exclam, "exclam" },
  { BDK_quotedbl, "quotedbl" },
  { BDK_numbersign, "numbersign" },
  { BDK_dollar, "dollar" },
  { BDK_percent, "percent" },
  { BDK_ampersand, "ampersand" },
  { BDK_apostrophe, "apostrophe" },
  { BDK_quoteright, "quoteright" },
  { BDK_parenleft, "parenleft" },
  { BDK_parenright, "parenright" },
  { BDK_asterisk, "asterisk" },
  { BDK_plus, "plus" },
  { BDK_comma, "comma" },
  { BDK_minus, "minus" },
  { BDK_period, "period" },
  { BDK_slash, "slash" },
  { BDK_0, "0" },
  { BDK_1, "1" },
  { BDK_2, "2" },
  { BDK_3, "3" },
  { BDK_4, "4" },
  { BDK_5, "5" },
  { BDK_6, "6" },
  { BDK_7, "7" },
  { BDK_8, "8" },
  { BDK_9, "9" },
  { BDK_colon, "colon" },
  { BDK_semicolon, "semicolon" },
  { BDK_less, "less" },
  { BDK_equal, "equal" },
  { BDK_greater, "greater" },
  { BDK_question, "question" },
  { BDK_at, "at" },
  { BDK_A, "A" },
  { BDK_B, "B" },
  { BDK_C, "C" },
  { BDK_D, "D" },
  { BDK_E, "E" },
  { BDK_F, "F" },
  { BDK_G, "G" },
  { BDK_H, "H" },
  { BDK_I, "I" },
  { BDK_J, "J" },
  { BDK_K, "K" },
  { BDK_L, "L" },
  { BDK_M, "M" },
  { BDK_N, "N" },
  { BDK_O, "O" },
  { BDK_P, "P" },
  { BDK_Q, "Q" },
  { BDK_R, "R" },
  { BDK_S, "S" },
  { BDK_T, "T" },
  { BDK_U, "U" },
  { BDK_V, "V" },
  { BDK_W, "W" },
  { BDK_X, "X" },
  { BDK_Y, "Y" },
  { BDK_Z, "Z" },
  { BDK_bracketleft, "bracketleft" },
  { BDK_backslash, "backslash" },
  { BDK_bracketright, "bracketright" },
  { BDK_asciicircum, "asciicircum" },
  { BDK_underscore, "underscore" },
  { BDK_grave, "grave" },
  { BDK_quoteleft, "quoteleft" },
  { BDK_a, "a" },
  { BDK_b, "b" },
  { BDK_c, "c" },
  { BDK_d, "d" },
  { BDK_e, "e" },
  { BDK_f, "f" },
  { BDK_g, "g" },
  { BDK_h, "h" },
  { BDK_i, "i" },
  { BDK_j, "j" },
  { BDK_k, "k" },
  { BDK_l, "l" },
  { BDK_m, "m" },
  { BDK_n, "n" },
  { BDK_o, "o" },
  { BDK_p, "p" },
  { BDK_q, "q" },
  { BDK_r, "r" },
  { BDK_s, "s" },
  { BDK_t, "t" },
  { BDK_u, "u" },
  { BDK_v, "v" },
  { BDK_w, "w" },
  { BDK_x, "x" },
  { BDK_y, "y" },
  { BDK_z, "z" },
  { BDK_braceleft, "braceleft" },
  { BDK_bar, "bar" },
  { BDK_braceright, "braceright" },
  { BDK_asciitilde, "asciitilde" },
  { BDK_nobreakspace, "nobreakspace" },
  { BDK_exclamdown, "exclamdown" },
  { BDK_cent, "cent" },
  { BDK_sterling, "sterling" },
  { BDK_currency, "currency" },
  { BDK_yen, "yen" },
  { BDK_brokenbar, "brokenbar" },
  { BDK_section, "section" },
  { BDK_diaeresis, "diaeresis" },
  { BDK_copyright, "copyright" },
  { BDK_ordfeminine, "ordfeminine" },
  { BDK_guillemotleft, "guillemotleft" },
  { BDK_notsign, "notsign" },
  { BDK_hyphen, "hyphen" },
  { BDK_registered, "registered" },
  { BDK_macron, "macron" },
  { BDK_degree, "degree" },
  { BDK_plusminus, "plusminus" },
  { BDK_twosuperior, "twosuperior" },
  { BDK_threesuperior, "threesuperior" },
  { BDK_acute, "acute" },
  { BDK_mu, "mu" },
  { BDK_paragraph, "paragraph" },
  { BDK_periodcentered, "periodcentered" },
  { BDK_cedilla, "cedilla" },
  { BDK_onesuperior, "onesuperior" },
  { BDK_masculine, "masculine" },
  { BDK_guillemotright, "guillemotright" },
  { BDK_onequarter, "onequarter" },
  { BDK_onehalf, "onehalf" },
  { BDK_threequarters, "threequarters" },
  { BDK_questiondown, "questiondown" },
  { BDK_Agrave, "Agrave" },
  { BDK_Aacute, "Aacute" },
  { BDK_Acircumflex, "Acircumflex" },
  { BDK_Atilde, "Atilde" },
  { BDK_Adiaeresis, "Adiaeresis" },
  { BDK_Aring, "Aring" },
  { BDK_AE, "AE" },
  { BDK_Ccedilla, "Ccedilla" },
  { BDK_Egrave, "Egrave" },
  { BDK_Eacute, "Eacute" },
  { BDK_Ecircumflex, "Ecircumflex" },
  { BDK_Ediaeresis, "Ediaeresis" },
  { BDK_Igrave, "Igrave" },
  { BDK_Iacute, "Iacute" },
  { BDK_Icircumflex, "Icircumflex" },
  { BDK_Idiaeresis, "Idiaeresis" },
  { BDK_ETH, "ETH" },
  { BDK_Eth, "Eth" },
  { BDK_Ntilde, "Ntilde" },
  { BDK_Ograve, "Ograve" },
  { BDK_Oacute, "Oacute" },
  { BDK_Ocircumflex, "Ocircumflex" },
  { BDK_Otilde, "Otilde" },
  { BDK_Odiaeresis, "Odiaeresis" },
  { BDK_multiply, "multiply" },
  { BDK_Ooblique, "Ooblique" },
  { BDK_Ugrave, "Ugrave" },
  { BDK_Uacute, "Uacute" },
  { BDK_Ucircumflex, "Ucircumflex" },
  { BDK_Udiaeresis, "Udiaeresis" },
  { BDK_Yacute, "Yacute" },
  { BDK_THORN, "THORN" },
  { BDK_Thorn, "Thorn" },
  { BDK_ssharp, "ssharp" },
  { BDK_agrave, "agrave" },
  { BDK_aacute, "aacute" },
  { BDK_acircumflex, "acircumflex" },
  { BDK_atilde, "atilde" },
  { BDK_adiaeresis, "adiaeresis" },
  { BDK_aring, "aring" },
  { BDK_ae, "ae" },
  { BDK_ccedilla, "ccedilla" },
  { BDK_egrave, "egrave" },
  { BDK_eacute, "eacute" },
  { BDK_ecircumflex, "ecircumflex" },
  { BDK_ediaeresis, "ediaeresis" },
  { BDK_igrave, "igrave" },
  { BDK_iacute, "iacute" },
  { BDK_icircumflex, "icircumflex" },
  { BDK_idiaeresis, "idiaeresis" },
  { BDK_eth, "eth" },
  { BDK_ntilde, "ntilde" },
  { BDK_ograve, "ograve" },
  { BDK_oacute, "oacute" },
  { BDK_ocircumflex, "ocircumflex" },
  { BDK_otilde, "otilde" },
  { BDK_odiaeresis, "odiaeresis" },
  { BDK_division, "division" },
  { BDK_oslash, "oslash" },
  { BDK_ugrave, "ugrave" },
  { BDK_uacute, "uacute" },
  { BDK_ucircumflex, "ucircumflex" },
  { BDK_udiaeresis, "udiaeresis" },
  { BDK_yacute, "yacute" },
  { BDK_thorn, "thorn" },
  { BDK_ydiaeresis, "ydiaeresis" },
  { BDK_Aogonek, "Aogonek" },
  { BDK_breve, "breve" },
  { BDK_Lstroke, "Lstroke" },
  { BDK_Lcaron, "Lcaron" },
  { BDK_Sacute, "Sacute" },
  { BDK_Scaron, "Scaron" },
  { BDK_Scedilla, "Scedilla" },
  { BDK_Tcaron, "Tcaron" },
  { BDK_Zacute, "Zacute" },
  { BDK_Zcaron, "Zcaron" },
  { BDK_Zabovedot, "Zabovedot" },
  { BDK_aogonek, "aogonek" },
  { BDK_ogonek, "ogonek" },
  { BDK_lstroke, "lstroke" },
  { BDK_lcaron, "lcaron" },
  { BDK_sacute, "sacute" },
  { BDK_caron, "caron" },
  { BDK_scaron, "scaron" },
  { BDK_scedilla, "scedilla" },
  { BDK_tcaron, "tcaron" },
  { BDK_zacute, "zacute" },
  { BDK_doubleacute, "doubleacute" },
  { BDK_zcaron, "zcaron" },
  { BDK_zabovedot, "zabovedot" },
  { BDK_Racute, "Racute" },
  { BDK_Abreve, "Abreve" },
  { BDK_Lacute, "Lacute" },
  { BDK_Cacute, "Cacute" },
  { BDK_Ccaron, "Ccaron" },
  { BDK_Eogonek, "Eogonek" },
  { BDK_Ecaron, "Ecaron" },
  { BDK_Dcaron, "Dcaron" },
  { BDK_Dstroke, "Dstroke" },
  { BDK_Nacute, "Nacute" },
  { BDK_Ncaron, "Ncaron" },
  { BDK_Odoubleacute, "Odoubleacute" },
  { BDK_Rcaron, "Rcaron" },
  { BDK_Uring, "Uring" },
  { BDK_Udoubleacute, "Udoubleacute" },
  { BDK_Tcedilla, "Tcedilla" },
  { BDK_racute, "racute" },
  { BDK_abreve, "abreve" },
  { BDK_lacute, "lacute" },
  { BDK_cacute, "cacute" },
  { BDK_ccaron, "ccaron" },
  { BDK_eogonek, "eogonek" },
  { BDK_ecaron, "ecaron" },
  { BDK_dcaron, "dcaron" },
  { BDK_dstroke, "dstroke" },
  { BDK_nacute, "nacute" },
  { BDK_ncaron, "ncaron" },
  { BDK_odoubleacute, "odoubleacute" },
  { BDK_rcaron, "rcaron" },
  { BDK_uring, "uring" },
  { BDK_udoubleacute, "udoubleacute" },
  { BDK_tcedilla, "tcedilla" },
  { BDK_abovedot, "abovedot" },
  { BDK_Hstroke, "Hstroke" },
  { BDK_Hcircumflex, "Hcircumflex" },
  { BDK_Iabovedot, "Iabovedot" },
  { BDK_Gbreve, "Gbreve" },
  { BDK_Jcircumflex, "Jcircumflex" },
  { BDK_hstroke, "hstroke" },
  { BDK_hcircumflex, "hcircumflex" },
  { BDK_idotless, "idotless" },
  { BDK_gbreve, "gbreve" },
  { BDK_jcircumflex, "jcircumflex" },
  { BDK_Cabovedot, "Cabovedot" },
  { BDK_Ccircumflex, "Ccircumflex" },
  { BDK_Gabovedot, "Gabovedot" },
  { BDK_Gcircumflex, "Gcircumflex" },
  { BDK_Ubreve, "Ubreve" },
  { BDK_Scircumflex, "Scircumflex" },
  { BDK_cabovedot, "cabovedot" },
  { BDK_ccircumflex, "ccircumflex" },
  { BDK_gabovedot, "gabovedot" },
  { BDK_gcircumflex, "gcircumflex" },
  { BDK_ubreve, "ubreve" },
  { BDK_scircumflex, "scircumflex" },
  { BDK_kra, "kra" },
  { BDK_kappa, "kappa" },
  { BDK_Rcedilla, "Rcedilla" },
  { BDK_Itilde, "Itilde" },
  { BDK_Lcedilla, "Lcedilla" },
  { BDK_Emacron, "Emacron" },
  { BDK_Gcedilla, "Gcedilla" },
  { BDK_Tslash, "Tslash" },
  { BDK_rcedilla, "rcedilla" },
  { BDK_itilde, "itilde" },
  { BDK_lcedilla, "lcedilla" },
  { BDK_emacron, "emacron" },
  { BDK_gcedilla, "gcedilla" },
  { BDK_tslash, "tslash" },
  { BDK_ENG, "ENG" },
  { BDK_eng, "eng" },
  { BDK_Amacron, "Amacron" },
  { BDK_Iogonek, "Iogonek" },
  { BDK_Eabovedot, "Eabovedot" },
  { BDK_Imacron, "Imacron" },
  { BDK_Ncedilla, "Ncedilla" },
  { BDK_Omacron, "Omacron" },
  { BDK_Kcedilla, "Kcedilla" },
  { BDK_Uogonek, "Uogonek" },
  { BDK_Utilde, "Utilde" },
  { BDK_Umacron, "Umacron" },
  { BDK_amacron, "amacron" },
  { BDK_iogonek, "iogonek" },
  { BDK_eabovedot, "eabovedot" },
  { BDK_imacron, "imacron" },
  { BDK_ncedilla, "ncedilla" },
  { BDK_omacron, "omacron" },
  { BDK_kcedilla, "kcedilla" },
  { BDK_uogonek, "uogonek" },
  { BDK_utilde, "utilde" },
  { BDK_umacron, "umacron" },
  { BDK_overline, "overline" },
  { BDK_kana_fullstop, "kana_fullstop" },
  { BDK_kana_openingbracket, "kana_openingbracket" },
  { BDK_kana_closingbracket, "kana_closingbracket" },
  { BDK_kana_comma, "kana_comma" },
  { BDK_kana_conjunctive, "kana_conjunctive" },
  { BDK_kana_middledot, "kana_middledot" },
  { BDK_kana_WO, "kana_WO" },
  { BDK_kana_a, "kana_a" },
  { BDK_kana_i, "kana_i" },
  { BDK_kana_u, "kana_u" },
  { BDK_kana_e, "kana_e" },
  { BDK_kana_o, "kana_o" },
  { BDK_kana_ya, "kana_ya" },
  { BDK_kana_yu, "kana_yu" },
  { BDK_kana_yo, "kana_yo" },
  { BDK_kana_tsu, "kana_tsu" },
  { BDK_kana_tu, "kana_tu" },
  { BDK_prolongedsound, "prolongedsound" },
  { BDK_kana_A, "kana_A" },
  { BDK_kana_I, "kana_I" },
  { BDK_kana_U, "kana_U" },
  { BDK_kana_E, "kana_E" },
  { BDK_kana_O, "kana_O" },
  { BDK_kana_KA, "kana_KA" },
  { BDK_kana_KI, "kana_KI" },
  { BDK_kana_KU, "kana_KU" },
  { BDK_kana_KE, "kana_KE" },
  { BDK_kana_KO, "kana_KO" },
  { BDK_kana_SA, "kana_SA" },
  { BDK_kana_SHI, "kana_SHI" },
  { BDK_kana_SU, "kana_SU" },
  { BDK_kana_SE, "kana_SE" },
  { BDK_kana_SO, "kana_SO" },
  { BDK_kana_TA, "kana_TA" },
  { BDK_kana_CHI, "kana_CHI" },
  { BDK_kana_TI, "kana_TI" },
  { BDK_kana_TSU, "kana_TSU" },
  { BDK_kana_TU, "kana_TU" },
  { BDK_kana_TE, "kana_TE" },
  { BDK_kana_TO, "kana_TO" },
  { BDK_kana_NA, "kana_NA" },
  { BDK_kana_NI, "kana_NI" },
  { BDK_kana_NU, "kana_NU" },
  { BDK_kana_NE, "kana_NE" },
  { BDK_kana_NO, "kana_NO" },
  { BDK_kana_HA, "kana_HA" },
  { BDK_kana_HI, "kana_HI" },
  { BDK_kana_FU, "kana_FU" },
  { BDK_kana_HU, "kana_HU" },
  { BDK_kana_HE, "kana_HE" },
  { BDK_kana_HO, "kana_HO" },
  { BDK_kana_MA, "kana_MA" },
  { BDK_kana_MI, "kana_MI" },
  { BDK_kana_MU, "kana_MU" },
  { BDK_kana_ME, "kana_ME" },
  { BDK_kana_MO, "kana_MO" },
  { BDK_kana_YA, "kana_YA" },
  { BDK_kana_YU, "kana_YU" },
  { BDK_kana_YO, "kana_YO" },
  { BDK_kana_RA, "kana_RA" },
  { BDK_kana_RI, "kana_RI" },
  { BDK_kana_RU, "kana_RU" },
  { BDK_kana_RE, "kana_RE" },
  { BDK_kana_RO, "kana_RO" },
  { BDK_kana_WA, "kana_WA" },
  { BDK_kana_N, "kana_N" },
  { BDK_voicedsound, "voicedsound" },
  { BDK_semivoicedsound, "semivoicedsound" },
  { BDK_Arabic_comma, "Arabic_comma" },
  { BDK_Arabic_semicolon, "Arabic_semicolon" },
  { BDK_Arabic_question_mark, "Arabic_question_mark" },
  { BDK_Arabic_hamza, "Arabic_hamza" },
  { BDK_Arabic_maddaonalef, "Arabic_maddaonalef" },
  { BDK_Arabic_hamzaonalef, "Arabic_hamzaonalef" },
  { BDK_Arabic_hamzaonwaw, "Arabic_hamzaonwaw" },
  { BDK_Arabic_hamzaunderalef, "Arabic_hamzaunderalef" },
  { BDK_Arabic_hamzaonyeh, "Arabic_hamzaonyeh" },
  { BDK_Arabic_alef, "Arabic_alef" },
  { BDK_Arabic_beh, "Arabic_beh" },
  { BDK_Arabic_tehmarbuta, "Arabic_tehmarbuta" },
  { BDK_Arabic_teh, "Arabic_teh" },
  { BDK_Arabic_theh, "Arabic_theh" },
  { BDK_Arabic_jeem, "Arabic_jeem" },
  { BDK_Arabic_hah, "Arabic_hah" },
  { BDK_Arabic_khah, "Arabic_khah" },
  { BDK_Arabic_dal, "Arabic_dal" },
  { BDK_Arabic_thal, "Arabic_thal" },
  { BDK_Arabic_ra, "Arabic_ra" },
  { BDK_Arabic_zain, "Arabic_zain" },
  { BDK_Arabic_seen, "Arabic_seen" },
  { BDK_Arabic_sheen, "Arabic_sheen" },
  { BDK_Arabic_sad, "Arabic_sad" },
  { BDK_Arabic_dad, "Arabic_dad" },
  { BDK_Arabic_tah, "Arabic_tah" },
  { BDK_Arabic_zah, "Arabic_zah" },
  { BDK_Arabic_ain, "Arabic_ain" },
  { BDK_Arabic_ghain, "Arabic_ghain" },
  { BDK_Arabic_tatweel, "Arabic_tatweel" },
  { BDK_Arabic_feh, "Arabic_feh" },
  { BDK_Arabic_qaf, "Arabic_qaf" },
  { BDK_Arabic_kaf, "Arabic_kaf" },
  { BDK_Arabic_lam, "Arabic_lam" },
  { BDK_Arabic_meem, "Arabic_meem" },
  { BDK_Arabic_noon, "Arabic_noon" },
  { BDK_Arabic_ha, "Arabic_ha" },
  { BDK_Arabic_heh, "Arabic_heh" },
  { BDK_Arabic_waw, "Arabic_waw" },
  { BDK_Arabic_alefmaksura, "Arabic_alefmaksura" },
  { BDK_Arabic_yeh, "Arabic_yeh" },
  { BDK_Arabic_fathatan, "Arabic_fathatan" },
  { BDK_Arabic_dammatan, "Arabic_dammatan" },
  { BDK_Arabic_kasratan, "Arabic_kasratan" },
  { BDK_Arabic_fatha, "Arabic_fatha" },
  { BDK_Arabic_damma, "Arabic_damma" },
  { BDK_Arabic_kasra, "Arabic_kasra" },
  { BDK_Arabic_shadda, "Arabic_shadda" },
  { BDK_Arabic_sukun, "Arabic_sukun" },
  { BDK_Serbian_dje, "Serbian_dje" },
  { BDK_Macedonia_gje, "Macedonia_gje" },
  { BDK_Cyrillic_io, "Cyrillic_io" },
  { BDK_Ukrainian_ie, "Ukrainian_ie" },
  { BDK_Ukranian_je, "Ukranian_je" },
  { BDK_Macedonia_dse, "Macedonia_dse" },
  { BDK_Ukrainian_i, "Ukrainian_i" },
  { BDK_Ukranian_i, "Ukranian_i" },
  { BDK_Ukrainian_yi, "Ukrainian_yi" },
  { BDK_Ukranian_yi, "Ukranian_yi" },
  { BDK_Cyrillic_je, "Cyrillic_je" },
  { BDK_Serbian_je, "Serbian_je" },
  { BDK_Cyrillic_lje, "Cyrillic_lje" },
  { BDK_Serbian_lje, "Serbian_lje" },
  { BDK_Cyrillic_nje, "Cyrillic_nje" },
  { BDK_Serbian_nje, "Serbian_nje" },
  { BDK_Serbian_tshe, "Serbian_tshe" },
  { BDK_Macedonia_kje, "Macedonia_kje" },
  { BDK_Byelorussian_shortu, "Byelorussian_shortu" },
  { BDK_Cyrillic_dzhe, "Cyrillic_dzhe" },
  { BDK_Serbian_dze, "Serbian_dze" },
  { BDK_numerosign, "numerosign" },
  { BDK_Serbian_DJE, "Serbian_DJE" },
  { BDK_Macedonia_GJE, "Macedonia_GJE" },
  { BDK_Cyrillic_IO, "Cyrillic_IO" },
  { BDK_Ukrainian_IE, "Ukrainian_IE" },
  { BDK_Ukranian_JE, "Ukranian_JE" },
  { BDK_Macedonia_DSE, "Macedonia_DSE" },
  { BDK_Ukrainian_I, "Ukrainian_I" },
  { BDK_Ukranian_I, "Ukranian_I" },
  { BDK_Ukrainian_YI, "Ukrainian_YI" },
  { BDK_Ukranian_YI, "Ukranian_YI" },
  { BDK_Cyrillic_JE, "Cyrillic_JE" },
  { BDK_Serbian_JE, "Serbian_JE" },
  { BDK_Cyrillic_LJE, "Cyrillic_LJE" },
  { BDK_Serbian_LJE, "Serbian_LJE" },
  { BDK_Cyrillic_NJE, "Cyrillic_NJE" },
  { BDK_Serbian_NJE, "Serbian_NJE" },
  { BDK_Serbian_TSHE, "Serbian_TSHE" },
  { BDK_Macedonia_KJE, "Macedonia_KJE" },
  { BDK_Byelorussian_SHORTU, "Byelorussian_SHORTU" },
  { BDK_Cyrillic_DZHE, "Cyrillic_DZHE" },
  { BDK_Serbian_DZE, "Serbian_DZE" },
  { BDK_Cyrillic_yu, "Cyrillic_yu" },
  { BDK_Cyrillic_a, "Cyrillic_a" },
  { BDK_Cyrillic_be, "Cyrillic_be" },
  { BDK_Cyrillic_tse, "Cyrillic_tse" },
  { BDK_Cyrillic_de, "Cyrillic_de" },
  { BDK_Cyrillic_ie, "Cyrillic_ie" },
  { BDK_Cyrillic_ef, "Cyrillic_ef" },
  { BDK_Cyrillic_ghe, "Cyrillic_ghe" },
  { BDK_Cyrillic_ha, "Cyrillic_ha" },
  { BDK_Cyrillic_i, "Cyrillic_i" },
  { BDK_Cyrillic_shorti, "Cyrillic_shorti" },
  { BDK_Cyrillic_ka, "Cyrillic_ka" },
  { BDK_Cyrillic_el, "Cyrillic_el" },
  { BDK_Cyrillic_em, "Cyrillic_em" },
  { BDK_Cyrillic_en, "Cyrillic_en" },
  { BDK_Cyrillic_o, "Cyrillic_o" },
  { BDK_Cyrillic_pe, "Cyrillic_pe" },
  { BDK_Cyrillic_ya, "Cyrillic_ya" },
  { BDK_Cyrillic_er, "Cyrillic_er" },
  { BDK_Cyrillic_es, "Cyrillic_es" },
  { BDK_Cyrillic_te, "Cyrillic_te" },
  { BDK_Cyrillic_u, "Cyrillic_u" },
  { BDK_Cyrillic_zhe, "Cyrillic_zhe" },
  { BDK_Cyrillic_ve, "Cyrillic_ve" },
  { BDK_Cyrillic_softsign, "Cyrillic_softsign" },
  { BDK_Cyrillic_yeru, "Cyrillic_yeru" },
  { BDK_Cyrillic_ze, "Cyrillic_ze" },
  { BDK_Cyrillic_sha, "Cyrillic_sha" },
  { BDK_Cyrillic_e, "Cyrillic_e" },
  { BDK_Cyrillic_shcha, "Cyrillic_shcha" },
  { BDK_Cyrillic_che, "Cyrillic_che" },
  { BDK_Cyrillic_hardsign, "Cyrillic_hardsign" },
  { BDK_Cyrillic_YU, "Cyrillic_YU" },
  { BDK_Cyrillic_A, "Cyrillic_A" },
  { BDK_Cyrillic_BE, "Cyrillic_BE" },
  { BDK_Cyrillic_TSE, "Cyrillic_TSE" },
  { BDK_Cyrillic_DE, "Cyrillic_DE" },
  { BDK_Cyrillic_IE, "Cyrillic_IE" },
  { BDK_Cyrillic_EF, "Cyrillic_EF" },
  { BDK_Cyrillic_GHE, "Cyrillic_GHE" },
  { BDK_Cyrillic_HA, "Cyrillic_HA" },
  { BDK_Cyrillic_I, "Cyrillic_I" },
  { BDK_Cyrillic_SHORTI, "Cyrillic_SHORTI" },
  { BDK_Cyrillic_KA, "Cyrillic_KA" },
  { BDK_Cyrillic_EL, "Cyrillic_EL" },
  { BDK_Cyrillic_EM, "Cyrillic_EM" },
  { BDK_Cyrillic_EN, "Cyrillic_EN" },
  { BDK_Cyrillic_O, "Cyrillic_O" },
  { BDK_Cyrillic_PE, "Cyrillic_PE" },
  { BDK_Cyrillic_YA, "Cyrillic_YA" },
  { BDK_Cyrillic_ER, "Cyrillic_ER" },
  { BDK_Cyrillic_ES, "Cyrillic_ES" },
  { BDK_Cyrillic_TE, "Cyrillic_TE" },
  { BDK_Cyrillic_U, "Cyrillic_U" },
  { BDK_Cyrillic_ZHE, "Cyrillic_ZHE" },
  { BDK_Cyrillic_VE, "Cyrillic_VE" },
  { BDK_Cyrillic_SOFTSIGN, "Cyrillic_SOFTSIGN" },
  { BDK_Cyrillic_YERU, "Cyrillic_YERU" },
  { BDK_Cyrillic_ZE, "Cyrillic_ZE" },
  { BDK_Cyrillic_SHA, "Cyrillic_SHA" },
  { BDK_Cyrillic_E, "Cyrillic_E" },
  { BDK_Cyrillic_SHCHA, "Cyrillic_SHCHA" },
  { BDK_Cyrillic_CHE, "Cyrillic_CHE" },
  { BDK_Cyrillic_HARDSIGN, "Cyrillic_HARDSIGN" },
  { BDK_Greek_ALPHAaccent, "Greek_ALPHAaccent" },
  { BDK_Greek_EPSILONaccent, "Greek_EPSILONaccent" },
  { BDK_Greek_ETAaccent, "Greek_ETAaccent" },
  { BDK_Greek_IOTAaccent, "Greek_IOTAaccent" },
  { BDK_Greek_IOTAdiaeresis, "Greek_IOTAdiaeresis" },
  { BDK_Greek_OMICRONaccent, "Greek_OMICRONaccent" },
  { BDK_Greek_UPSILONaccent, "Greek_UPSILONaccent" },
  { BDK_Greek_UPSILONdieresis, "Greek_UPSILONdieresis" },
  { BDK_Greek_OMEGAaccent, "Greek_OMEGAaccent" },
  { BDK_Greek_accentdieresis, "Greek_accentdieresis" },
  { BDK_Greek_horizbar, "Greek_horizbar" },
  { BDK_Greek_alphaaccent, "Greek_alphaaccent" },
  { BDK_Greek_epsilonaccent, "Greek_epsilonaccent" },
  { BDK_Greek_etaaccent, "Greek_etaaccent" },
  { BDK_Greek_iotaaccent, "Greek_iotaaccent" },
  { BDK_Greek_iotadieresis, "Greek_iotadieresis" },
  { BDK_Greek_iotaaccentdieresis, "Greek_iotaaccentdieresis" },
  { BDK_Greek_omicronaccent, "Greek_omicronaccent" },
  { BDK_Greek_upsilonaccent, "Greek_upsilonaccent" },
  { BDK_Greek_upsilondieresis, "Greek_upsilondieresis" },
  { BDK_Greek_upsilonaccentdieresis, "Greek_upsilonaccentdieresis" },
  { BDK_Greek_omegaaccent, "Greek_omegaaccent" },
  { BDK_Greek_ALPHA, "Greek_ALPHA" },
  { BDK_Greek_BETA, "Greek_BETA" },
  { BDK_Greek_GAMMA, "Greek_GAMMA" },
  { BDK_Greek_DELTA, "Greek_DELTA" },
  { BDK_Greek_EPSILON, "Greek_EPSILON" },
  { BDK_Greek_ZETA, "Greek_ZETA" },
  { BDK_Greek_ETA, "Greek_ETA" },
  { BDK_Greek_THETA, "Greek_THETA" },
  { BDK_Greek_IOTA, "Greek_IOTA" },
  { BDK_Greek_KAPPA, "Greek_KAPPA" },
  { BDK_Greek_LAMDA, "Greek_LAMDA" },
  { BDK_Greek_LAMBDA, "Greek_LAMBDA" },
  { BDK_Greek_MU, "Greek_MU" },
  { BDK_Greek_NU, "Greek_NU" },
  { BDK_Greek_XI, "Greek_XI" },
  { BDK_Greek_OMICRON, "Greek_OMICRON" },
  { BDK_Greek_PI, "Greek_PI" },
  { BDK_Greek_RHO, "Greek_RHO" },
  { BDK_Greek_SIGMA, "Greek_SIGMA" },
  { BDK_Greek_TAU, "Greek_TAU" },
  { BDK_Greek_UPSILON, "Greek_UPSILON" },
  { BDK_Greek_PHI, "Greek_PHI" },
  { BDK_Greek_CHI, "Greek_CHI" },
  { BDK_Greek_PSI, "Greek_PSI" },
  { BDK_Greek_OMEGA, "Greek_OMEGA" },
  { BDK_Greek_alpha, "Greek_alpha" },
  { BDK_Greek_beta, "Greek_beta" },
  { BDK_Greek_gamma, "Greek_gamma" },
  { BDK_Greek_delta, "Greek_delta" },
  { BDK_Greek_epsilon, "Greek_epsilon" },
  { BDK_Greek_zeta, "Greek_zeta" },
  { BDK_Greek_eta, "Greek_eta" },
  { BDK_Greek_theta, "Greek_theta" },
  { BDK_Greek_iota, "Greek_iota" },
  { BDK_Greek_kappa, "Greek_kappa" },
  { BDK_Greek_lamda, "Greek_lamda" },
  { BDK_Greek_lambda, "Greek_lambda" },
  { BDK_Greek_mu, "Greek_mu" },
  { BDK_Greek_nu, "Greek_nu" },
  { BDK_Greek_xi, "Greek_xi" },
  { BDK_Greek_omicron, "Greek_omicron" },
  { BDK_Greek_pi, "Greek_pi" },
  { BDK_Greek_rho, "Greek_rho" },
  { BDK_Greek_sigma, "Greek_sigma" },
  { BDK_Greek_finalsmallsigma, "Greek_finalsmallsigma" },
  { BDK_Greek_tau, "Greek_tau" },
  { BDK_Greek_upsilon, "Greek_upsilon" },
  { BDK_Greek_phi, "Greek_phi" },
  { BDK_Greek_chi, "Greek_chi" },
  { BDK_Greek_psi, "Greek_psi" },
  { BDK_Greek_omega, "Greek_omega" },
  { BDK_leftradical, "leftradical" },
  { BDK_topleftradical, "topleftradical" },
  { BDK_horizconnector, "horizconnector" },
  { BDK_topintegral, "topintegral" },
  { BDK_botintegral, "botintegral" },
  { BDK_vertconnector, "vertconnector" },
  { BDK_topleftsqbracket, "topleftsqbracket" },
  { BDK_botleftsqbracket, "botleftsqbracket" },
  { BDK_toprightsqbracket, "toprightsqbracket" },
  { BDK_botrightsqbracket, "botrightsqbracket" },
  { BDK_topleftparens, "topleftparens" },
  { BDK_botleftparens, "botleftparens" },
  { BDK_toprightparens, "toprightparens" },
  { BDK_botrightparens, "botrightparens" },
  { BDK_leftmiddlecurlybrace, "leftmiddlecurlybrace" },
  { BDK_rightmiddlecurlybrace, "rightmiddlecurlybrace" },
  { BDK_topleftsummation, "topleftsummation" },
  { BDK_botleftsummation, "botleftsummation" },
  { BDK_topvertsummationconnector, "topvertsummationconnector" },
  { BDK_botvertsummationconnector, "botvertsummationconnector" },
  { BDK_toprightsummation, "toprightsummation" },
  { BDK_botrightsummation, "botrightsummation" },
  { BDK_rightmiddlesummation, "rightmiddlesummation" },
  { BDK_lessthanequal, "lessthanequal" },
  { BDK_notequal, "notequal" },
  { BDK_greaterthanequal, "greaterthanequal" },
  { BDK_integral, "integral" },
  { BDK_therefore, "therefore" },
  { BDK_variation, "variation" },
  { BDK_infinity, "infinity" },
  { BDK_nabla, "nabla" },
  { BDK_approximate, "approximate" },
  { BDK_similarequal, "similarequal" },
  { BDK_ifonlyif, "ifonlyif" },
  { BDK_implies, "implies" },
  { BDK_identical, "identical" },
  { BDK_radical, "radical" },
  { BDK_includedin, "includedin" },
  { BDK_includes, "includes" },
  { BDK_intersection, "intersection" },
  { BDK_union, "union" },
  { BDK_logicaland, "logicaland" },
  { BDK_logicalor, "logicalor" },
  { BDK_partialderivative, "partialderivative" },
  { BDK_function, "function" },
  { BDK_leftarrow, "leftarrow" },
  { BDK_uparrow, "uparrow" },
  { BDK_rightarrow, "rightarrow" },
  { BDK_downarrow, "downarrow" },
  { BDK_blank, "blank" },
  { BDK_soliddiamond, "soliddiamond" },
  { BDK_checkerboard, "checkerboard" },
  { BDK_ht, "ht" },
  { BDK_ff, "ff" },
  { BDK_cr, "cr" },
  { BDK_lf, "lf" },
  { BDK_nl, "nl" },
  { BDK_vt, "vt" },
  { BDK_lowrightcorner, "lowrightcorner" },
  { BDK_uprightcorner, "uprightcorner" },
  { BDK_upleftcorner, "upleftcorner" },
  { BDK_lowleftcorner, "lowleftcorner" },
  { BDK_crossinglines, "crossinglines" },
  { BDK_horizlinescan1, "horizlinescan1" },
  { BDK_horizlinescan3, "horizlinescan3" },
  { BDK_horizlinescan5, "horizlinescan5" },
  { BDK_horizlinescan7, "horizlinescan7" },
  { BDK_horizlinescan9, "horizlinescan9" },
  { BDK_leftt, "leftt" },
  { BDK_rightt, "rightt" },
  { BDK_bott, "bott" },
  { BDK_topt, "topt" },
  { BDK_vertbar, "vertbar" },
  { BDK_emspace, "emspace" },
  { BDK_enspace, "enspace" },
  { BDK_em3space, "em3space" },
  { BDK_em4space, "em4space" },
  { BDK_digitspace, "digitspace" },
  { BDK_punctspace, "punctspace" },
  { BDK_thinspace, "thinspace" },
  { BDK_hairspace, "hairspace" },
  { BDK_emdash, "emdash" },
  { BDK_endash, "endash" },
  { BDK_signifblank, "signifblank" },
  { BDK_ellipsis, "ellipsis" },
  { BDK_doubbaselinedot, "doubbaselinedot" },
  { BDK_onethird, "onethird" },
  { BDK_twothirds, "twothirds" },
  { BDK_onefifth, "onefifth" },
  { BDK_twofifths, "twofifths" },
  { BDK_threefifths, "threefifths" },
  { BDK_fourfifths, "fourfifths" },
  { BDK_onesixth, "onesixth" },
  { BDK_fivesixths, "fivesixths" },
  { BDK_careof, "careof" },
  { BDK_figdash, "figdash" },
  { BDK_leftanglebracket, "leftanglebracket" },
  { BDK_decimalpoint, "decimalpoint" },
  { BDK_rightanglebracket, "rightanglebracket" },
  { BDK_marker, "marker" },
  { BDK_oneeighth, "oneeighth" },
  { BDK_threeeighths, "threeeighths" },
  { BDK_fiveeighths, "fiveeighths" },
  { BDK_seveneighths, "seveneighths" },
  { BDK_trademark, "trademark" },
  { BDK_signaturemark, "signaturemark" },
  { BDK_trademarkincircle, "trademarkincircle" },
  { BDK_leftopentriangle, "leftopentriangle" },
  { BDK_rightopentriangle, "rightopentriangle" },
  { BDK_emopencircle, "emopencircle" },
  { BDK_emopenrectangle, "emopenrectangle" },
  { BDK_leftsinglequotemark, "leftsinglequotemark" },
  { BDK_rightsinglequotemark, "rightsinglequotemark" },
  { BDK_leftdoublequotemark, "leftdoublequotemark" },
  { BDK_rightdoublequotemark, "rightdoublequotemark" },
  { BDK_prescription, "prescription" },
  { BDK_minutes, "minutes" },
  { BDK_seconds, "seconds" },
  { BDK_latincross, "latincross" },
  { BDK_hexagram, "hexagram" },
  { BDK_filledrectbullet, "filledrectbullet" },
  { BDK_filledlefttribullet, "filledlefttribullet" },
  { BDK_filledrighttribullet, "filledrighttribullet" },
  { BDK_emfilledcircle, "emfilledcircle" },
  { BDK_emfilledrect, "emfilledrect" },
  { BDK_enopencircbullet, "enopencircbullet" },
  { BDK_enopensquarebullet, "enopensquarebullet" },
  { BDK_openrectbullet, "openrectbullet" },
  { BDK_opentribulletup, "opentribulletup" },
  { BDK_opentribulletdown, "opentribulletdown" },
  { BDK_openstar, "openstar" },
  { BDK_enfilledcircbullet, "enfilledcircbullet" },
  { BDK_enfilledsqbullet, "enfilledsqbullet" },
  { BDK_filledtribulletup, "filledtribulletup" },
  { BDK_filledtribulletdown, "filledtribulletdown" },
  { BDK_leftpointer, "leftpointer" },
  { BDK_rightpointer, "rightpointer" },
  { BDK_club, "club" },
  { BDK_diamond, "diamond" },
  { BDK_heart, "heart" },
  { BDK_maltesecross, "maltesecross" },
  { BDK_dagger, "dagger" },
  { BDK_doubledagger, "doubledagger" },
  { BDK_checkmark, "checkmark" },
  { BDK_ballotcross, "ballotcross" },
  { BDK_musicalsharp, "musicalsharp" },
  { BDK_musicalflat, "musicalflat" },
  { BDK_malesymbol, "malesymbol" },
  { BDK_femalesymbol, "femalesymbol" },
  { BDK_telephone, "telephone" },
  { BDK_telephonerecorder, "telephonerecorder" },
  { BDK_phonographcopyright, "phonographcopyright" },
  { BDK_caret, "caret" },
  { BDK_singlelowquotemark, "singlelowquotemark" },
  { BDK_doublelowquotemark, "doublelowquotemark" },
  { BDK_cursor, "cursor" },
  { BDK_leftcaret, "leftcaret" },
  { BDK_rightcaret, "rightcaret" },
  { BDK_downcaret, "downcaret" },
  { BDK_upcaret, "upcaret" },
  { BDK_overbar, "overbar" },
  { BDK_downtack, "downtack" },
  { BDK_upshoe, "upshoe" },
  { BDK_downstile, "downstile" },
  { BDK_underbar, "underbar" },
  { BDK_jot, "jot" },
  { BDK_quad, "quad" },
  { BDK_uptack, "uptack" },
  { BDK_circle, "circle" },
  { BDK_upstile, "upstile" },
  { BDK_downshoe, "downshoe" },
  { BDK_rightshoe, "rightshoe" },
  { BDK_leftshoe, "leftshoe" },
  { BDK_lefttack, "lefttack" },
  { BDK_righttack, "righttack" },
  { BDK_hebrew_doublelowline, "hebrew_doublelowline" },
  { BDK_hebrew_aleph, "hebrew_aleph" },
  { BDK_hebrew_bet, "hebrew_bet" },
  { BDK_hebrew_beth, "hebrew_beth" },
  { BDK_hebrew_gimel, "hebrew_gimel" },
  { BDK_hebrew_gimmel, "hebrew_gimmel" },
  { BDK_hebrew_dalet, "hebrew_dalet" },
  { BDK_hebrew_daleth, "hebrew_daleth" },
  { BDK_hebrew_he, "hebrew_he" },
  { BDK_hebrew_waw, "hebrew_waw" },
  { BDK_hebrew_zain, "hebrew_zain" },
  { BDK_hebrew_zayin, "hebrew_zayin" },
  { BDK_hebrew_chet, "hebrew_chet" },
  { BDK_hebrew_het, "hebrew_het" },
  { BDK_hebrew_tet, "hebrew_tet" },
  { BDK_hebrew_teth, "hebrew_teth" },
  { BDK_hebrew_yod, "hebrew_yod" },
  { BDK_hebrew_finalkaph, "hebrew_finalkaph" },
  { BDK_hebrew_kaph, "hebrew_kaph" },
  { BDK_hebrew_lamed, "hebrew_lamed" },
  { BDK_hebrew_finalmem, "hebrew_finalmem" },
  { BDK_hebrew_mem, "hebrew_mem" },
  { BDK_hebrew_finalnun, "hebrew_finalnun" },
  { BDK_hebrew_nun, "hebrew_nun" },
  { BDK_hebrew_samech, "hebrew_samech" },
  { BDK_hebrew_samekh, "hebrew_samekh" },
  { BDK_hebrew_ayin, "hebrew_ayin" },
  { BDK_hebrew_finalpe, "hebrew_finalpe" },
  { BDK_hebrew_pe, "hebrew_pe" },
  { BDK_hebrew_finalzade, "hebrew_finalzade" },
  { BDK_hebrew_finalzadi, "hebrew_finalzadi" },
  { BDK_hebrew_zade, "hebrew_zade" },
  { BDK_hebrew_zadi, "hebrew_zadi" },
  { BDK_hebrew_qoph, "hebrew_qoph" },
  { BDK_hebrew_kuf, "hebrew_kuf" },
  { BDK_hebrew_resh, "hebrew_resh" },
  { BDK_hebrew_shin, "hebrew_shin" },
  { BDK_hebrew_taw, "hebrew_taw" },
  { BDK_hebrew_taf, "hebrew_taf" },
  { BDK_Thai_kokai, "Thai_kokai" },
  { BDK_Thai_khokhai, "Thai_khokhai" },
  { BDK_Thai_khokhuat, "Thai_khokhuat" },
  { BDK_Thai_khokhwai, "Thai_khokhwai" },
  { BDK_Thai_khokhon, "Thai_khokhon" },
  { BDK_Thai_khorakhang, "Thai_khorakhang" },
  { BDK_Thai_ngongu, "Thai_ngongu" },
  { BDK_Thai_chochan, "Thai_chochan" },
  { BDK_Thai_choching, "Thai_choching" },
  { BDK_Thai_chochang, "Thai_chochang" },
  { BDK_Thai_soso, "Thai_soso" },
  { BDK_Thai_chochoe, "Thai_chochoe" },
  { BDK_Thai_yoying, "Thai_yoying" },
  { BDK_Thai_dochada, "Thai_dochada" },
  { BDK_Thai_topatak, "Thai_topatak" },
  { BDK_Thai_thothan, "Thai_thothan" },
  { BDK_Thai_thonangmontho, "Thai_thonangmontho" },
  { BDK_Thai_thophuthao, "Thai_thophuthao" },
  { BDK_Thai_nonen, "Thai_nonen" },
  { BDK_Thai_dodek, "Thai_dodek" },
  { BDK_Thai_totao, "Thai_totao" },
  { BDK_Thai_thothung, "Thai_thothung" },
  { BDK_Thai_thothahan, "Thai_thothahan" },
  { BDK_Thai_thothong, "Thai_thothong" },
  { BDK_Thai_nonu, "Thai_nonu" },
  { BDK_Thai_bobaimai, "Thai_bobaimai" },
  { BDK_Thai_popla, "Thai_popla" },
  { BDK_Thai_phophung, "Thai_phophung" },
  { BDK_Thai_fofa, "Thai_fofa" },
  { BDK_Thai_phophan, "Thai_phophan" },
  { BDK_Thai_fofan, "Thai_fofan" },
  { BDK_Thai_phosamphao, "Thai_phosamphao" },
  { BDK_Thai_moma, "Thai_moma" },
  { BDK_Thai_yoyak, "Thai_yoyak" },
  { BDK_Thai_rorua, "Thai_rorua" },
  { BDK_Thai_ru, "Thai_ru" },
  { BDK_Thai_loling, "Thai_loling" },
  { BDK_Thai_lu, "Thai_lu" },
  { BDK_Thai_wowaen, "Thai_wowaen" },
  { BDK_Thai_sosala, "Thai_sosala" },
  { BDK_Thai_sorusi, "Thai_sorusi" },
  { BDK_Thai_sosua, "Thai_sosua" },
  { BDK_Thai_hohip, "Thai_hohip" },
  { BDK_Thai_lochula, "Thai_lochula" },
  { BDK_Thai_oang, "Thai_oang" },
  { BDK_Thai_honokhuk, "Thai_honokhuk" },
  { BDK_Thai_paiyannoi, "Thai_paiyannoi" },
  { BDK_Thai_saraa, "Thai_saraa" },
  { BDK_Thai_maihanakat, "Thai_maihanakat" },
  { BDK_Thai_saraaa, "Thai_saraaa" },
  { BDK_Thai_saraam, "Thai_saraam" },
  { BDK_Thai_sarai, "Thai_sarai" },
  { BDK_Thai_saraii, "Thai_saraii" },
  { BDK_Thai_saraue, "Thai_saraue" },
  { BDK_Thai_sarauee, "Thai_sarauee" },
  { BDK_Thai_sarau, "Thai_sarau" },
  { BDK_Thai_sarauu, "Thai_sarauu" },
  { BDK_Thai_phinthu, "Thai_phinthu" },
  { BDK_Thai_maihanakat_maitho, "Thai_maihanakat_maitho" },
  { BDK_Thai_baht, "Thai_baht" },
  { BDK_Thai_sarae, "Thai_sarae" },
  { BDK_Thai_saraae, "Thai_saraae" },
  { BDK_Thai_sarao, "Thai_sarao" },
  { BDK_Thai_saraaimaimuan, "Thai_saraaimaimuan" },
  { BDK_Thai_saraaimaimalai, "Thai_saraaimaimalai" },
  { BDK_Thai_lakkhangyao, "Thai_lakkhangyao" },
  { BDK_Thai_maiyamok, "Thai_maiyamok" },
  { BDK_Thai_maitaikhu, "Thai_maitaikhu" },
  { BDK_Thai_maiek, "Thai_maiek" },
  { BDK_Thai_maitho, "Thai_maitho" },
  { BDK_Thai_maitri, "Thai_maitri" },
  { BDK_Thai_maichattawa, "Thai_maichattawa" },
  { BDK_Thai_thanthakhat, "Thai_thanthakhat" },
  { BDK_Thai_nikhahit, "Thai_nikhahit" },
  { BDK_Thai_leksun, "Thai_leksun" },
  { BDK_Thai_leknung, "Thai_leknung" },
  { BDK_Thai_leksong, "Thai_leksong" },
  { BDK_Thai_leksam, "Thai_leksam" },
  { BDK_Thai_leksi, "Thai_leksi" },
  { BDK_Thai_lekha, "Thai_lekha" },
  { BDK_Thai_lekhok, "Thai_lekhok" },
  { BDK_Thai_lekchet, "Thai_lekchet" },
  { BDK_Thai_lekpaet, "Thai_lekpaet" },
  { BDK_Thai_lekkao, "Thai_lekkao" },
  { BDK_Hangul_Kiyeog, "Hangul_Kiyeog" },
  { BDK_Hangul_SsangKiyeog, "Hangul_SsangKiyeog" },
  { BDK_Hangul_KiyeogSios, "Hangul_KiyeogSios" },
  { BDK_Hangul_Nieun, "Hangul_Nieun" },
  { BDK_Hangul_NieunJieuj, "Hangul_NieunJieuj" },
  { BDK_Hangul_NieunHieuh, "Hangul_NieunHieuh" },
  { BDK_Hangul_Dikeud, "Hangul_Dikeud" },
  { BDK_Hangul_SsangDikeud, "Hangul_SsangDikeud" },
  { BDK_Hangul_Rieul, "Hangul_Rieul" },
  { BDK_Hangul_RieulKiyeog, "Hangul_RieulKiyeog" },
  { BDK_Hangul_RieulMieum, "Hangul_RieulMieum" },
  { BDK_Hangul_RieulPieub, "Hangul_RieulPieub" },
  { BDK_Hangul_RieulSios, "Hangul_RieulSios" },
  { BDK_Hangul_RieulTieut, "Hangul_RieulTieut" },
  { BDK_Hangul_RieulPhieuf, "Hangul_RieulPhieuf" },
  { BDK_Hangul_RieulHieuh, "Hangul_RieulHieuh" },
  { BDK_Hangul_Mieum, "Hangul_Mieum" },
  { BDK_Hangul_Pieub, "Hangul_Pieub" },
  { BDK_Hangul_SsangPieub, "Hangul_SsangPieub" },
  { BDK_Hangul_PieubSios, "Hangul_PieubSios" },
  { BDK_Hangul_Sios, "Hangul_Sios" },
  { BDK_Hangul_SsangSios, "Hangul_SsangSios" },
  { BDK_Hangul_Ieung, "Hangul_Ieung" },
  { BDK_Hangul_Jieuj, "Hangul_Jieuj" },
  { BDK_Hangul_SsangJieuj, "Hangul_SsangJieuj" },
  { BDK_Hangul_Cieuc, "Hangul_Cieuc" },
  { BDK_Hangul_Khieuq, "Hangul_Khieuq" },
  { BDK_Hangul_Tieut, "Hangul_Tieut" },
  { BDK_Hangul_Phieuf, "Hangul_Phieuf" },
  { BDK_Hangul_Hieuh, "Hangul_Hieuh" },
  { BDK_Hangul_A, "Hangul_A" },
  { BDK_Hangul_AE, "Hangul_AE" },
  { BDK_Hangul_YA, "Hangul_YA" },
  { BDK_Hangul_YAE, "Hangul_YAE" },
  { BDK_Hangul_EO, "Hangul_EO" },
  { BDK_Hangul_E, "Hangul_E" },
  { BDK_Hangul_YEO, "Hangul_YEO" },
  { BDK_Hangul_YE, "Hangul_YE" },
  { BDK_Hangul_O, "Hangul_O" },
  { BDK_Hangul_WA, "Hangul_WA" },
  { BDK_Hangul_WAE, "Hangul_WAE" },
  { BDK_Hangul_OE, "Hangul_OE" },
  { BDK_Hangul_YO, "Hangul_YO" },
  { BDK_Hangul_U, "Hangul_U" },
  { BDK_Hangul_WEO, "Hangul_WEO" },
  { BDK_Hangul_WE, "Hangul_WE" },
  { BDK_Hangul_WI, "Hangul_WI" },
  { BDK_Hangul_YU, "Hangul_YU" },
  { BDK_Hangul_EU, "Hangul_EU" },
  { BDK_Hangul_YI, "Hangul_YI" },
  { BDK_Hangul_I, "Hangul_I" },
  { BDK_Hangul_J_Kiyeog, "Hangul_J_Kiyeog" },
  { BDK_Hangul_J_SsangKiyeog, "Hangul_J_SsangKiyeog" },
  { BDK_Hangul_J_KiyeogSios, "Hangul_J_KiyeogSios" },
  { BDK_Hangul_J_Nieun, "Hangul_J_Nieun" },
  { BDK_Hangul_J_NieunJieuj, "Hangul_J_NieunJieuj" },
  { BDK_Hangul_J_NieunHieuh, "Hangul_J_NieunHieuh" },
  { BDK_Hangul_J_Dikeud, "Hangul_J_Dikeud" },
  { BDK_Hangul_J_Rieul, "Hangul_J_Rieul" },
  { BDK_Hangul_J_RieulKiyeog, "Hangul_J_RieulKiyeog" },
  { BDK_Hangul_J_RieulMieum, "Hangul_J_RieulMieum" },
  { BDK_Hangul_J_RieulPieub, "Hangul_J_RieulPieub" },
  { BDK_Hangul_J_RieulSios, "Hangul_J_RieulSios" },
  { BDK_Hangul_J_RieulTieut, "Hangul_J_RieulTieut" },
  { BDK_Hangul_J_RieulPhieuf, "Hangul_J_RieulPhieuf" },
  { BDK_Hangul_J_RieulHieuh, "Hangul_J_RieulHieuh" },
  { BDK_Hangul_J_Mieum, "Hangul_J_Mieum" },
  { BDK_Hangul_J_Pieub, "Hangul_J_Pieub" },
  { BDK_Hangul_J_PieubSios, "Hangul_J_PieubSios" },
  { BDK_Hangul_J_Sios, "Hangul_J_Sios" },
  { BDK_Hangul_J_SsangSios, "Hangul_J_SsangSios" },
  { BDK_Hangul_J_Ieung, "Hangul_J_Ieung" },
  { BDK_Hangul_J_Jieuj, "Hangul_J_Jieuj" },
  { BDK_Hangul_J_Cieuc, "Hangul_J_Cieuc" },
  { BDK_Hangul_J_Khieuq, "Hangul_J_Khieuq" },
  { BDK_Hangul_J_Tieut, "Hangul_J_Tieut" },
  { BDK_Hangul_J_Phieuf, "Hangul_J_Phieuf" },
  { BDK_Hangul_J_Hieuh, "Hangul_J_Hieuh" },
  { BDK_Hangul_RieulYeorinHieuh, "Hangul_RieulYeorinHieuh" },
  { BDK_Hangul_SunkyeongeumMieum, "Hangul_SunkyeongeumMieum" },
  { BDK_Hangul_SunkyeongeumPieub, "Hangul_SunkyeongeumPieub" },
  { BDK_Hangul_PanSios, "Hangul_PanSios" },
  { BDK_Hangul_KkogjiDalrinIeung, "Hangul_KkogjiDalrinIeung" },
  { BDK_Hangul_SunkyeongeumPhieuf, "Hangul_SunkyeongeumPhieuf" },
  { BDK_Hangul_YeorinHieuh, "Hangul_YeorinHieuh" },
  { BDK_Hangul_AraeA, "Hangul_AraeA" },
  { BDK_Hangul_AraeAE, "Hangul_AraeAE" },
  { BDK_Hangul_J_PanSios, "Hangul_J_PanSios" },
  { BDK_Hangul_J_KkogjiDalrinIeung, "Hangul_J_KkogjiDalrinIeung" },
  { BDK_Hangul_J_YeorinHieuh, "Hangul_J_YeorinHieuh" },
  { BDK_Korean_Won, "Korean_Won" },
  { BDK_OE, "OE" },
  { BDK_oe, "oe" },
  { BDK_Ydiaeresis, "Ydiaeresis" },
  { BDK_EcuSign, "EcuSign" },
  { BDK_ColonSign, "ColonSign" },
  { BDK_CruzeiroSign, "CruzeiroSign" },
  { BDK_FFrancSign, "FFrancSign" },
  { BDK_LiraSign, "LiraSign" },
  { BDK_MillSign, "MillSign" },
  { BDK_NairaSign, "NairaSign" },
  { BDK_PesetaSign, "PesetaSign" },
  { BDK_RupeeSign, "RupeeSign" },
  { BDK_WonSign, "WonSign" },
  { BDK_NewSheqelSign, "NewSheqelSign" },
  { BDK_DongSign, "DongSign" },
  { BDK_EuroSign, "EuroSign" },
  { BDK_3270_Duplicate, "3270_Duplicate" },
  { BDK_3270_FieldMark, "3270_FieldMark" },
  { BDK_3270_Right2, "3270_Right2" },
  { BDK_3270_Left2, "3270_Left2" },
  { BDK_3270_BackTab, "3270_BackTab" },
  { BDK_3270_EraseEOF, "3270_EraseEOF" },
  { BDK_3270_EraseInput, "3270_EraseInput" },
  { BDK_3270_Reset, "3270_Reset" },
  { BDK_3270_Quit, "3270_Quit" },
  { BDK_3270_PA1, "3270_PA1" },
  { BDK_3270_PA2, "3270_PA2" },
  { BDK_3270_PA3, "3270_PA3" },
  { BDK_3270_Test, "3270_Test" },
  { BDK_3270_Attn, "3270_Attn" },
  { BDK_3270_CursorBlink, "3270_CursorBlink" },
  { BDK_3270_AltCursor, "3270_AltCursor" },
  { BDK_3270_KeyClick, "3270_KeyClick" },
  { BDK_3270_Jump, "3270_Jump" },
  { BDK_3270_Ident, "3270_Ident" },
  { BDK_3270_Rule, "3270_Rule" },
  { BDK_3270_Copy, "3270_Copy" },
  { BDK_3270_Play, "3270_Play" },
  { BDK_3270_Setup, "3270_Setup" },
  { BDK_3270_Record, "3270_Record" },
  { BDK_3270_ChangeScreen, "3270_ChangeScreen" },
  { BDK_3270_DeleteWord, "3270_DeleteWord" },
  { BDK_3270_ExSelect, "3270_ExSelect" },
  { BDK_3270_CursorSelect, "3270_CursorSelect" },
  { BDK_3270_PrintScreen, "3270_PrintScreen" },
  { BDK_3270_Enter, "3270_Enter" },
  { BDK_ISO_Lock, "ISO_Lock" },
  { BDK_ISO_Level2_Latch, "ISO_Level2_Latch" },
  { BDK_ISO_Level3_Shift, "ISO_Level3_Shift" },
  { BDK_ISO_Level3_Latch, "ISO_Level3_Latch" },
  { BDK_ISO_Level3_Lock, "ISO_Level3_Lock" },
  { BDK_ISO_Group_Latch, "ISO_Group_Latch" },
  { BDK_ISO_Group_Lock, "ISO_Group_Lock" },
  { BDK_ISO_Next_Group, "ISO_Next_Group" },
  { BDK_ISO_Next_Group_Lock, "ISO_Next_Group_Lock" },
  { BDK_ISO_Prev_Group, "ISO_Prev_Group" },
  { BDK_ISO_Prev_Group_Lock, "ISO_Prev_Group_Lock" },
  { BDK_ISO_First_Group, "ISO_First_Group" },
  { BDK_ISO_First_Group_Lock, "ISO_First_Group_Lock" },
  { BDK_ISO_Last_Group, "ISO_Last_Group" },
  { BDK_ISO_Last_Group_Lock, "ISO_Last_Group_Lock" },
  { BDK_ISO_Left_Tab, "ISO_Left_Tab" },
  { BDK_ISO_Move_Line_Up, "ISO_Move_Line_Up" },
  { BDK_ISO_Move_Line_Down, "ISO_Move_Line_Down" },
  { BDK_ISO_Partial_Line_Up, "ISO_Partial_Line_Up" },
  { BDK_ISO_Partial_Line_Down, "ISO_Partial_Line_Down" },
  { BDK_ISO_Partial_Space_Left, "ISO_Partial_Space_Left" },
  { BDK_ISO_Partial_Space_Right, "ISO_Partial_Space_Right" },
  { BDK_ISO_Set_Margin_Left, "ISO_Set_Margin_Left" },
  { BDK_ISO_Set_Margin_Right, "ISO_Set_Margin_Right" },
  { BDK_ISO_Release_Margin_Left, "ISO_Release_Margin_Left" },
  { BDK_ISO_Release_Margin_Right, "ISO_Release_Margin_Right" },
  { BDK_ISO_Release_Both_Margins, "ISO_Release_Both_Margins" },
  { BDK_ISO_Fast_Cursor_Left, "ISO_Fast_Cursor_Left" },
  { BDK_ISO_Fast_Cursor_Right, "ISO_Fast_Cursor_Right" },
  { BDK_ISO_Fast_Cursor_Up, "ISO_Fast_Cursor_Up" },
  { BDK_ISO_Fast_Cursor_Down, "ISO_Fast_Cursor_Down" },
  { BDK_ISO_Continuous_Underline, "ISO_Continuous_Underline" },
  { BDK_ISO_Discontinuous_Underline, "ISO_Discontinuous_Underline" },
  { BDK_ISO_Emphasize, "ISO_Emphasize" },
  { BDK_ISO_Center_Object, "ISO_Center_Object" },
  { BDK_ISO_Enter, "ISO_Enter" },
  { BDK_dead_grave, "dead_grave" },
  { BDK_dead_acute, "dead_acute" },
  { BDK_dead_circumflex, "dead_circumflex" },
  { BDK_dead_tilde, "dead_tilde" },
  { BDK_dead_macron, "dead_macron" },
  { BDK_dead_breve, "dead_breve" },
  { BDK_dead_abovedot, "dead_abovedot" },
  { BDK_dead_diaeresis, "dead_diaeresis" },
  { BDK_dead_abovering, "dead_abovering" },
  { BDK_dead_doubleacute, "dead_doubleacute" },
  { BDK_dead_caron, "dead_caron" },
  { BDK_dead_cedilla, "dead_cedilla" },
  { BDK_dead_ogonek, "dead_ogonek" },
  { BDK_dead_iota, "dead_iota" },
  { BDK_dead_voiced_sound, "dead_voiced_sound" },
  { BDK_dead_semivoiced_sound, "dead_semivoiced_sound" },
  { BDK_dead_belowdot, "dead_belowdot" },
  { BDK_AccessX_Enable, "AccessX_Enable" },
  { BDK_AccessX_Feedback_Enable, "AccessX_Feedback_Enable" },
  { BDK_RepeatKeys_Enable, "RepeatKeys_Enable" },
  { BDK_SlowKeys_Enable, "SlowKeys_Enable" },
  { BDK_BounceKeys_Enable, "BounceKeys_Enable" },
  { BDK_StickyKeys_Enable, "StickyKeys_Enable" },
  { BDK_MouseKeys_Enable, "MouseKeys_Enable" },
  { BDK_MouseKeys_Accel_Enable, "MouseKeys_Accel_Enable" },
  { BDK_Overlay1_Enable, "Overlay1_Enable" },
  { BDK_Overlay2_Enable, "Overlay2_Enable" },
  { BDK_AudibleBell_Enable, "AudibleBell_Enable" },
  { BDK_First_Virtual_Screen, "First_Virtual_Screen" },
  { BDK_Prev_Virtual_Screen, "Prev_Virtual_Screen" },
  { BDK_Next_Virtual_Screen, "Next_Virtual_Screen" },
  { BDK_Last_Virtual_Screen, "Last_Virtual_Screen" },
  { BDK_Terminate_Server, "Terminate_Server" },
  { BDK_Pointer_Left, "Pointer_Left" },
  { BDK_Pointer_Right, "Pointer_Right" },
  { BDK_Pointer_Up, "Pointer_Up" },
  { BDK_Pointer_Down, "Pointer_Down" },
  { BDK_Pointer_UpLeft, "Pointer_UpLeft" },
  { BDK_Pointer_UpRight, "Pointer_UpRight" },
  { BDK_Pointer_DownLeft, "Pointer_DownLeft" },
  { BDK_Pointer_DownRight, "Pointer_DownRight" },
  { BDK_Pointer_Button_Dflt, "Pointer_Button_Dflt" },
  { BDK_Pointer_Button1, "Pointer_Button1" },
  { BDK_Pointer_Button2, "Pointer_Button2" },
  { BDK_Pointer_Button3, "Pointer_Button3" },
  { BDK_Pointer_Button4, "Pointer_Button4" },
  { BDK_Pointer_Button5, "Pointer_Button5" },
  { BDK_Pointer_DblClick_Dflt, "Pointer_DblClick_Dflt" },
  { BDK_Pointer_DblClick1, "Pointer_DblClick1" },
  { BDK_Pointer_DblClick2, "Pointer_DblClick2" },
  { BDK_Pointer_DblClick3, "Pointer_DblClick3" },
  { BDK_Pointer_DblClick4, "Pointer_DblClick4" },
  { BDK_Pointer_DblClick5, "Pointer_DblClick5" },
  { BDK_Pointer_Drag_Dflt, "Pointer_Drag_Dflt" },
  { BDK_Pointer_Drag1, "Pointer_Drag1" },
  { BDK_Pointer_Drag2, "Pointer_Drag2" },
  { BDK_Pointer_Drag3, "Pointer_Drag3" },
  { BDK_Pointer_Drag4, "Pointer_Drag4" },
  { BDK_Pointer_EnableKeys, "Pointer_EnableKeys" },
  { BDK_Pointer_Accelerate, "Pointer_Accelerate" },
  { BDK_Pointer_DfltBtnNext, "Pointer_DfltBtnNext" },
  { BDK_Pointer_DfltBtnPrev, "Pointer_DfltBtnPrev" },
  { BDK_Pointer_Drag5, "Pointer_Drag5" },
  { BDK_BackSpace, "BackSpace" },
  { BDK_Tab, "Tab" },
  { BDK_Linefeed, "Linefeed" },
  { BDK_Clear, "Clear" },
  { BDK_Return, "Return" },
  { BDK_Pause, "Pause" },
  { BDK_Scroll_Lock, "Scroll_Lock" },
  { BDK_Sys_Req, "Sys_Req" },
  { BDK_Escape, "Escape" },
  { BDK_Multi_key, "Multi_key" },
  { BDK_Kanji, "Kanji" },
  { BDK_Muhenkan, "Muhenkan" },
  { BDK_Henkan_Mode, "Henkan_Mode" },
  { BDK_Henkan, "Henkan" },
  { BDK_Romaji, "Romaji" },
  { BDK_Hiragana, "Hiragana" },
  { BDK_Katakana, "Katakana" },
  { BDK_Hiragana_Katakana, "Hiragana_Katakana" },
  { BDK_Zenkaku, "Zenkaku" },
  { BDK_Hankaku, "Hankaku" },
  { BDK_Zenkaku_Hankaku, "Zenkaku_Hankaku" },
  { BDK_Touroku, "Touroku" },
  { BDK_Massyo, "Massyo" },
  { BDK_Kana_Lock, "Kana_Lock" },
  { BDK_Kana_Shift, "Kana_Shift" },
  { BDK_Eisu_Shift, "Eisu_Shift" },
  { BDK_Eisu_toggle, "Eisu_toggle" },
  { BDK_Hangul, "Hangul" },
  { BDK_Hangul_Start, "Hangul_Start" },
  { BDK_Hangul_End, "Hangul_End" },
  { BDK_Hangul_Hanja, "Hangul_Hanja" },
  { BDK_Hangul_Jamo, "Hangul_Jamo" },
  { BDK_Hangul_Romaja, "Hangul_Romaja" },
  { BDK_Codeinput, "Codeinput" },
  { BDK_Kanji_Bangou, "Kanji_Bangou" },
  { BDK_Hangul_Codeinput, "Hangul_Codeinput" },
  { BDK_Hangul_Jeonja, "Hangul_Jeonja" },
  { BDK_Hangul_Banja, "Hangul_Banja" },
  { BDK_Hangul_PreHanja, "Hangul_PreHanja" },
  { BDK_Hangul_PostHanja, "Hangul_PostHanja" },
  { BDK_SingleCandidate, "SingleCandidate" },
  { BDK_Hangul_SingleCandidate, "Hangul_SingleCandidate" },
  { BDK_MultipleCandidate, "MultipleCandidate" },
  { BDK_Zen_Koho, "Zen_Koho" },
  { BDK_Hangul_MultipleCandidate, "Hangul_MultipleCandidate" },
  { BDK_PreviousCandidate, "PreviousCandidate" },
  { BDK_Mae_Koho, "Mae_Koho" },
  { BDK_Hangul_PreviousCandidate, "Hangul_PreviousCandidate" },
  { BDK_Hangul_Special, "Hangul_Special" },
  { BDK_Home, "Home" },
  { BDK_Left, "Left" },
  { BDK_Up, "Up" },
  { BDK_Right, "Right" },
  { BDK_Down, "Down" },
  { BDK_Prior, "Prior" },
  { BDK_Page_Up, "Page_Up" },
  { BDK_Next, "Next" },
  { BDK_Page_Down, "Page_Down" },
  { BDK_End, "End" },
  { BDK_Begin, "Begin" },
  { BDK_Select, "Select" },
  { BDK_Print, "Print" },
  { BDK_Execute, "Execute" },
  { BDK_Insert, "Insert" },
  { BDK_Undo, "Undo" },
  { BDK_Redo, "Redo" },
  { BDK_Menu, "Menu" },
  { BDK_Find, "Find" },
  { BDK_Cancel, "Cancel" },
  { BDK_Help, "Help" },
  { BDK_Break, "Break" },
  { BDK_Mode_switch, "Mode_switch" },
  { BDK_script_switch, "script_switch" },
  { BDK_ISO_Group_Shift, "ISO_Group_Shift" },
  { BDK_kana_switch, "kana_switch" },
  { BDK_Arabic_switch, "Arabic_switch" },
  { BDK_Greek_switch, "Greek_switch" },
  { BDK_Hebrew_switch, "Hebrew_switch" },
  { BDK_Hangul_switch, "Hangul_switch" },
  { BDK_Num_Lock, "Num_Lock" },
  { BDK_KP_Space, "KP_Space" },
  { BDK_KP_Tab, "KP_Tab" },
  { BDK_KP_Enter, "KP_Enter" },
  { BDK_KP_F1, "KP_F1" },
  { BDK_KP_F2, "KP_F2" },
  { BDK_KP_F3, "KP_F3" },
  { BDK_KP_F4, "KP_F4" },
  { BDK_KP_Home, "KP_Home" },
  { BDK_KP_Left, "KP_Left" },
  { BDK_KP_Up, "KP_Up" },
  { BDK_KP_Right, "KP_Right" },
  { BDK_KP_Down, "KP_Down" },
  { BDK_KP_Prior, "KP_Prior" },
  { BDK_KP_Page_Up, "KP_Page_Up" },
  { BDK_KP_Next, "KP_Next" },
  { BDK_KP_Page_Down, "KP_Page_Down" },
  { BDK_KP_End, "KP_End" },
  { BDK_KP_Begin, "KP_Begin" },
  { BDK_KP_Insert, "KP_Insert" },
  { BDK_KP_Delete, "KP_Delete" },
  { BDK_KP_Multiply, "KP_Multiply" },
  { BDK_KP_Add, "KP_Add" },
  { BDK_KP_Separator, "KP_Separator" },
  { BDK_KP_Subtract, "KP_Subtract" },
  { BDK_KP_Decimal, "KP_Decimal" },
  { BDK_KP_Divide, "KP_Divide" },
  { BDK_KP_0, "KP_0" },
  { BDK_KP_1, "KP_1" },
  { BDK_KP_2, "KP_2" },
  { BDK_KP_3, "KP_3" },
  { BDK_KP_4, "KP_4" },
  { BDK_KP_5, "KP_5" },
  { BDK_KP_6, "KP_6" },
  { BDK_KP_7, "KP_7" },
  { BDK_KP_8, "KP_8" },
  { BDK_KP_9, "KP_9" },
  { BDK_KP_Equal, "KP_Equal" },
  { BDK_F1, "F1" },
  { BDK_F2, "F2" },
  { BDK_F3, "F3" },
  { BDK_F4, "F4" },
  { BDK_F5, "F5" },
  { BDK_F6, "F6" },
  { BDK_F7, "F7" },
  { BDK_F8, "F8" },
  { BDK_F9, "F9" },
  { BDK_F10, "F10" },
  { BDK_F11, "F11" },
  { BDK_F12, "F12" },
  { BDK_F13, "F13" },
  { BDK_F14, "F14" },
  { BDK_F15, "F15" },
  { BDK_F16, "F16" },
  { BDK_F17, "F17" },
  { BDK_F18, "F18" },
  { BDK_F19, "F19" },
  { BDK_F20, "F20" },
  { BDK_F21, "F21" },
  { BDK_F22, "F22" },
  { BDK_F23, "F23" },
  { BDK_F24, "F24" },
  { BDK_F25, "F25" },
  { BDK_F26, "F26" },
  { BDK_F27, "F27" },
  { BDK_F28, "F28" },
  { BDK_F29, "F29" },
  { BDK_F30, "F30" },
  { BDK_F31, "F31" },
  { BDK_F32, "F32" },
  { BDK_F33, "F33" },
  { BDK_F34, "F34" },
  { BDK_F35, "F35" },
  { BDK_Shift_L, "Shift_L" },
  { BDK_Shift_R, "Shift_R" },
  { BDK_Control_L, "Control_L" },
  { BDK_Control_R, "Control_R" },
  { BDK_Caps_Lock, "Caps_Lock" },
  { BDK_Shift_Lock, "Shift_Lock" },
  { BDK_Meta_L, "Meta_L" },
  { BDK_Meta_R, "Meta_R" },
  { BDK_Alt_L, "Alt_L" },
  { BDK_Alt_R, "Alt_R" },
  { BDK_Super_L, "Super_L" },
  { BDK_Super_R, "Super_R" },
  { BDK_Hyper_L, "Hyper_L" },
  { BDK_Hyper_R, "Hyper_R" },
  { BDK_Delete, "Delete" },
  { BDK_VoidSymbol, "VoidSymbol" }
};

#define BDK_NUM_KEYS (G_N_ELEMENTS (bdk_keys_by_keyval))


static bint
bdk_keys_keyval_compare (const void *pkey,
                         const void *pbase)
{
  return (*(bint *) pkey) - ((struct bdk_key *) pbase)->keyval;
}

bchar *
bdk_keyval_name (buint keyval)
{
  struct bdk_key *found;

  /* these keyvals have two entries (PageUp/Prior | PageDown/Next) */
  switch (keyval)
    {
    case BDK_Page_Up:
      return "Page_Up";
    case BDK_Page_Down:
      return "Page_Down";
    case BDK_KP_Page_Up:
      return "KP_Page_Up";
    case BDK_KP_Page_Down:
      return "KP_Page_Down";
    }

  found = bsearch (&keyval, bdk_keys_by_keyval,
                   BDK_NUM_KEYS, sizeof (struct bdk_key),
                   bdk_keys_keyval_compare);

  if (found)
    return (bchar *) found->name;
  else
    return NULL;
}

static bint
bdk_key_compare_by_name (const void *a,
                         const void *b)
{
  return strcmp (((const struct bdk_key *) a)->name,
                 ((const struct bdk_key *) b)->name);
}

static bint
bdk_keys_name_compare (const void *pkey,
                       const void *pbase)
{
  return strcmp ((const char *) pkey,
                 ((const struct bdk_key *) pbase)->name);
}

buint
bdk_keyval_from_name (const bchar *keyval_name)
{
  struct bdk_key *found;

  g_return_val_if_fail (keyval_name != NULL, 0);

  if (bdk_keys_by_name == NULL)
    {
      bdk_keys_by_name = g_new (struct bdk_key, BDK_NUM_KEYS);

      memcpy (bdk_keys_by_name, bdk_keys_by_keyval,
              BDK_NUM_KEYS * sizeof (struct bdk_key));

      qsort (bdk_keys_by_name, BDK_NUM_KEYS, sizeof (struct bdk_key),
             bdk_key_compare_by_name);
    }

  found = bsearch (keyval_name, bdk_keys_by_name,
                   BDK_NUM_KEYS, sizeof (struct bdk_key),
                   bdk_keys_name_compare);

  if (found)
    return found->keyval;
  else
    return BDK_VoidSymbol;
}

static void
bdk_directfb_convert_modifiers (DFBInputDeviceModifierMask dfbmod,
                                DFBInputDeviceLockState    dfblock)
{
  if (dfbmod & DIMM_ALT)
    _bdk_directfb_modifiers |= BDK_MOD1_MASK;
  else
    _bdk_directfb_modifiers &= ~BDK_MOD1_MASK;

  if (dfbmod & DIMM_ALTGR)
    _bdk_directfb_modifiers |= BDK_MOD2_MASK;
  else
    _bdk_directfb_modifiers &= ~BDK_MOD2_MASK;

  if (dfbmod & DIMM_CONTROL)
    _bdk_directfb_modifiers |= BDK_CONTROL_MASK;
  else
    _bdk_directfb_modifiers &= ~BDK_CONTROL_MASK;

  if (dfbmod & DIMM_SHIFT)
    _bdk_directfb_modifiers |= BDK_SHIFT_MASK;
  else
    _bdk_directfb_modifiers &= ~BDK_SHIFT_MASK;

  if (dfblock & DILS_CAPS)
    _bdk_directfb_modifiers |= BDK_LOCK_MASK;
  else
    _bdk_directfb_modifiers &= ~BDK_LOCK_MASK;
}

static buint
bdk_directfb_translate_key (DFBInputDeviceKeyIdentifier key_id,
                            DFBInputDeviceKeySymbol     key_symbol)
{
  buint keyval = BDK_VoidSymbol;

  /* special case numpad */
  if (key_id >= DIKI_KP_DIV && key_id <= DIKI_KP_9)
    {
      switch (key_symbol)
        {
        case DIKS_SLASH:         keyval = BDK_KP_Divide;    break;
        case DIKS_ASTERISK:      keyval = BDK_KP_Multiply;  break;
        case DIKS_PLUS_SIGN:     keyval = BDK_KP_Add;       break;
        case DIKS_MINUS_SIGN:    keyval = BDK_KP_Subtract;  break;
        case DIKS_ENTER:         keyval = BDK_KP_Enter;     break;
        case DIKS_SPACE:         keyval = BDK_KP_Space;     break;
        case DIKS_TAB:           keyval = BDK_KP_Tab;       break;
        case DIKS_EQUALS_SIGN:   keyval = BDK_KP_Equal;     break;
        case DIKS_COMMA:
        case DIKS_PERIOD:        keyval = BDK_KP_Decimal;   break;
        case DIKS_HOME:          keyval = BDK_KP_Home;      break;
        case DIKS_END:           keyval = BDK_KP_End;       break;
        case DIKS_PAGE_UP:       keyval = BDK_KP_Page_Up;   break;
        case DIKS_PAGE_DOWN:     keyval = BDK_KP_Page_Down; break;
        case DIKS_CURSOR_LEFT:   keyval = BDK_KP_Left;      break;
        case DIKS_CURSOR_RIGHT:  keyval = BDK_KP_Right;     break;
        case DIKS_CURSOR_UP:     keyval = BDK_KP_Up;        break;
        case DIKS_CURSOR_DOWN:   keyval = BDK_KP_Down;      break;
        case DIKS_BEGIN:         keyval = BDK_KP_Begin;     break;

        case DIKS_0 ... DIKS_9:
          keyval = BDK_KP_0 + key_symbol - DIKS_0;
          break;
        case DIKS_F1 ... DIKS_F4:
          keyval = BDK_KP_F1 + key_symbol - DIKS_F1;
          break;

        default:
          break;
        }
    }
  else
    {
      switch (DFB_KEY_TYPE (key_symbol))
        {
        case DIKT_UNICODE:
          switch (key_symbol)
            {
            case DIKS_NULL:       keyval = BDK_VoidSymbol; break;
            case DIKS_BACKSPACE:  keyval = BDK_BackSpace;  break;
            case DIKS_TAB:        keyval = BDK_Tab;        break;
            case DIKS_RETURN:     keyval = BDK_Return;     break;
            case DIKS_CANCEL:     keyval = BDK_Cancel;     break;
            case DIKS_ESCAPE:     keyval = BDK_Escape;     break;
            case DIKS_SPACE:      keyval = BDK_space;      break;
            case DIKS_DELETE:     keyval = BDK_Delete;     break;

            default:
              keyval = bdk_unicode_to_keyval (key_symbol);
              if (keyval & 0x01000000)
                keyval = BDK_VoidSymbol;
            }
          break;

        case DIKT_SPECIAL:
          switch (key_symbol)
            {
            case DIKS_CURSOR_LEFT:   keyval = BDK_Left;      break;
            case DIKS_CURSOR_RIGHT:  keyval = BDK_Right;     break;
            case DIKS_CURSOR_UP:     keyval = BDK_Up;        break;
            case DIKS_CURSOR_DOWN:   keyval = BDK_Down;      break;
            case DIKS_INSERT:        keyval = BDK_Insert;    break;
            case DIKS_HOME:          keyval = BDK_Home;      break;
            case DIKS_END:           keyval = BDK_End;       break;
            case DIKS_PAGE_UP:       keyval = BDK_Page_Up;   break;
            case DIKS_PAGE_DOWN:     keyval = BDK_Page_Down; break;
            case DIKS_PRINT:         keyval = BDK_Print;     break;
            case DIKS_PAUSE:         keyval = BDK_Pause;     break;
            case DIKS_SELECT:        keyval = BDK_Select;    break;
            case DIKS_CLEAR:         keyval = BDK_Clear;     break;
            case DIKS_MENU:          keyval = BDK_Menu;      break;
            case DIKS_HELP:          keyval = BDK_Help;      break;
            case DIKS_NEXT:          keyval = BDK_Next;      break;
            case DIKS_BEGIN:         keyval = BDK_Begin;     break;
            case DIKS_BREAK:         keyval = BDK_Break;     break;
            default:
              break;
            }
          break;

        case DIKT_FUNCTION:
          keyval = BDK_F1 + key_symbol - DIKS_F1;
          if (keyval > BDK_F35)
            keyval = BDK_VoidSymbol;
          break;

        case DIKT_MODIFIER:
          switch (key_id)
            {
            case DIKI_SHIFT_L:    keyval = BDK_Shift_L;     break;
            case DIKI_SHIFT_R:    keyval = BDK_Shift_R;     break;
            case DIKI_CONTROL_L:  keyval = BDK_Control_L;   break;
            case DIKI_CONTROL_R:  keyval = BDK_Control_R;   break;
            case DIKI_ALT_L:      keyval = BDK_Alt_L;       break;
            case DIKI_ALT_R:      keyval = BDK_Alt_R;       break;
            case DIKI_META_L:     keyval = BDK_Meta_L;      break;
            case DIKI_META_R:     keyval = BDK_Meta_R;      break;
            case DIKI_SUPER_L:    keyval = BDK_Super_L;     break;
            case DIKI_SUPER_R:    keyval = BDK_Super_R;     break;
            case DIKI_HYPER_L:    keyval = BDK_Hyper_L;     break;
            case DIKI_HYPER_R:    keyval = BDK_Hyper_R;     break;
            default:
              break;
            }
          break;

        case DIKT_LOCK:
          switch (key_symbol)
            {
            case DIKS_CAPS_LOCK:    keyval = BDK_Caps_Lock;   break;
            case DIKS_NUM_LOCK:     keyval = BDK_Num_Lock;    break;
            case DIKS_SCROLL_LOCK:  keyval = BDK_Scroll_Lock; break;
            default:
              break;
            }
          break;

        case DIKT_DEAD:
          /* dead keys are handled directly by directfb */
          break;

        case DIKT_CUSTOM:
          break;
        }
    }

  return keyval;
}

void
_bdk_directfb_keyboard_init (void)
{
  DFBInputDeviceDescription  desc;
  bint                       i, n, length;
  IDirectFBInputDevice      *keyboard = _bdk_display->keyboard;

  if (!keyboard)
    return;
  if (directfb_keymap)
    return;

  keyboard->GetDescription (keyboard, &desc);
  _bdk_display->keymap = g_object_new (bdk_keymap_get_type (), NULL);

  if (desc.min_keycode < 0 || desc.max_keycode < desc.min_keycode)
    return;

  directfb_min_keycode = desc.min_keycode;
  directfb_max_keycode = desc.max_keycode;

  length = directfb_max_keycode - desc.min_keycode + 1;


  directfb_keymap = g_new0 (buint, 4 * length);

  for (i = 0; i < length; i++)
    {
      DFBInputDeviceKeymapEntry  entry;

      if (keyboard->GetKeymapEntry (keyboard,
                                    i + desc.min_keycode, &entry) != DFB_OK)
        continue;
      for (n = 0; n < 4; n++) {
        directfb_keymap[i * 4 + n] =
          bdk_directfb_translate_key (entry.identifier, entry.symbols[n]);
      }
    }
}

void
_bdk_directfb_keyboard_exit (void)
{
  if (!directfb_keymap)
    return;

  g_free (directfb_keymap);
  g_free (_bdk_display->keymap);
  _bdk_display->keymap = NULL;
  directfb_keymap = NULL;
}

void
bdk_directfb_translate_key_event (DFBWindowEvent *dfb_event,
                                  BdkEventKey    *event)
{
  bint  len;
  bchar buf[6];

  g_return_if_fail (dfb_event != NULL);
  g_return_if_fail (event != NULL);

  bdk_directfb_convert_modifiers (dfb_event->modifiers, dfb_event->locks);

  event->state            = _bdk_directfb_modifiers;
  event->group            = (dfb_event->modifiers & DIMM_ALTGR) ? 1 : 0;
  event->hardware_keycode = dfb_event->key_code;
  event->keyval           = bdk_directfb_translate_key (dfb_event->key_id,
                                                        dfb_event->key_symbol);
  /* If the device driver didn't send a key_code (happens with remote
     controls), we try to find a suitable key_code by looking at the
     default keymap. */
  if (dfb_event->key_code == -1 && directfb_keymap)
    {
      bint i;

      for (i = directfb_min_keycode; i <= directfb_max_keycode; i++)
        {
          if (directfb_keymap[(i - directfb_min_keycode) * 4] == event->keyval)
            {
              event->hardware_keycode = i;
              break;
            }
        }
    }

  len = g_unichar_to_utf8 (dfb_event->key_symbol, buf);

  event->string = g_strndup (buf, len);
  event->length = len;
}

/**
 * bdk_keymap_get_caps_lock_state:
 * @keymap: a #BdkKeymap
 *
 * Returns whether the Caps Lock modifer is locked.
 *
 * Returns: %TRUE if Caps Lock is on
 *
 * Since: 2.16
 */
bboolean
bdk_keymap_get_caps_lock_state (BdkKeymap *keymap)
{
  IDirectFBInputDevice *keyboard = _bdk_display->keyboard;

  if (keyboard)
    {
      DFBInputDeviceLockState  state;

      if (keyboard->GetLockState (keyboard, &state) == DFB_OK)
        return ((state & DILS_CAPS) != 0);
    }

  return FALSE;
}

/**
 * bdk_keymap_get_entries_for_keycode:
 * @keymap: a #BdkKeymap or %NULL to use the default keymap
 * @hardware_keycode: a keycode
 * @keys: return location for array of #BdkKeymapKey, or NULL
 * @keyvals: return location for array of keyvals, or NULL
 * @n_entries: length of @keys and @keyvals
 *
 * Returns the keyvals bound to @hardware_keycode.
 * The Nth #BdkKeymapKey in @keys is bound to the Nth
 * keyval in @keyvals. Free the returned arrays with g_free().
 * When a keycode is pressed by the user, the keyval from
 * this list of entries is selected by considering the effective
 * keyboard group and level. See bdk_keymap_translate_keyboard_state().
 *
 * Returns: %TRUE if there were any entries
 **/
bboolean
bdk_keymap_get_entries_for_keycode (BdkKeymap     *keymap,
                                    buint          hardware_keycode,
                                    BdkKeymapKey **keys,
                                    buint        **keyvals,
                                    bint          *n_entries)
{
  bint num = 0;
  bint i, j;
  bint index;

  index = (hardware_keycode - directfb_min_keycode) * 4;

  if (directfb_keymap && (hardware_keycode >= directfb_min_keycode &&
                          hardware_keycode <= directfb_max_keycode))
    {
      for (i = 0; i < 4; i++)
        if (directfb_keymap[index + i] != BDK_VoidSymbol)
          num++;
    }

  if (keys)
    {
      *keys = g_new (BdkKeymapKey, num);

      for (i = 0, j = 0; num > 0 && i < 4; i++)
        if (directfb_keymap[index + i] != BDK_VoidSymbol)
          {
            (*keys)[j].keycode = hardware_keycode;
            (*keys)[j].level   = j % 2;
            (*keys)[j].group   = j > DIKSI_BASE_SHIFT ? 1 : 0;
            j++;
          }
    }
  if (keyvals)
    {
      *keyvals = g_new (buint, num);

      for (i = 0, j = 0; num > 0 && i < 4; i++)
        if (directfb_keymap[index + i] != BDK_VoidSymbol)
          {
            (*keyvals)[j] = directfb_keymap[index + i];
            j++;
          }
    }

  if (n_entries)
    *n_entries = num;

  return (num > 0);
}

static inline void
append_keymap_key (GArray *array,
                   buint   keycode,
                   bint    group,
                   bint    level)
{
  BdkKeymapKey key = { keycode, group, level };

  g_array_append_val (array, key);
}

/**
 * bdk_keymap_get_entries_for_keyval:
 * @keymap: a #BdkKeymap, or %NULL to use the default keymap
 * @keyval: a keyval, such as %BDK_a, %BDK_Up, %BDK_Return, etc.
 * @keys: return location for an array of #BdkKeymapKey
 * @n_keys: return location for number of elements in returned array
 *
 * Obtains a list of keycode/group/level combinations that will
 * generate @keyval. Groups and levels are two kinds of keyboard mode;
 * in general, the level determines whether the top or bottom symbol
 * on a key is used, and the group determines whether the left or
 * right symbol is used. On US keyboards, the shift key changes the
 * keyboard level, and there are no groups. A group switch key might
 * convert a keyboard between Hebrew to English modes, for example.
 * #BdkEventKey contains a %group field that indicates the active
 * keyboard group. The level is computed from the modifier mask.
 * The returned array should be freed
 * with g_free().
 *
 * Return value: %TRUE if keys were found and returned
 **/
bboolean
bdk_keymap_get_entries_for_keyval (BdkKeymap     *keymap,
                                   buint          keyval,
                                   BdkKeymapKey **keys,
                                   bint          *n_keys)
{
  GArray *retval;
  bint    i, j;

  g_return_val_if_fail (keys != NULL, FALSE);
  g_return_val_if_fail (n_keys != NULL, FALSE);
  g_return_val_if_fail (keyval != BDK_VoidSymbol, FALSE);

  retval = g_array_new (FALSE, FALSE, sizeof (BdkKeymapKey));

  for (i = directfb_min_keycode;
       directfb_keymap && i <= directfb_max_keycode;
       i++)
    {
      bint index = i - directfb_min_keycode;

      for (j = 0; j < 4; j++)
        {
          if (directfb_keymap[index * 4 + j] == keyval)
            append_keymap_key (retval, i, j % 2, j > DIKSI_BASE_SHIFT ? 1 : 0);
        }
    }

  if (retval->len > 0)
    {
      *keys = (BdkKeymapKey *) retval->data;
      *n_keys = retval->len;
    }
  else
    {
      *keys = NULL;
      *n_keys = 0;
    }

  g_array_free (retval, retval->len > 0 ? FALSE : TRUE);

  return (*n_keys > 0);
}

bboolean
bdk_keymap_translate_keyboard_state (BdkKeymap       *keymap,
                                     buint            keycode,
                                     BdkModifierType  state,
                                     bint             group,
                                     buint           *keyval,
                                     bint            *effective_group,
                                     bint            *level,
                                     BdkModifierType *consumed_modifiers)
{
  if (directfb_keymap &&
      (keycode >= directfb_min_keycode && keycode <= directfb_max_keycode) &&
      (group == 0 || group == 1))
    {
      bint index = (keycode - directfb_min_keycode) * 4;
      bint i     = (state & BDK_SHIFT_MASK) ? 1 : 0;

      if (directfb_keymap[index + i + 2 * group] != BDK_VoidSymbol)
        {
          if (keyval)
            *keyval = directfb_keymap[index + i + 2 * group];

          if (group && directfb_keymap[index + i] == *keyval)
            {
              if (effective_group)
                *effective_group = 0;
              if (consumed_modifiers)
                *consumed_modifiers = 0;
            }
          else
            {
              if (effective_group)
                *effective_group = group;
              if (consumed_modifiers)
                *consumed_modifiers = BDK_MOD2_MASK;
            }
          if (level)
            *level = i;

          if (i && directfb_keymap[index + 2 * *effective_group] != *keyval)
            if (consumed_modifiers)
              *consumed_modifiers |= BDK_SHIFT_MASK;

          return TRUE;
        }
    }
  if (keyval)
    *keyval             = 0;
  if (effective_group)
    *effective_group    = 0;
  if (level)
    *level              = 0;
  if (consumed_modifiers)
    *consumed_modifiers = 0;

  return FALSE;
}

BdkKeymap *
bdk_keymap_get_for_display (BdkDisplay *display)
{
  if (display == NULL)
    return NULL;

  g_assert (BDK_IS_DISPLAY_DFB (display));

  BdkDisplayDFB *gdisplay = BDK_DISPLAY_DFB (display);

  g_assert (gdisplay->keymap != NULL);

  return gdisplay->keymap;
}

BangoDirection
bdk_keymap_get_direction (BdkKeymap *keymap)
{
  return BANGO_DIRECTION_NEUTRAL;
}

/**
 * bdk_keymap_lookup_key:
 * @keymap: a #BdkKeymap or %NULL to use the default keymap
 * @key: a #BdkKeymapKey with keycode, group, and level initialized
 *
 * Looks up the keyval mapped to a keycode/group/level triplet.
 * If no keyval is bound to @key, returns 0. For normal user input,
 * you want to use bdk_keymap_translate_keyboard_state() instead of
 * this function, since the effective group/level may not be
 * the same as the current keyboard state.
 *
 * Return value: a keyval, or 0 if none was mapped to the given @key
 **/
buint
bdk_keymap_lookup_key (BdkKeymap          *keymap,
                       const BdkKeymapKey *key)
{
  g_warning ("bdk_keymap_lookup_key unimplemented \n");
  return 0;
}

void
bdk_keymap_add_virtual_modifiers (BdkKeymap       *keymap,
                                  BdkModifierType *state)
{
  g_warning ("bdk_keymap_add_virtual_modifiers unimplemented \n");
}

bboolean
bdk_keymap_map_virtual_modifiers (BdkKeymap       *keymap,
                                  BdkModifierType *state)
{
  g_warning ("bdk_keymap_map_virtual_modifiers unimplemented \n");

  return TRUE;
}

#define __BDK_KEYS_DIRECTFB_C__
#include "bdkaliasdef.c"
