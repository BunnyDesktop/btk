/* BTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat Software
 * Copyright (C) 2000 SuSE Linux Ltd
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
 * Original author: Owen Taylor <otaylor@redhat.com>
 * 
 * Modified for VIQR - Robert Brady <robert@suse.co.uk>
 *
 */

#include "config.h"
#include <string.h>

#include "btk/btk.h"
#include "bdk/bdkkeysyms.h"

#include "btk/btkimmodule.h"
#include "btk/btkintl.h"

GType type_viqr_translit = 0;

static void viqr_class_init (BtkIMContextSimpleClass *class);
static void viqr_init (BtkIMContextSimple *im_context);

static void
viqr_register_type (GTypeModule *module)
{
  const GTypeInfo object_info =
  {
    sizeof (BtkIMContextSimpleClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) viqr_class_init,
    NULL,           /* class_finalize */
    NULL,           /* class_data */
    sizeof (BtkIMContextSimple),
    0,
    (GInstanceInitFunc) viqr_init,
  };

  type_viqr_translit = 
    g_type_module_register_type (module,
				 BTK_TYPE_IM_CONTEXT_SIMPLE,
				 "BtkIMContextViqr",
				 &object_info, 0);
}

static guint16 viqr_compose_seqs[] = {
  BDK_A,                   0,                0, 0, 0, 'A',
  BDK_A,                   BDK_apostrophe,   0, 0, 0, 0xc1,
  BDK_A,  BDK_parenleft,   0,                0, 0,    0x102,
  BDK_A,  BDK_parenleft,   BDK_apostrophe,   0, 0,    0x1eae,
  BDK_A,  BDK_parenleft,   BDK_period,       0, 0,    0x1eb6,
  BDK_A,  BDK_parenleft,   BDK_question,     0, 0,    0x1eb2,
  BDK_A,  BDK_parenleft,   BDK_grave,        0, 0,    0x1eb0,
  BDK_A,  BDK_parenleft,   BDK_asciitilde,   0, 0,    0x1eb4,
  BDK_A,                   BDK_period,       0, 0, 0, 0x1ea0,
  BDK_A,                   BDK_question,     0, 0, 0, 0x1ea2,
  BDK_A,  BDK_asciicircum, 0,                0, 0,    0xc2,
  BDK_A,  BDK_asciicircum, BDK_apostrophe,   0, 0,    0x1ea4,
  BDK_A,  BDK_asciicircum, BDK_period,       0, 0,    0x1eac,
  BDK_A,  BDK_asciicircum, BDK_question,     0, 0,    0x1ea8,
  BDK_A,  BDK_asciicircum, BDK_grave,        0, 0,    0x1ea6,
  BDK_A,  BDK_asciicircum, BDK_asciitilde,   0, 0,    0x1eaa,
  BDK_A,                   BDK_grave,        0, 0, 0, 0xc0,
  BDK_A,                   BDK_asciitilde,   0, 0, 0, 0xc3,
  BDK_D,                   0,                0, 0, 0, 'D',
  BDK_D,                   BDK_D,            0, 0, 0, 0x110,
  BDK_D,                   BDK_d,            0, 0, 0, 0x110,
  BDK_E,                   0,                0, 0, 0, 'E',
  BDK_E,                   BDK_apostrophe,   0, 0, 0, 0xc9,
  BDK_E,                   BDK_period,       0, 0, 0, 0x1eb8,
  BDK_E,                   BDK_question,     0, 0, 0, 0x1eba,
  BDK_E,  BDK_asciicircum, 0,                0, 0,    0xca,
  BDK_E,  BDK_asciicircum, BDK_apostrophe,   0, 0,    0x1ebe,
  BDK_E,  BDK_asciicircum, BDK_period,       0, 0,    0x1ec6,
  BDK_E,  BDK_asciicircum, BDK_question,     0, 0,    0x1ec2,
  BDK_E,  BDK_asciicircum, BDK_grave,        0, 0,    0x1ec0,
  BDK_E,  BDK_asciicircum, BDK_asciitilde,   0, 0,    0x1ec4,
  BDK_E,                   BDK_grave,        0, 0, 0, 0xc8,
  BDK_E,                   BDK_asciitilde,   0, 0, 0, 0x1ebc,
  BDK_I,                   0,                0, 0, 0, 'I',
  BDK_I,                   BDK_apostrophe,   0, 0, 0, 0xcd,
  BDK_I,                   BDK_period,       0, 0, 0, 0x1eca,
  BDK_I,                   BDK_question,     0, 0, 0, 0x1ec8,
  BDK_I,                   BDK_grave,        0, 0, 0, 0xcc,
  BDK_I,                   BDK_asciitilde,   0, 0, 0, 0x128,
  BDK_O,                   0,                0, 0, 0, 'O',
  BDK_O,                   BDK_apostrophe,   0, 0, 0, 0xD3,
  BDK_O,  BDK_plus,        0,                0, 0,    0x1a0,
  BDK_O,  BDK_plus,        BDK_apostrophe,   0, 0,    0x1eda,
  BDK_O,  BDK_plus,        BDK_period,       0, 0,    0x1ee2,
  BDK_O,  BDK_plus,        BDK_question,     0, 0,    0x1ede,
  BDK_O,  BDK_plus,        BDK_grave,        0, 0,    0x1edc,
  BDK_O,  BDK_plus,        BDK_asciitilde,   0, 0,    0x1ee0,
  BDK_O,                   BDK_period,       0, 0, 0, 0x1ecc,
  BDK_O,                   BDK_question,     0, 0, 0, 0x1ece,
  BDK_O,  BDK_asciicircum, 0,                0, 0,    0xd4,
  BDK_O,  BDK_asciicircum, BDK_apostrophe,   0, 0,    0x1ed0,
  BDK_O,  BDK_asciicircum, BDK_period,       0, 0,    0x1ed8,
  BDK_O,  BDK_asciicircum, BDK_question,     0, 0,    0x1ed4,
  BDK_O,  BDK_asciicircum, BDK_grave,        0, 0,    0x1ed2,
  BDK_O,  BDK_asciicircum, BDK_asciitilde,   0, 0,    0x1ed6,
  BDK_O,                   BDK_grave,        0, 0, 0, 0xD2,
  BDK_O,                   BDK_asciitilde,   0, 0, 0, 0xD5,
  BDK_U,                   0,                0, 0, 0, 'U',
  BDK_U,                   BDK_apostrophe,   0, 0, 0, 0xDA,
  BDK_U,  BDK_plus,        0,                0, 0,    0x1af,
  BDK_U,  BDK_plus,        BDK_apostrophe,   0, 0,    0x1ee8,
  BDK_U,  BDK_plus,        BDK_period,       0, 0,    0x1ef0,
  BDK_U,  BDK_plus,        BDK_question,     0, 0,    0x1eec,
  BDK_U,  BDK_plus,        BDK_grave,        0, 0,    0x1eea,
  BDK_U,  BDK_plus,        BDK_asciitilde,   0, 0,    0x1eee,
  BDK_U,                   BDK_period,       0, 0, 0, 0x1ee4,
  BDK_U,                   BDK_question,     0, 0, 0, 0x1ee6,
  BDK_U,                   BDK_grave,        0, 0, 0, 0xd9,
  BDK_U,                   BDK_asciitilde,   0, 0, 0, 0x168,
  BDK_Y,                   0,                0, 0, 0, 'Y',
  BDK_Y,                   BDK_apostrophe,   0, 0, 0, 0xdd,
  BDK_Y,                   BDK_period,       0, 0, 0, 0x1ef4,
  BDK_Y,                   BDK_question,     0, 0, 0, 0x1ef6,
  BDK_Y,                   BDK_grave,        0, 0, 0, 0x1ef2,
  BDK_Y,                   BDK_asciitilde,   0, 0, 0, 0x1ef8,
  /* Do we need anything else here? */
  BDK_backslash,           0,                0, 0, 0, 0,
  BDK_backslash,           BDK_apostrophe,   0, 0, 0, '\'',
  BDK_backslash,           BDK_parenleft,    0, 0, 0, '(',
  BDK_backslash,           BDK_plus,         0, 0, 0, '+',
  BDK_backslash,           BDK_period,       0, 0, 0, '.',
  BDK_backslash,           BDK_question,     0, 0, 0, '?',
  BDK_backslash,           BDK_D,            0, 0, 0, 'D',
  BDK_backslash,           BDK_backslash,    0, 0, 0, '\\',
  BDK_backslash,           BDK_asciicircum,  0, 0, 0, '^',
  BDK_backslash,           BDK_grave,        0, 0, 0, '`',
  BDK_backslash,           BDK_d,            0, 0, 0, 'd',
  BDK_backslash,           BDK_asciitilde,   0, 0, 0, '~',
  BDK_a,                   0,                0, 0, 0, 'a',
  BDK_a,                   BDK_apostrophe,   0, 0, 0, 0xe1,
  BDK_a, BDK_parenleft,    0,                0, 0,    0x103,
  BDK_a, BDK_parenleft,    BDK_apostrophe,   0, 0,    0x1eaf,
  BDK_a, BDK_parenleft,    BDK_period,       0, 0,    0x1eb7,
  BDK_a, BDK_parenleft,    BDK_question,     0, 0,    0x1eb3,
  BDK_a, BDK_parenleft,    BDK_grave,        0, 0,    0x1eb1,
  BDK_a, BDK_parenleft,    BDK_asciitilde,   0, 0,    0x1eb5,
  BDK_a,                   BDK_period,       0, 0, 0, 0x1ea1,
  BDK_a,                   BDK_question,     0, 0, 0, 0x1ea3,
  BDK_a, BDK_asciicircum,  0,                0, 0,    0xe2,
  BDK_a, BDK_asciicircum,  BDK_apostrophe,   0, 0,    0x1ea5,
  BDK_a, BDK_asciicircum,  BDK_period,       0, 0,    0x1ead,
  BDK_a, BDK_asciicircum,  BDK_question,     0, 0,    0x1ea9,
  BDK_a, BDK_asciicircum,  BDK_grave,        0, 0,    0x1ea7,
  BDK_a, BDK_asciicircum,  BDK_asciitilde,   0, 0,    0x1eab,
  BDK_a,                   BDK_grave,        0, 0, 0, 0xe0,
  BDK_a,                   BDK_asciitilde,   0, 0, 0, 0xe3,
  BDK_d,                   0,                0, 0, 0, 'd',
  BDK_d,                   BDK_d,            0, 0, 0, 0x111,
  BDK_e,                   0,                0, 0, 0, 'e',
  BDK_e,                   BDK_apostrophe,   0, 0, 0, 0xe9,
  BDK_e,                   BDK_period,       0, 0, 0, 0x1eb9,
  BDK_e,                   BDK_question,     0, 0, 0, 0x1ebb,
  BDK_e, BDK_asciicircum,  0,                0, 0,    0xea,
  BDK_e, BDK_asciicircum,  BDK_apostrophe,   0, 0,    0x1ebf,
  BDK_e, BDK_asciicircum,  BDK_period,       0, 0,    0x1ec7,
  BDK_e, BDK_asciicircum,  BDK_question,     0, 0,    0x1ec3,
  BDK_e, BDK_asciicircum,  BDK_grave,        0, 0,    0x1ec1,
  BDK_e, BDK_asciicircum,  BDK_asciitilde,   0, 0,    0x1ec5,
  BDK_e,                   BDK_grave,        0, 0, 0, 0xe8,
  BDK_e,                   BDK_asciitilde,   0, 0, 0, 0x1ebd,
  BDK_i,                   0,                0, 0, 0, 'i',
  BDK_i,                   BDK_apostrophe,   0, 0, 0, 0xed,
  BDK_i,                   BDK_period,       0, 0, 0, 0x1ecb,
  BDK_i,                   BDK_question,     0, 0, 0, 0x1ec9,
  BDK_i,                   BDK_grave,        0, 0, 0, 0xec,
  BDK_i,                   BDK_asciitilde,   0, 0, 0, 0x129,
  BDK_o,                   0,                0, 0, 0, 'o',
  BDK_o,                   BDK_apostrophe,   0, 0, 0, 0xF3,
  BDK_o,  BDK_plus,        0,                0, 0,    0x1a1,
  BDK_o,  BDK_plus,        BDK_apostrophe,   0, 0,    0x1edb,
  BDK_o,  BDK_plus,        BDK_period,       0, 0,    0x1ee3,
  BDK_o,  BDK_plus,        BDK_question,     0, 0,    0x1edf,
  BDK_o,  BDK_plus,        BDK_grave,        0, 0,    0x1edd,
  BDK_o,  BDK_plus,        BDK_asciitilde,   0, 0,    0x1ee1,
  BDK_o,                   BDK_period,       0, 0, 0, 0x1ecd,
  BDK_o,                   BDK_question,     0, 0, 0, 0x1ecf,
  BDK_o,  BDK_asciicircum, 0,                0, 0,    0xf4,
  BDK_o,  BDK_asciicircum, BDK_apostrophe,   0, 0,    0x1ed1,
  BDK_o,  BDK_asciicircum, BDK_period,       0, 0,    0x1ed9,
  BDK_o,  BDK_asciicircum, BDK_question,     0, 0,    0x1ed5,
  BDK_o,  BDK_asciicircum, BDK_grave,        0, 0,    0x1ed3,
  BDK_o,  BDK_asciicircum, BDK_asciitilde,   0, 0,    0x1ed7,
  BDK_o,                   BDK_grave,        0, 0, 0, 0xF2,
  BDK_o,                   BDK_asciitilde,   0, 0, 0, 0xF5,
  BDK_u,                   0,                0, 0, 0, 'u',
  BDK_u,                   BDK_apostrophe,   0, 0, 0, 0xFA,
  BDK_u,  BDK_plus,        0,                0, 0,    0x1b0,
  BDK_u,  BDK_plus,        BDK_apostrophe,   0, 0,    0x1ee9,
  BDK_u,  BDK_plus,        BDK_period,       0, 0,    0x1ef1,
  BDK_u,  BDK_plus,        BDK_question,     0, 0,    0x1eed,
  BDK_u,  BDK_plus,        BDK_grave,        0, 0,    0x1eeb,
  BDK_u,  BDK_plus,        BDK_asciitilde,   0, 0,    0x1eef,
  BDK_u,                   BDK_period,       0, 0, 0, 0x1ee5,
  BDK_u,                   BDK_question,     0, 0, 0, 0x1ee7,
  BDK_u,                   BDK_grave,        0, 0, 0, 0xf9,
  BDK_u,                   BDK_asciitilde,   0, 0, 0, 0x169,
  BDK_y,                   0,                0, 0, 0, 'y',
  BDK_y,                   BDK_apostrophe,   0, 0, 0, 0xfd,
  BDK_y,                   BDK_period,       0, 0, 0, 0x1ef5,
  BDK_y,                   BDK_question,     0, 0, 0, 0x1ef7,
  BDK_y,                   BDK_grave,        0, 0, 0, 0x1ef3,
  BDK_y,                   BDK_asciitilde,   0, 0, 0, 0x1ef9,
};

