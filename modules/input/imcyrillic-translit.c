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

GType type_cyrillic_translit = 0;

static void cyrillic_translit_class_init (BtkIMContextSimpleClass *class);
static void cyrillic_translit_init (BtkIMContextSimple *im_context);

static void
cyrillic_translit_register_type (GTypeModule *module)
{
  const GTypeInfo object_info =
  {
    sizeof (BtkIMContextSimpleClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) cyrillic_translit_class_init,
    NULL,           /* class_finalize */
    NULL,           /* class_data */
    sizeof (BtkIMContextSimple),
    0,
    (GInstanceInitFunc) cyrillic_translit_init,
  };

  type_cyrillic_translit = 
    g_type_module_register_type (module,
				 BTK_TYPE_IM_CONTEXT_SIMPLE,
				 "BtkIMContextCyrillicTranslit",
				 &object_info, 0);
}

/* The sequences here match the sequences used in the emacs quail
 * mode cryllic-translit; they allow entering all characters
 * in iso-8859-5
 */
static guint16 cyrillic_compose_seqs[] = {
  BDK_apostrophe,    0,      0,      0,      0,      0x44C, 	/* CYRILLIC SMALL LETTER SOFT SIGN */
  BDK_apostrophe,    BDK_apostrophe,      0,      0,      0,      0x42C, 	/* CYRILLIC CAPITAL LETTER SOFT SIGN */
  BDK_slash,    BDK_C,  BDK_H,      0,      0,      0x040B, /* CYRILLIC CAPITAL LETTER TSHE */
  BDK_slash,    BDK_C,  BDK_h,      0,      0,      0x040B, /* CYRILLIC CAPITAL LETTER TSHE */
  BDK_slash,    BDK_D,  0,      0,      0,      0x0402, /* CYRILLIC CAPITAL LETTER DJE */
  BDK_slash,    BDK_E,  0,      0,      0,      0x0404, /* CYRILLIC CAPITAL LETTER UKRAINIAN IE */
  BDK_slash,    BDK_G,  0,      0,      0,      0x0403, /* CYRILLIC CAPITAL LETTER GJE */
  BDK_slash,    BDK_I,  0,      0,      0,      0x0406, /* CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I */
  BDK_slash,    BDK_J,  0,      0,      0,      0x0408, /* CYRILLIC CAPITAL LETTER JE */
  BDK_slash,    BDK_K,  0,      0,      0,      0x040C, /* CYRILLIC CAPITAL LETTER KJE */
  BDK_slash,    BDK_L,  0,      0,      0,      0x0409, /* CYRILLIC CAPITAL LETTER LJE */
  BDK_slash,    BDK_N,  0,      0,      0,      0x040A, /* CYRILLIC CAPITAL LETTER NJE */
  BDK_slash,    BDK_S,  0,      0,      0,      0x0405, /* CYRILLIC CAPITAL LETTER DZE */
  BDK_slash,    BDK_S,  BDK_H,  BDK_T,  0,      0x0429, /* CYRILLIC CAPITAL LETTER SHCH */
  BDK_slash,    BDK_S,  BDK_H,  BDK_t,  0,      0x0429, /* CYRILLIC CAPITAL LETTER SHCH */
  BDK_slash,    BDK_S,  BDK_h,  BDK_t,  0,      0x0429, /* CYRILLIC CAPITAL LETTER SHCH */
  BDK_slash,    BDK_T,  0,      0,      0,      0x0429, /* CYRILLIC CAPITAL LETTER SHCH */
  BDK_slash,    BDK_Z,  0,      0,      0,      0x040F, /* CYRILLIC CAPITAL LETTER DZHE */
  BDK_slash,    BDK_c,  BDK_h,      0,      0,      0x045B, /* CYRILLIC SMALL LETTER TSHE */
  BDK_slash,    BDK_d,  0,      0,      0,      0x0452, /* CYRILLIC SMALL LETTER DJE */
  BDK_slash,    BDK_e,  0,      0,      0,      0x0454, /* CYRILLIC SMALL LETTER UKRAINIAN IE */
  BDK_slash,    BDK_g,  0,      0,      0,      0x0453, /* CYRILLIC SMALL LETTER GJE */
  BDK_slash,    BDK_i,  0,      0,      0,      0x0456, /* CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I */
  BDK_slash,    BDK_j,  0,      0,      0,      0x0458, /* CYRILLIC SMALL LETTER JE */
  BDK_slash,    BDK_k,  0,      0,      0,      0x045C, /* CYRILLIC SMALL LETTER KJE */
  BDK_slash,    BDK_l,  0,      0,      0,      0x0459, /* CYRILLIC SMALL LETTER LJE */
  BDK_slash,    BDK_n,  0,      0,      0,      0x045A, /* CYRILLIC SMALL LETTER NJE */
  BDK_slash,    BDK_s,  0,      0,      0,      0x0455, /* CYRILLIC SMALL LETTER DZE */
  BDK_slash,    BDK_s,  BDK_h,  BDK_t,  0,      0x0449, /* CYRILLIC SMALL LETTER SHCH */
  BDK_slash,    BDK_t,  0,      0,      0,      0x0449, /* CYRILLIC SMALL LETTER SHCH */
  BDK_slash,    BDK_z,  0,      0,      0,      0x045F, /* CYRILLIC SMALL LETTER DZHE */
  BDK_A,	0,	0,	0,	0,	0x0410,	/* CYRILLIC CAPITAL LETTER A */
  BDK_B,	0,	0,	0,	0,	0x0411,	/* CYRILLIC CAPITAL LETTER BE */
  BDK_C,	0,	0,	0,	0,	0x0426,	/* CYRILLIC CAPITAL LETTER TSE */
  BDK_C,	BDK_H,	0,	0,	0,	0x0427,	/* CYRILLIC CAPITAL LETTER CHE */
  BDK_C,	BDK_h,	0,	0,	0,	0x0427,	/* CYRILLIC CAPITAL LETTER CHE */
  BDK_D,	0,	0,	0,	0,	0x0414,	/* CYRILLIC CAPITAL LETTER DE */
  BDK_E,	0,	0,	0,	0,	0x0415,	/* CYRILLIC CAPITAL LETTER IE */
  BDK_E,	BDK_apostrophe,	0,	0,	0,	0x042D,	/* CYRILLIC CAPITAL LETTER E */
  BDK_F,	0,	0,	0,	0,	0x0424,	/* CYRILLIC CAPITAL LETTER EF */
  BDK_G,	0,	0,	0,	0,	0x0413,	/* CYRILLIC CAPITAL LETTER GE */
  BDK_H,	0,	0,	0,	0,	0x0425,	/* CYRILLIC CAPITAL LETTER HA */
  BDK_I,	0,	0,	0,	0,	0x0418,	/* CYRILLIC CAPITAL LETTER I */
  BDK_J,	0,	0,	0,	0,	0x0419,	/* CYRILLIC CAPITAL LETTER SHORT I */
  BDK_J,	BDK_apostrophe,	0,	0,	0,	0x0419,	/* CYRILLIC CAPITAL LETTER SHORT I */
  BDK_J,	BDK_A,	0,	0,	0,	0x042F,	/* CYRILLIC CAPITAL LETTER YA */
  BDK_J,	BDK_I,	0,	0,	0,	0x0407,	/* CYRILLIC CAPITAL LETTER YI */
  BDK_J,	BDK_O,	0,	0,	0,	0x0401,	/* CYRILLIC CAPITAL LETTER YO */
  BDK_J,	BDK_U,	0,	0,	0,	0x042E,	/* CYRILLIC CAPITAL LETTER YA */
  BDK_J,	BDK_a,	0,	0,	0,	0x042F,	/* CYRILLIC CAPITAL LETTER YA */
  BDK_J,	BDK_i,	0,	0,	0,	0x0407,	/* CYRILLIC CAPITAL LETTER YI */
  BDK_J,	BDK_o,	0,	0,	0,	0x0401,	/* CYRILLIC CAPITAL LETTER YO */
  BDK_J,	BDK_u,	0,	0,	0,	0x042E,	/* CYRILLIC CAPITAL LETTER YA */
  BDK_K,	0,	0,	0,	0,	0x041A,	/* CYRILLIC CAPITAL LETTER KA */
  BDK_K,	BDK_H,	0,	0,	0,	0x0425,	/* CYRILLIC CAPITAL LETTER HA */
  BDK_L,	0,	0,	0,	0,	0x041B,	/* CYRILLIC CAPITAL LETTER EL */
  BDK_M,	0,	0,	0,	0,	0x041C,	/* CYRILLIC CAPITAL LETTER EM */
  BDK_N,	0,	0,	0,	0,	0x041D,	/* CYRILLIC CAPITAL LETTER EN */
  BDK_O,	0,	0,	0,	0,	0x041E,	/* CYRILLIC CAPITAL LETTER O */
  BDK_P,	0,	0,	0,	0,	0x041F,	/* CYRILLIC CAPITAL LETTER PE */
  BDK_Q,	0,	0,	0,	0,	0x042F,	/* CYRILLIC CAPITAL LETTER YA */
  BDK_R,	0,	0,	0,	0,	0x0420,	/* CYRILLIC CAPITAL LETTER ER */
  BDK_S,	0,	0,	0,	0,	0x0421,	/* CYRILLIC CAPITAL LETTER ES */
  BDK_S,	BDK_H,	0,	0,	0,	0x0428,	/* CYRILLIC CAPITAL LETTER SHA */
  BDK_S,	BDK_H,	BDK_C,	BDK_H,	0, 	0x0429,	/* CYRILLIC CAPITAL LETTER SHCA */
  BDK_S,	BDK_H,	BDK_C,	BDK_h,	0,	0x0429,	/* CYRILLIC CAPITAL LETTER SHCA */
  BDK_S,	BDK_H,	BDK_c,	BDK_h,	0,	0x0429,	/* CYRILLIC CAPITAL LETTER SHCA */
  BDK_S,	BDK_H,	BDK_c,	BDK_h,	0,	0x0429,	/* CYRILLIC CAPITAL LETTER SHCA */
  BDK_S,	BDK_J,	0,	0,	0,	0x0429,	/* CYRILLIC CAPITAL LETTER SHCA */
  BDK_S,	BDK_h,	0,	0,	0,	0x0428,	/* CYRILLIC CAPITAL LETTER SHA */
  BDK_S,	BDK_j,	0,	0,	0,	0x0429,	/* CYRILLIC CAPITAL LETTER SHCA */
  BDK_T,	0,	0,	0,	0,	0x0422,	/* CYRILLIC CAPITAL LETTER TE */
  BDK_U,	0,	0,	0,	0,	0x0423,	/* CYRILLIC CAPITAL LETTER U */
  BDK_U,	BDK_apostrophe,	0,	0,	0,	0x040E,	/* CYRILLIC CAPITAL LETTER SHORT */
  BDK_V,	0,	0,	0,	0,	0x0412,	/* CYRILLIC CAPITAL LETTER VE */
  BDK_W,	0,	0,	0,	0,	0x0412,	/* CYRILLIC CAPITAL LETTER VE */
  BDK_X,	0,	0,	0,	0,	0x0425,	/* CYRILLIC CAPITAL LETTER HA */
  BDK_Y,	0,	0,	0,	0,	0x042B,	/* CYRILLIC CAPITAL LETTER YERU */
  BDK_Y,	BDK_A,	0,	0,	0,	0x042F,	/* CYRILLIC CAPITAL LETTER YA */
  BDK_Y,	BDK_I,	0,	0,	0,	0x0407,	/* CYRILLIC CAPITAL LETTER YI */
  BDK_Y,	BDK_O,	0,	0,	0,	0x0401,	/* CYRILLIC CAPITAL LETTER YO */
  BDK_Y,	BDK_U,	0,	0,	0,	0x042E,	/* CYRILLIC CAPITAL LETTER YU */
  BDK_Y,	BDK_a,	0,	0,	0,	0x042F,	/* CYRILLIC CAPITAL LETTER YA */
  BDK_Y,	BDK_i,	0,	0,	0,	0x0407,	/* CYRILLIC CAPITAL LETTER YI */
  BDK_Y,	BDK_o,	0,	0,	0,	0x0401,	/* CYRILLIC CAPITAL LETTER YO */
  BDK_Y,	BDK_u,	0,	0,	0,	0x042E,	/* CYRILLIC CAPITAL LETTER YU */
  BDK_Z,	0,	0,	0,	0,	0x0417,	/* CYRILLIC CAPITAL LETTER ZE */
  BDK_Z,	BDK_H,	0,	0,	0,	0x0416,	/* CYRILLIC CAPITAL LETTER ZHE */
  BDK_Z,	BDK_h,	0,	0,	0,	0x0416,	/* CYRILLIC CAPITAL LETTER ZHE */
  BDK_a,	0,	0,	0,	0,	0x0430,	/* CYRILLIC SMALL LETTER A */
  BDK_b,	0,	0,	0,	0,	0x0431,	/* CYRILLIC SMALL LETTER BE */
  BDK_c,	0,	0,	0,	0,	0x0446,	/* CYRILLIC SMALL LETTER TSE */
  BDK_c,	BDK_h,	0,	0,	0,	0x0447,	/* CYRILLIC SMALL LETTER CHE */
  BDK_d,	0,	0,	0,	0,	0x0434,	/* CYRILLIC SMALL LETTER DE */
  BDK_e,	0,	0,	0,	0,	0x0435,	/* CYRILLIC SMALL LETTER IE */
  BDK_e,	BDK_apostrophe,	0,	0,	0,	0x044D,	/* CYRILLIC SMALL LETTER E */
  BDK_f,	0,	0,	0,	0,	0x0444,	/* CYRILLIC SMALL LETTER EF */
  BDK_g,	0,	0,	0,	0,	0x0433,	/* CYRILLIC SMALL LETTER GE */
  BDK_h,	0,	0,	0,	0,	0x0445,	/* CYRILLIC SMALL LETTER HA */
  BDK_i,	0,	0,	0,	0,	0x0438,	/* CYRILLIC SMALL LETTER I */
  BDK_j,	0,	0,	0,	0,	0x0439,	/* CYRILLIC SMALL LETTER SHORT I */
  BDK_j,	BDK_apostrophe,	0,	0,	0,	0x0439,	/* CYRILLIC SMALL LETTER SHORT I */
  BDK_j,	BDK_a,	0,	0,	0,	0x044F,	/* CYRILLIC SMALL LETTER YU */
  BDK_j,	BDK_i,	0,	0,	0,	0x0457,	/* CYRILLIC SMALL LETTER YI */
  BDK_j,	BDK_o,	0,	0,	0,	0x0451,	/* CYRILLIC SMALL LETTER YO */
  BDK_j,	BDK_u,	0,	0,	0,	0x044E,	/* CYRILLIC SMALL LETTER YA */
  BDK_k,	0,	0,	0,	0,	0x043A,	/* CYRILLIC SMALL LETTER KA */
  BDK_k,	BDK_h,	0,	0,	0,	0x0445,	/* CYRILLIC SMALL LETTER HA */
  BDK_l,	0,	0,	0,	0,	0x043B,	/* CYRILLIC SMALL LETTER EL */
  BDK_m,	0,	0,	0,	0,	0x043C,	/* CYRILLIC SMALL LETTER EM */
  BDK_n,	0,	0,	0,	0,	0x043D,	/* CYRILLIC SMALL LETTER EN */
  BDK_o,	0,	0,	0,	0,	0x043E,	/* CYRILLIC SMALL LETTER O */
  BDK_p,	0,	0,	0,	0,	0x043F,	/* CYRILLIC SMALL LETTER PE */
  BDK_q,	0,	0,	0,	0,	0x044F,	/* CYRILLIC SMALL LETTER YA */
  BDK_r,	0,	0,	0,	0,	0x0440,	/* CYRILLIC SMALL LETTER ER */
  BDK_s,	0,	0,	0,	0,	0x0441,	/* CYRILLIC SMALL LETTER ES */
  BDK_s,	BDK_h,	0,	0,	0,	0x0448,	/* CYRILLIC SMALL LETTER SHA */
  BDK_s,	BDK_h,	BDK_c,	BDK_h,	0, 	0x0449,	/* CYRILLIC SMALL LETTER SHCA */
  BDK_s,	BDK_j,	0,	0,	0,	0x0449,	/* CYRILLIC SMALL LETTER SHCA */
  BDK_t,	0,	0,	0,	0,	0x0442,	/* CYRILLIC SMALL LETTER TE */
  BDK_u,	0,	0,	0,	0,	0x0443,	/* CYRILLIC SMALL LETTER U */
  BDK_u,	BDK_apostrophe,	0,	0,	0,	0x045E,	/* CYRILLIC SMALL LETTER SHORT */
  BDK_v,	0,	0,	0,	0,	0x0432,	/* CYRILLIC SMALL LETTER VE */
  BDK_w,	0,	0,	0,	0,	0x0432,	/* CYRILLIC SMALL LETTER VE */
  BDK_x,	0,	0,	0,	0,	0x0445,	/* CYRILLIC SMALL LETTER HA */
  BDK_y,	0,	0,	0,	0,	0x044B,	/* CYRILLIC SMALL LETTER YERU */
  BDK_y,	BDK_a,	0,	0,	0,	0x044F,	/* CYRILLIC SMALL LETTER YU */
  BDK_y,	BDK_i,	0,	0,	0,	0x0457,	/* CYRILLIC SMALL LETTER YI */
  BDK_y,	BDK_o,	0,	0,	0,	0x0451,	/* CYRILLIC SMALL LETTER YO */
  BDK_y,	BDK_u,	0,	0,	0,	0x044E,	/* CYRILLIC SMALL LETTER YA */
  BDK_z,	0,	0,	0,	0,	0x0437,	/* CYRILLIC SMALL LETTER ZE */
  BDK_z,	BDK_h,	0,	0,	0,	0x0436,	/* CYRILLIC SMALL LETTER ZHE */
  BDK_asciitilde,    0,      0,      0,      0,      0x44A, 	/* CYRILLIC SMALL LETTER HARD SIGN */
  BDK_asciitilde,    BDK_asciitilde,      0,      0,      0,      0x42A, 	/* CYRILLIC CAPITAL LETTER HARD SIGN */
};

