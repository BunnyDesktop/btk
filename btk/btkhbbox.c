/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
#include "btkhbbox.h"
#include "btkorientable.h"
#include "btkintl.h"
#include "btkalias.h"


static bint default_spacing = 30;
static bint default_layout_style = BTK_BUTTONBOX_EDGE;

G_DEFINE_TYPE (BtkHButtonBox, btk_hbutton_box, BTK_TYPE_BUTTON_BOX)

static void
btk_hbutton_box_class_init (BtkHButtonBoxClass *class)
{
}

static void
btk_hbutton_box_init (BtkHButtonBox *hbutton_box)
{
  btk_orientable_set_orientation (BTK_ORIENTABLE (hbutton_box),
                                  BTK_ORIENTATION_HORIZONTAL);
}

BtkWidget *
btk_hbutton_box_new (void)
{
  return g_object_new (BTK_TYPE_HBUTTON_BOX, NULL);
}


/* set default value for spacing */

void
btk_hbutton_box_set_spacing_default (bint spacing)
{
  default_spacing = spacing;
}


/* set default value for layout style */

void
btk_hbutton_box_set_layout_default (BtkButtonBoxStyle layout)
{
  g_return_if_fail (layout >= BTK_BUTTONBOX_DEFAULT_STYLE &&
		    layout <= BTK_BUTTONBOX_CENTER);

  default_layout_style = layout;
}

/* get default value for spacing */

bint
btk_hbutton_box_get_spacing_default (void)
{
  return default_spacing;
}


/* get default value for layout style */

BtkButtonBoxStyle
btk_hbutton_box_get_layout_default (void)
{
  return default_layout_style;
}

BtkButtonBoxStyle
_btk_hbutton_box_get_layout_default (void)
{
  return default_layout_style;
}

#define __BTK_HBUTTON_BOX_C__
#include "btkaliasdef.c"
