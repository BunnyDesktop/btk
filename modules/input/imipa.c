/* BTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Owen Taylor <otaylor@redhat.com>
 *
 */

#include "config.h"
#include <string.h>

#include "btk/btk.h"
#include "bdk/bdkkeysyms.h"

#include "btk/btkimmodule.h"
#include "btk/btkintl.h"

GType type_ipa = 0;

static void ipa_class_init (BtkIMContextSimpleClass *class);
static void ipa_init (BtkIMContextSimple *im_context);

static void
ipa_register_type (GTypeModule *module)
{
  const GTypeInfo object_info =
  {
    sizeof (BtkIMContextSimpleClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) ipa_class_init,
    NULL,           /* class_finalize */
    NULL,           /* class_data */
    sizeof (BtkIMContextSimple),
    0,
    (GInstanceInitFunc) ipa_init,
  };

  type_ipa = 
    g_type_module_register_type (module,
				 BTK_TYPE_IM_CONTEXT_SIMPLE,
				 "BtkIMContextIpa",
				 &object_info, 0);
}

/* The sequences here match the sequences used in the emacs quail
 * mode cryllic-translit; they allow entering all characters
 * in iso-8859-5
 */
static buint16 ipa_compose_seqs[] = {
  BDK_ampersand, 0,           0,      0,      0,      0x263, 	/* LATIN SMALL LETTER GAMMA */
  BDK_apostrophe, 0,          0,      0,      0,      0x2C8, 	/* MODIFIER LETTER VERTICAL LINE */
  BDK_slash,  BDK_apostrophe, 0,      0,      0,      0x2CA, 	/* MODIFIER LETTER ACUTE ACCENT */
  BDK_slash,  BDK_slash,      0,      0,      0,      0x02F, 	/* SOLIDUS */
  BDK_slash,  BDK_3,          0,      0,      0,      0x25B, 	/* LATIN SMALL LETTER OPEN E */
  BDK_slash,  BDK_A,          0,      0,      0,      0x252, 	/* LATIN LETTER TURNED ALPHA */
  BDK_slash,  BDK_R,          0,      0,      0,      0x281, 	/* LATIN LETTER SMALL CAPITAL INVERTED R */
  BDK_slash,  BDK_a,          0,      0,      0,      0x250, 	/* LATIN SMALL LETTER TURNED A */
  BDK_slash,  BDK_c,          0,      0,      0,      0x254, 	/* LATIN SMALL LETTER OPEN O */
  BDK_slash,  BDK_e,          0,      0,      0,      0x259, 	/* LATIN SMALL LETTER SCHWA */
  BDK_slash,  BDK_h,          0,      0,      0,      0x265, 	/* LATIN SMALL LETTER TURNED H */
  BDK_slash,  BDK_m,          0,      0,      0,      0x26F, 	/* LATIN SMALL LETTER TURNED M */
  BDK_slash,  BDK_r,          0,      0,      0,      0x279, 	/* LATIN SMALL LETTER TURNED R */
  BDK_slash,  BDK_v,          0,      0,      0,      0x28C, 	/* LATIN SMALL LETTER TURNED V */
  BDK_slash,  BDK_w,          0,      0,      0,      0x28D, 	/* LATIN SMALL LETTER TURNED W */
  BDK_slash,  BDK_y,          0,      0,      0,      0x28E, 	/* LATIN SMALL LETTER TRUEND Y*/
  BDK_3,      0,              0,      0,      0,      0x292, 	/* LATIN SMALL LETTER EZH */
  BDK_colon,  0,              0,      0,      0,      0x2D0, 	/* MODIFIER LETTER TRIANGULAR COLON */
  BDK_A,      0,              0,      0,      0,      0x251, 	/* LATIN SMALL LETTER ALPHA */
  BDK_E,      0,              0,      0,      0,      0x25B, 	/* LATIN SMALL LETTER OPEN E */
  BDK_I,      0,              0,      0,      0,      0x26A, 	/* LATIN LETTER SMALL CAPITAL I */
  BDK_L,      0,              0,      0,      0,      0x29F, 	/* LATIN LETTER SMALL CAPITAL L */
  BDK_M,      0,              0,      0,      0,      0x28D, 	/* LATIN SMALL LETTER TURNED W */
  BDK_O,      0,              0,      0,      0,      0x04F, 	/* LATIN LETTER SMALL CAPITAL OE */
  BDK_O,      BDK_E,          0,      0,      0,      0x276, 	/* LATIN LETTER SMALL CAPITAL OE */
  BDK_R,      0,              0,      0,      0,      0x280, 	/* LATIN LETTER SMALL CAPITAL R */
  BDK_U,      0,              0,      0,      0,      0x28A, 	/* LATIN SMALL LETTER UPSILON */
  BDK_Y,      0,              0,      0,      0,      0x28F, 	/* LATIN LETTER SMALL CAPITAL Y */
  BDK_grave,  0,              0,      0,      0,      0x2CC, 	/* MODIFIER LETTER LOW VERTICAL LINE */
  BDK_a,      0,              0,      0,      0,      0x061, 	/* LATIN SMALL LETTER A */
  BDK_a,      BDK_e,          0,      0,      0,      0x0E6, 	/* LATIN SMALL LETTER AE */
  BDK_c,      0,              0,      0,      0,      0x063,    /* LATIN SMALL LETTER C */
  BDK_c,      BDK_comma,      0,      0,      0,      0x0E7,    /* LATIN SMALL LETTER C WITH CEDILLA */
  BDK_d,      0,              0,      0,      0,      0x064, 	/* LATIN SMALL LETTER E */
  BDK_d,      BDK_apostrophe, 0,      0,      0,      0x064, 	/* LATIN SMALL LETTER D */
  BDK_d,      BDK_h,          0,      0,      0,      0x0F0, 	/* LATIN SMALL LETTER ETH */
  BDK_e,      0,              0,      0,      0,      0x065, 	/* LATIN SMALL LETTER E */
  BDK_e,      BDK_minus,      0,      0,      0,      0x25A, 	/* LATIN SMALL LETTER SCHWA WITH HOOK */
  BDK_e,      BDK_bar,        0,      0,      0,      0x25A, 	/* LATIN SMALL LETTER SCHWA WITH HOOK */
  BDK_g,      0,              0,      0,      0,      0x067, 	/* LATIN SMALL LETTER G */
  BDK_g,      BDK_n,          0,      0,      0,      0x272, 	/* LATIN SMALL LETTER N WITH LEFT HOOK */
  BDK_i,      0,              0,      0,      0,      0x069, 	/* LATIN SMALL LETTER I */
  BDK_i,      BDK_minus,      0,      0,      0,      0x268, 	/* LATIN SMALL LETTER I WITH STROKE */
  BDK_n,      0,              0,      0,      0,      0x06e, 	/* LATIN SMALL LETTER N */
  BDK_n,      BDK_g,          0,      0,      0,      0x14B, 	/* LATIN SMALL LETTER ENG */
  BDK_o,      0,              0,      0,      0,      0x06f, 	/* LATIN SMALL LETTER O */
  BDK_o,      BDK_minus,      0,      0,      0,      0x275, 	/* LATIN LETTER BARRED O */
  BDK_o,      BDK_slash,      0,      0,      0,      0x0F8, 	/* LATIN SMALL LETTER O WITH STROKE */
  BDK_o,      BDK_e,          0,      0,      0,      0x153, 	/* LATIN SMALL LIGATURE OE */
  BDK_o,      BDK_bar,        0,      0,      0,      0x251, 	/* LATIN SMALL LETTER ALPHA */
  BDK_s,      0,              0,      0,      0,      0x073, 	/* LATIN SMALL LETTER_ESH */
  BDK_s,      BDK_h,          0,      0,      0,      0x283, 	/* LATIN SMALL LETTER_ESH */
  BDK_t,      0,              0,      0,      0,      0x074, 	/* LATIN SMALL LETTER T */
  BDK_t,      BDK_h,          0,      0,      0,      0x3B8, 	/* GREEK SMALL LETTER THETA */
  BDK_u,      0,              0,      0,      0,      0x075, 	/* LATIN SMALL LETTER U */
  BDK_u,      BDK_minus,      0,      0,      0,      0x289, 	/* LATIN LETTER U BAR */
  BDK_z,      0,              0,      0,      0,      0x07A, 	/* LATIN SMALL LETTER Z */
  BDK_z,      BDK_h,          0,      0,      0,      0x292, 	/* LATIN SMALL LETTER EZH */
  BDK_bar,    BDK_o,          0,      0,      0,      0x252, 	/* LATIN LETTER TURNED ALPHA */

  BDK_asciitilde, 0,          0,      0,      0,      0x303,    /* COMBINING TILDE */

};

