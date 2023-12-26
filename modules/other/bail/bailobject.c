/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2003 Sun Microsystems Inc.
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
#include "bailobject.h"

static void       bail_object_class_init               (BailObjectClass *klass);

static void       bail_object_init                     (BailObject *object);

static void       bail_object_real_initialize          (BatkObject       *obj,
                                                        bpointer        data);

G_DEFINE_TYPE (BailObject, bail_object, BATK_TYPE_BOBJECT_ACCESSIBLE)

static void
bail_object_class_init (BailObjectClass *klass)
{
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);

  class->initialize = bail_object_real_initialize;
}

static void
bail_object_init (BailObject *object)
{
}

static void
bail_object_real_initialize (BatkObject *obj,
                             bpointer  data)
{
  BATK_OBJECT_CLASS (bail_object_parent_class)->initialize (obj, data);

  obj->role = BATK_ROLE_UNKNOWN;
}
