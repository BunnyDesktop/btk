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

#ifndef __BAIL_LIST_H__
#define __BAIL_LIST_H__

#include <bail/bailcontainer.h>

B_BEGIN_DECLS

#define BAIL_TYPE_LIST                       (bail_list_get_type ())
#define BAIL_LIST(obj)                       (B_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_LIST, BailList))
#define BAIL_LIST_CLASS(klass)               (B_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_LIST, BailListClass))
#define BAIL_IS_LIST(obj)                    (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_LIST))
#define BAIL_IS_LIST_CLASS(klass)            (B_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_LIST))
#define BAIL_LIST_GET_CLASS(obj)             (B_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_LIST, BailListClass))

typedef struct _BailList              BailList;
typedef struct _BailListClass         BailListClass;

struct _BailList
{
  BailContainer parent;
};

GType bail_list_get_type (void);

struct _BailListClass
{
  BailContainerClass parent_class;
};

B_END_DECLS

#endif /* __BAIL_LIST_H__ */