static void
cyrillic_translit_class_init (BtkIMContextSimpleClass *class)
{
}

static void
cyrillic_translit_init (BtkIMContextSimple *im_context)
{
  btk_im_context_simple_add_table (im_context,
				   cyrillic_compose_seqs,
				   4,
				   G_N_ELEMENTS (cyrillic_compose_seqs) / (4 + 2));
}

static const BtkIMContextInfo cyrillic_translit_info = { 
  "cyrillic_translit",		   /* ID */
  N_("Cyrillic (Transliterated)"), /* Human readable name */
  GETTEXT_PACKAGE,		   /* Translation domain */
   BTK_LOCALEDIR,		   /* Dir for bindtextdomain (not strictly needed for "btk+") */
  ""			           /* Languages for which this module is the default */
};

static const BtkIMContextInfo *info_list[] = {
  &cyrillic_translit_info
};

#ifndef INCLUDE_IM_cyrillic_translit
#define MODULE_ENTRY(type, function) G_MODULE_EXPORT type im_module_ ## function
#else
#define MODULE_ENTRY(type, function) type _btk_immodule_cyrillic_translit_ ## function
#endif

MODULE_ENTRY (void, init) (GTypeModule *module)
{
  cyrillic_translit_register_type (module);
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

MODULE_ENTRY (BtkIMContext *, create) (const gchar *context_id)
{
  if (strcmp (context_id, "cyrillic_translit") == 0)
    return g_object_new (type_cyrillic_translit, NULL);
  else
    return NULL;
}
