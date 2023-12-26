/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2001 Sun Microsystems Inc.
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
 */

#ifndef __BAIL_BOX_H__
#define __BAIL_BOX_H__

#include <bail/bailcontainer.h>

B_BEGIN_DECLS

#define BAIL_TYPE_BOX                        (bail_box_get_type ())
#define BAIL_BOX(obj)                        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_BOX, BailBox))
#define BAIL_BOX_CLASS(klass)                (G_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_BOX, BailBoxClass))
#define BAIL_IS_BOX(obj)                     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_BOX))
#define BAIL_IS_BOX_CLASS(klass)             (G_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_BOX))
#define BAIL_BOX_GET_CLASS(obj)              (G_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_BOX, BailBoxClass))

typedef struct _BailBox              BailBox;
typedef struct _BailBoxClass         BailBoxClass;

struct _BailBox
{
  BailContainer parent;
};

GType bail_box_get_type (void);

struct _BailBoxClass
{
  BailContainerClass parent_class;
};

B_END_DECLS

#endif /* __BAIL_BOX_H__ */
