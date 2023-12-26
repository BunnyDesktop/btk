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

#undef BTK_DISABLE_DEPRECATED

#include "btkorientable.h"
#include "btkvruler.h"
#include "btkalias.h"

/**
 * SECTION:btkvruler
 * @Short_description: A vertical ruler
 * @Title: BtkVRuler
 *
 * <note>
 *  This widget is considered too specialized/little-used for
 *  BTK+, and will be removed in BTK 3.  If your application needs this widget,
 *  feel free to use it, as the widget is useful in some applications; it's just
 *  not of general interest. However, we are not accepting new features for the
 *  widget, and it will move out of the BTK+ distribution.
 * </note>
 *
 * The VRuler widget is a widget arranged vertically creating a ruler that is
 * utilized around other widgets such as a text widget. The ruler is used to show
 * the location of the mouse on the window and to show the size of the window in
 * specified units. The available units of measurement are BTK_PIXELS, BTK_INCHES
 * and BTK_CENTIMETERS. BTK_PIXELS is the default unit of measurement.
 */

G_DEFINE_TYPE (BtkVRuler, btk_vruler, BTK_TYPE_RULER)

static void
btk_vruler_class_init (BtkVRulerClass *klass)
{
}

static void
btk_vruler_init (BtkVRuler *vruler)
{
  btk_orientable_set_orientation (BTK_ORIENTABLE (vruler),
                                  BTK_ORIENTATION_VERTICAL);
}

/**
 * btk_vruler_new:
 *
 * Creates a new vertical ruler
 *
 * Returns: a new #BtkVRuler.
 *
 * @Deprecated: 2.24: #BtkRuler has been removed from BTK 3 for being
 *              unmaintained and too specialized. There is no replacement.
 */
BtkWidget *
btk_vruler_new (void)
{
  return g_object_new (BTK_TYPE_VRULER, NULL);
}

#define __BTK_VRULER_C__
#include "btkaliasdef.c"
