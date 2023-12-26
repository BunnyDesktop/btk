/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2001, 2002, 2003 Sun Microsystems Inc.
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

#ifndef __BAIL_CONTAINER_H__
#define __BAIL_CONTAINER_H__

#include <bail/bailwidget.h>

B_BEGIN_DECLS

#define BAIL_TYPE_CONTAINER                  (bail_container_get_type ())
#define BAIL_CONTAINER(obj)                  (B_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_CONTAINER, BailContainer))
#define BAIL_CONTAINER_CLASS(klass)          (B_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_CONTAINER, BailContainerClass))
#define BAIL_IS_CONTAINER(obj)               (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_CONTAINER))
#define BAIL_IS_CONTAINER_CLASS(klass)       (B_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_CONTAINER))
#define BAIL_CONTAINER_GET_CLASS(obj)        (B_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_CONTAINER, BailContainerClass))

typedef struct _BailContainer                 BailContainer;
typedef struct _BailContainerClass            BailContainerClass;

struct _BailContainer
{
  BailWidget parent;

  /*
   * Cached list of children
   */
  GList      *children;
};

GType bail_container_get_type (void);

struct _BailContainerClass
{
  BailWidgetClass parent_class;

  bint (*add_btk) (BtkContainer *container,
                   BtkWidget    *widget,
                   bpointer     data);
  bint (*remove_btk) (BtkContainer *container,
                      BtkWidget    *widget,
                      bpointer     data);
};

B_END_DECLS

#endif /* __BAIL_CONTAINER_H__ */