static void
ipa_class_init (BtkIMContextSimpleClass *class)
{
}

static void
ipa_init (BtkIMContextSimple *im_context)
{
  btk_im_context_simple_add_table (im_context,
				   ipa_compose_seqs,
				   4,
				   G_N_ELEMENTS (ipa_compose_seqs) / (4 + 2));
}

static const BtkIMContextInfo ipa_info = { 
  "ipa",		   /* ID */
  N_("IPA"),                       /* Human readable name */
  GETTEXT_PACKAGE,		   /* Translation domain */
   BTK_LOCALEDIR,		   /* Dir for bindtextdomain (not strictly needed for "btk+") */
  ""			           /* Languages for which this module is the default */
};

static const BtkIMContextInfo *info_list[] = {
  &ipa_info
};

#ifndef INCLUDE_IM_ipa
#define MODULE_ENTRY(type, function) G_MODULE_EXPORT type im_module_ ## function
#else
#define MODULE_ENTRY(type, function) type _btk_immodule_ipa_ ## function
#endif

MODULE_ENTRY (void, init) (GTypeModule *module)
{
  ipa_register_type (module);
}

MODULE_ENTRY (void, exit) (void)
{
}

MODULE_ENTRY (void, list) (const BtkIMContextInfo ***contexts,
			   int                      *n_contexts)
{
  *contexts = info_list;
  *n_contexts = G_N_ELEMENTS (info_list);
}

MODULE_ENTRY (BtkIMContext *, create) (const bchar *context_id)
{
  if (strcmp (context_id, "ipa") == 0)
    return g_object_new (type_ipa, NULL);
  else
    return NULL;
}
