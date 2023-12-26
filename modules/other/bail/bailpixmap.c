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

#undef BTK_DISABLE_DEPRECATED

#include <btk/btk.h>

#include "bailpixmap.h"

static void	 bail_pixmap_class_init		(BailPixmapClass *klass);
static void      bail_pixmap_init               (BailPixmap      *pixmap);
static void      bail_pixmap_initialize         (BatkObject       *accessible,
                                                 gpointer         data);

/* BatkImage */
static void  batk_image_interface_init   (BatkImageIface  *iface);
static const gchar* bail_pixmap_get_image_description
                                        (BatkImage       *obj);
static void  bail_pixmap_get_image_position    
                                        (BatkImage       *obj,
                                         gint           *x,
                                         gint           *y,
                                         BatkCoordType   coord_type);
static void  bail_pixmap_get_image_size (BatkImage       *obj,
                                         gint           *width,
                                         gint           *height);
static gboolean bail_pixmap_set_image_description 
                                        (BatkImage       *obj,
                                        const gchar    *description);
static void  bail_pixmap_finalize       (BObject         *object);

G_DEFINE_TYPE_WITH_CODE (BailPixmap, bail_pixmap, BAIL_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_IMAGE, batk_image_interface_init))

static void	 
bail_pixmap_class_init (BailPixmapClass *klass)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);
  BatkObjectClass *batk_object_class = BATK_OBJECT_CLASS (klass);
 
  batk_object_class->initialize = bail_pixmap_initialize;

  bobject_class->finalize = bail_pixmap_finalize;
}

static void
bail_pixmap_init (BailPixmap *pixmap)
{
  pixmap->image_description = NULL;
}

static void
bail_pixmap_initialize (BatkObject *accessible,
                        gpointer  data)
{
  BATK_OBJECT_CLASS (bail_pixmap_parent_class)->initialize (accessible, data);

  accessible->role = BATK_ROLE_ICON;
}

static void
batk_image_interface_init (BatkImageIface *iface)
{
  iface->get_image_description = bail_pixmap_get_image_description;
  iface->get_image_position = bail_pixmap_get_image_position;
  iface->get_image_size = bail_pixmap_get_image_size;
  iface->set_image_description = bail_pixmap_set_image_description;
}

static const gchar*
bail_pixmap_get_image_description (BatkImage       *obj)
{
  BailPixmap* pixmap;

  g_return_val_if_fail (BAIL_IS_PIXMAP (obj), NULL);

  pixmap = BAIL_PIXMAP (obj);

  return pixmap->image_description;
}

static void
bail_pixmap_get_image_position (BatkImage       *obj,
                                gint           *x,
                                gint           *y,
                                BatkCoordType   coord_type)
{
  batk_component_get_position (BATK_COMPONENT (obj), x, y, coord_type);
}

static void  
bail_pixmap_get_image_size (BatkImage       *obj,
                            gint           *width,
                            gint           *height)
{
  BtkWidget *widget;
  BtkPixmap *pixmap;
 
  *width = -1;
  *height = -1;

  g_return_if_fail (BAIL_IS_PIXMAP (obj));

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == 0)
    /* State is defunct */
    return;

  g_return_if_fail (BTK_IS_PIXMAP (widget));

  pixmap = BTK_PIXMAP (widget);

  if (pixmap->pixmap)
    bdk_pixmap_get_size (pixmap->pixmap, width, height);
}

static gboolean 
bail_pixmap_set_image_description (BatkImage       *obj,
                                   const gchar    *description)
{ 
  BailPixmap* pixmap;

  g_return_val_if_fail (BAIL_IS_PIXMAP (obj), FALSE);

  pixmap = BAIL_PIXMAP (obj);
  g_free (pixmap->image_description);

  pixmap->image_description = g_strdup (description);

  return TRUE;
}

static void
bail_pixmap_finalize (BObject      *object)
{
  BailPixmap *pixmap = BAIL_PIXMAP (object);

  g_free (pixmap->image_description);
  B_OBJECT_CLASS (bail_pixmap_parent_class)->finalize (object);
}
