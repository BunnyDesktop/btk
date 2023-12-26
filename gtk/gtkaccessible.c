/* BTK - The GIMP Toolkit
 * Copyright 2001 Sun Microsystems Inc.
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

#include "config.h"
#include <string.h>

#include "btkwidget.h"
#include "btkintl.h"
#include "btkaccessible.h"
#include "btkalias.h"

/**
 * SECTION:btkaccessible
 * @Short_description: Accessibility support for widgets
 * @Title: BtkAccessible
 */


static void btk_accessible_real_connect_widget_destroyed (BtkAccessible *accessible);

G_DEFINE_TYPE (BtkAccessible, btk_accessible, BATK_TYPE_OBJECT)

static void
btk_accessible_init (BtkAccessible *object)
{
}

static void
btk_accessible_class_init (BtkAccessibleClass *klass)
{
  klass->connect_widget_destroyed = btk_accessible_real_connect_widget_destroyed;
}

/**
 * btk_accessible_set_widget:
 * @accessible: a #BtkAccessible
 * @widget: a #BtkWidget
 *
 * Sets the #BtkWidget corresponding to the #BtkAccessible.
 *
 * Since: 2.22
 **/
void
btk_accessible_set_widget (BtkAccessible *accessible,
                           BtkWidget     *widget)
{
  g_return_if_fail (BTK_IS_ACCESSIBLE (accessible));

  accessible->widget = widget;
}

/**
 * btk_accessible_get_widget:
 * @accessible: a #BtkAccessible
 *
 * Gets the #BtkWidget corresponding to the #BtkAccessible. The returned widget
 * does not have a reference added, so you do not need to unref it.
 *
 * Returns: (transfer none): pointer to the #BtkWidget corresponding to
 *   the #BtkAccessible, or %NULL.
 *
 * Since: 2.22
 **/
BtkWidget*
btk_accessible_get_widget (BtkAccessible *accessible)
{
  g_return_val_if_fail (BTK_IS_ACCESSIBLE (accessible), NULL);

  return accessible->widget;
}

/**
 * btk_accessible_connect_widget_destroyed
 * @accessible: a #BtkAccessible
 *
 * This function specifies the callback function to be called when the widget
 * corresponding to a BtkAccessible is destroyed.
 */
void
btk_accessible_connect_widget_destroyed (BtkAccessible *accessible)
{
  BtkAccessibleClass *class;

  g_return_if_fail (BTK_IS_ACCESSIBLE (accessible));

  class = BTK_ACCESSIBLE_GET_CLASS (accessible);

  if (class->connect_widget_destroyed)
    class->connect_widget_destroyed (accessible);
}

static void
btk_accessible_real_connect_widget_destroyed (BtkAccessible *accessible)
{
  if (accessible->widget)
  {
    g_signal_connect (accessible->widget,
                      "destroy",
                      G_CALLBACK (btk_widget_destroyed),
                      &accessible->widget);
  }
}

#define __BTK_ACCESSIBLE_C__
#include "btkaliasdef.c"
