/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2003 Sun Microsystems Inc.
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

#ifndef __BAIL_OBJECT_H__
#define __BAIL_OBJECT_H__

#include <batk/batk.h>

B_BEGIN_DECLS

#define BAIL_TYPE_OBJECT                  (bail_object_get_type ())
#define BAIL_OBJECT(obj)                  (B_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_OBJECT, BailObject)
#define BAIL_OBJECT_CLASS(klass)          (B_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_OBJECT, BailObjectlass))
#define BAIL_IS_OBJECT(obj)               (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_OBJECT))
#define BAIL_IS_OBJECT_CLASS(klass)       (B_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_OBJECT))
#define BAIL_OBJECT_GET_CLASS(obj)        (B_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_OBJECT, BailObjectlass))

typedef struct _BailObject                 BailObject;
typedef struct _BailObjectClass            BailObjectClass;

struct _BailObject
{
  BatkBObjectAccessible parent;
};

GType bail_object_get_type (void);

struct _BailObjectClass
{
  BatkBObjectAccessibleClass parent_class;
};

B_END_DECLS

#endif /* __BAIL_OBJECT_H__ */
