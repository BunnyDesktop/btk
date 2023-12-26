/* BAIL - The GNOME Accessibility Enabling Library
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
#include "bailimagecell.h"

static void      bail_image_cell_class_init          (BailImageCellClass *klass);
static void      bail_image_cell_init                (BailImageCell      *image_cell);

static void      bail_image_cell_finalize            (GObject            *object);

/* BatkImage */
static void      batk_image_interface_init              (BatkImageIface  *iface);
static const gchar *
                 bail_image_cell_get_image_description (BatkImage       *image);
static gboolean  bail_image_cell_set_image_description (BatkImage       *image,
                                                        const gchar    *description);
static void      bail_image_cell_get_image_position    (BatkImage       *image,
                                                        gint           *x,
                                                        gint           *y,
                                                        BatkCoordType   coord_type);
static void      bail_image_cell_get_image_size        (BatkImage       *image,
                                                        gint           *width,
                                                        gint           *height);

/* Misc */

static gboolean  bail_image_cell_update_cache          (BailRendererCell *cell,
                                                        gboolean         emit_change_signal);

// FIXMEchpe static!!!
gchar *bail_image_cell_property_list[] = {
  "pixbuf",
  NULL
};

G_DEFINE_TYPE_WITH_CODE (BailImageCell, bail_image_cell, BAIL_TYPE_RENDERER_CELL,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_IMAGE, batk_image_interface_init))

static void 
bail_image_cell_class_init (BailImageCellClass *klass)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (klass);
  BailRendererCellClass *renderer_cell_class = BAIL_RENDERER_CELL_CLASS(klass);

  bobject_class->finalize = bail_image_cell_finalize;

  renderer_cell_class->update_cache = bail_image_cell_update_cache;
  renderer_cell_class->property_list = bail_image_cell_property_list;
}

BatkObject* 
bail_image_cell_new (void)
{
  GObject *object;
  BatkObject *batk_object;
  BailRendererCell *cell;

  object = g_object_new (BAIL_TYPE_IMAGE_CELL, NULL);

  g_return_val_if_fail (object != NULL, NULL);

  batk_object = BATK_OBJECT (object);
  batk_object->role = BATK_ROLE_TABLE_CELL;

  cell = BAIL_RENDERER_CELL(object);

  cell->renderer = btk_cell_renderer_pixbuf_new ();
  g_object_ref_sink (cell->renderer);
  return batk_object;
}

static void
bail_image_cell_init (BailImageCell *image_cell)
{
  image_cell->image_description = NULL;
}


static void
bail_image_cell_finalize (GObject *object)
{
  BailImageCell *image_cell = BAIL_IMAGE_CELL (object);

  g_free (image_cell->image_description);
  G_OBJECT_CLASS (bail_image_cell_parent_class)->finalize (object);
}

static gboolean
bail_image_cell_update_cache (BailRendererCell *cell, 
                              gboolean         emit_change_signal)
{
  return FALSE;
}

static void
batk_image_interface_init (BatkImageIface  *iface)
{
  iface->get_image_description = bail_image_cell_get_image_description;
  iface->set_image_description = bail_image_cell_set_image_description;
  iface->get_image_position = bail_image_cell_get_image_position;
  iface->get_image_size = bail_image_cell_get_image_size;
}

static const gchar *
bail_image_cell_get_image_description (BatkImage     *image)
{
  BailImageCell *image_cell;

  image_cell = BAIL_IMAGE_CELL (image);
  return image_cell->image_description;
}

static gboolean
bail_image_cell_set_image_description (BatkImage     *image,
                                       const gchar  *description)
{
  BailImageCell *image_cell;

  image_cell = BAIL_IMAGE_CELL (image);
  g_free (image_cell->image_description);
  image_cell->image_description = g_strdup (description);
  if (image_cell->image_description)
    return TRUE;
  else
    return FALSE;
}

static void
bail_image_cell_get_image_position (BatkImage     *image,
                                    gint         *x,
                                    gint         *y,
                                    BatkCoordType coord_type)
{
  batk_component_get_position (BATK_COMPONENT (image), x, y, coord_type);
}

static void
bail_image_cell_get_image_size (BatkImage *image,
                                gint     *width,
                                gint     *height)
{
  BailImageCell *cell = BAIL_IMAGE_CELL (image);
  BtkCellRenderer *cell_renderer;
  BdkPixbuf *pixbuf;

  cell_renderer  = BAIL_RENDERER_CELL (cell)->renderer;
  pixbuf = BTK_CELL_RENDERER_PIXBUF (cell_renderer)->pixbuf;

  *width = bdk_pixbuf_get_width (pixbuf);
  *height = bdk_pixbuf_get_height (pixbuf);
}