static void
viqr_class_init (BtkIMContextSimpleClass *class)
{
}

static void
viqr_init (BtkIMContextSimple *im_context)
{
  btk_im_context_simple_add_table (im_context,
				   viqr_compose_seqs,
				   4,
				   G_N_ELEMENTS (viqr_compose_seqs) / (4 + 2));
}

static const BtkIMContextInfo viqr_info = { 
  "viqr",		   /* ID */
  N_("Vietnamese (VIQR)"), /* Human readable name */
  GETTEXT_PACKAGE,	   /* Translation domain */
   BTK_LOCALEDIR,	   /* Dir for bindtextdomain (not strictly needed for "btk+") */
  ""			   /* Languages for which this module is the default */
};

static const BtkIMContextInfo *info_list[] = {
  &viqr_info
};

#ifndef INCLUDE_IM_viqr
#define MODULE_ENTRY(type, function) G_MODULE_EXPORT type im_module_ ## function
#else
#define MODULE_ENTRY(type, function) type _btk_immodule_viqr_ ## function
#endif

MODULE_ENTRY (void, init) (GTypeModule *module)
{
  viqr_register_type (module);
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
  if (strcmp (context_id, "viqr") == 0)
    return g_object_new (type_viqr_translit, NULL);
  else
    return NULL;
}
