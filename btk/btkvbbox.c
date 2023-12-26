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
#include "btkvbbox.h"
#include "btkorientable.h"
#include "btkintl.h"
#include "btkalias.h"

/**
 * SECTION:btkvbbox
 * @Short_description: A container for arranging buttons vertically
 * @Title: BtkVButtonBox
 * @See_also: #BtkBox, #BtkButtonBox, #BtkHButtonBox
 *
 * A button box should be used to provide a consistent layout of buttons
 * throughout your application. The layout/spacing can be altered by the
 * programmer, or if desired, by the user to alter the 'feel' of a
 * program to a small degree.
 *
 * A #BtkVButtonBox is created with btk_vbutton_box_new(). Buttons are
 * packed into a button box the same way widgets are added to any other
 * container, using btk_container_add(). You can also use
 * btk_box_pack_start() or btk_box_pack_end(), but for button boxes both
 * these functions work just like btk_container_add(), ie., they pack the
 * button in a way that depends on the current layout style and on
 * whether the button has had btk_button_box_set_child_secondary() called
 * on it.
 *
 * The spacing between buttons can be set with btk_box_set_spacing(). The
 * arrangement and layout of the buttons can be changed with
 * btk_button_box_set_layout().
 */

static bint default_spacing = 10;
static BtkButtonBoxStyle default_layout_style = BTK_BUTTONBOX_EDGE;

G_DEFINE_TYPE (BtkVButtonBox, btk_vbutton_box, BTK_TYPE_BUTTON_BOX)

static void
btk_vbutton_box_class_init (BtkVButtonBoxClass *class)
{
}

static void
btk_vbutton_box_init (BtkVButtonBox *vbutton_box)
{
  btk_orientable_set_orientation (BTK_ORIENTABLE (vbutton_box),
                                  BTK_ORIENTATION_VERTICAL);
}

/**
 * btk_vbutton_box_new:
 *
 * Creates a new vertical button box.
 *
 * Returns: a new button box #BtkWidget.
 */
BtkWidget *
btk_vbutton_box_new (void)
{
  return g_object_new (BTK_TYPE_VBUTTON_BOX, NULL);
}

/**
 * btk_vbutton_box_set_spacing_default:
 * @spacing: an integer value.
 *
 * Changes the default spacing that is placed between widgets in an
 * vertical button box.
 *
 * Deprecated: 2.0: Use btk_box_set_spacing() instead.
 */
void
btk_vbutton_box_set_spacing_default (bint spacing)
{
  default_spacing = spacing;
}

/**
 * btk_vbutton_box_set_layout_default:
 * @layout: a new #BtkButtonBoxStyle.
 *
 * Sets a new layout mode that will be used by all button boxes.
 *
 * Deprecated: 2.0: Use btk_button_box_set_layout() instead.
 */
void
btk_vbutton_box_set_layout_default (BtkButtonBoxStyle layout)
{
  g_return_if_fail (layout >= BTK_BUTTONBOX_DEFAULT_STYLE &&
		    layout <= BTK_BUTTONBOX_CENTER);

  default_layout_style = layout;
}

/**
 * btk_vbutton_box_get_spacing_default:
 *
 * Retrieves the current default spacing for vertical button boxes. This is the number of pixels
 * to be placed between the buttons when they are arranged.
 *
 * Returns: the default number of pixels between buttons.
 *
 * Deprecated: 2.0: Use btk_box_get_spacing() instead.
 */
bint
btk_vbutton_box_get_spacing_default (void)
{
  return default_spacing;
}

/**
 * btk_vbutton_box_get_layout_default:
 *
 * Retrieves the current layout used to arrange buttons in button box widgets.
 *
 * Returns: the current #BtkButtonBoxStyle.
 *
 * Deprecated: 2.0: Use btk_button_box_get_layout() instead.
 */
BtkButtonBoxStyle
btk_vbutton_box_get_layout_default (void)
{
  return default_layout_style;
}

BtkButtonBoxStyle
_btk_vbutton_box_get_layout_default (void)
{
  return default_layout_style;
}


#define __BTK_VBBOX_C__
#include "btkaliasdef.c"
