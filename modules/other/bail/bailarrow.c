/* BAIL - The BUNNY Accessibility Implementation Library
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

#include <btk/btk.h>
#include "bailarrow.h"

static void bail_arrow_class_init	(BailArrowClass *klass);
static void bail_arrow_init		(BailArrow	*arrow);
static void bail_arrow_initialize       (BatkObject      *accessible,
                                         gpointer        data);

/* BatkImage */
static void  batk_image_interface_init   (BatkImageIface  *iface);
static const gchar* bail_arrow_get_image_description
                                        (BatkImage       *obj);
static gboolean bail_arrow_set_image_description 
                                        (BatkImage       *obj,
                                        const gchar    *description);
static void  bail_arrow_finalize       (BObject         *object);

G_DEFINE_TYPE_WITH_CODE (BailArrow, bail_arrow, BAIL_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_IMAGE, batk_image_interface_init))

static void	 
bail_arrow_class_init		(BailArrowClass *klass)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);
  BatkObjectClass *batk_object_class = BATK_OBJECT_CLASS (klass);

  batk_object_class->initialize = bail_arrow_initialize;

  bobject_class->finalize = bail_arrow_finalize;
}

static void
bail_arrow_init (BailArrow *arrow)
{
  arrow->image_description = NULL;
}

static void
bail_arrow_initialize (BatkObject *accessible,
                       gpointer data)
{
  BATK_OBJECT_CLASS (bail_arrow_parent_class)->initialize (accessible, data);

  accessible->role = BATK_ROLE_ICON;
}

static void
batk_image_interface_init (BatkImageIface *iface)
{
  iface->get_image_description = bail_arrow_get_image_description;
  iface->set_image_description = bail_arrow_set_image_description;
}

static const gchar*
bail_arrow_get_image_description (BatkImage       *obj)
{
  BailArrow* arrow;

  g_return_val_if_fail(BAIL_IS_ARROW(obj), NULL);

  arrow = BAIL_ARROW (obj);

  return arrow->image_description;
}

static gboolean 
bail_arrow_set_image_description (BatkImage       *obj,
                                  const gchar    *description)
{
  BailArrow* arrow;

  g_return_val_if_fail(BAIL_IS_ARROW(obj), FALSE);

  arrow = BAIL_ARROW (obj);
  g_free (arrow->image_description);

  arrow->image_description = g_strdup (description);

  return TRUE;

}

/*
 * static void  
 * bail_arrow_get_image_size (BatkImage       *obj,
 *                          gint           *height,
 *                          gint           *width)
 *
 * We dont implement this function for BailArrow as btk hardcodes the size 
 * of the arrow to be 7x5 and it is not possible to query this.
 */

static void
bail_arrow_finalize (BObject      *object)
{
  BailArrow *arrow = BAIL_ARROW (object);

  g_free (arrow->image_description);
  B_OBJECT_CLASS (bail_arrow_parent_class)->finalize (object);
}
