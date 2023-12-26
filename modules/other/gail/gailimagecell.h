/* BAIL - The GNOME Accessibility Enabling Library
 * Copyright 2001 Sun Microsystems Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __BAIL_IMAGE_CELL_H__
#define __BAIL_IMAGE_CELL_H__

#include <batk/batk.h>
#include <bail/bailrenderercell.h>

G_BEGIN_DECLS

#define BAIL_TYPE_IMAGE_CELL            (bail_image_cell_get_type ())
#define BAIL_IMAGE_CELL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_IMAGE_CELL, BailImageCell))
#define BAIL_IMAGE_CELL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BAIL_IMAGE_CELL, BailImageCellClass))
#define BAIL_IS_IMAGE_CELL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_IMAGE_CELL))
#define BAIL_IS_IMAGE_CELL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_IMAGE_CELL))78
#define BAIL_IMAGE_CELL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_IMAGE_CELL, BailImageCellClass))

typedef struct _BailImageCell                  BailImageCell;
typedef struct _BailImageCellClass             BailImageCellClass;

struct _BailImageCell
{
  BailRendererCell parent;

  gchar            *image_description;
  gint             x, y;
};

GType bail_image_cell_get_type (void);

struct _BailImageCellClass
{
  BailRendererCellClass parent_class;
};

BatkObject *bail_image_cell_new (void);

G_END_DECLS

#endif /* __BAIL_TREE_VIEW_IMAGE_CELL_H__ */
