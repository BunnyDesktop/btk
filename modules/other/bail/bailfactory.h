/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2002 Sun Microsystems Inc.
 * Copyright (C) 1998-1999, 2000-2001 Tim Janik and Red Hat, Inc.
 * Copyright (C) 2007 Christian Persch
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

#ifndef _BAIL_FACTORY_H__
#define _BAIL_FACTORY_H__

#include <bunnylib-object.h>
#include <batk/batk.h>

#define _BAIL_IMPLEMENT_FACTORY_CREATE_ACCESSIBLE(BAIL_TYPE, TYPE) \
{ \
  BatkObject *accessible; \
\
  g_return_val_if_fail (B_TYPE_CHECK_INSTANCE_TYPE (object, TYPE), NULL); \
\
  accessible = g_object_new (BAIL_TYPE, NULL); \
  batk_object_initialize (accessible, object); \
\
  return accessible; \
}

#define _BAIL_IMPLEMENT_FACTORY_BEGIN(BAIL_TYPE, TypeName, type_name) \
\
static GType \
type_name##_factory_get_accessible_type (void) \
{ \
  return BAIL_TYPE; \
} \
\
static BatkObject* \
type_name##_factory_create_accessible (BObject *object) \
{

#define _BAIL_IMPLEMENT_FACTORY_END(BAIL_TYPE, TypeName, type_name) \
} \
\
static void \
type_name##_factory_class_init (BatkObjectFactoryClass *klass) \
{ \
  klass->create_accessible   = type_name ## _factory_create_accessible;	\
  klass->get_accessible_type = type_name ## _factory_get_accessible_type;\
} \
\
GType \
type_name##_factory_get_type (void) \
{ \
  static volatile gsize g_define_type_id__volatile = 0; \
  if (g_once_init_enter (&g_define_type_id__volatile)) \
    { \
      GType g_define_type_id = \
        g_type_register_static_simple (BATK_TYPE_OBJECT_FACTORY, \
                                       g_intern_static_string (#TypeName "Factory"), \
                                       sizeof (BatkObjectFactoryClass), \
                                       (GClassInitFunc) type_name##_factory_class_init, \
                                       sizeof (BatkObjectFactory), \
                                       (GInstanceInitFunc) NULL, \
                                       (GTypeFlags) 0); \
      g_once_init_leave (&g_define_type_id__volatile, g_define_type_id); \
    } \
  return g_define_type_id__volatile; \
}

/* Implements a BatkObjectFactory creating accessibles of type
 * @BAIL_TYPE for objects of type @TYPE.
 */
#define BAIL_IMPLEMENT_FACTORY(BAIL_TYPE, TypeName, type_name, TYPE) \
_BAIL_IMPLEMENT_FACTORY_BEGIN (BAIL_TYPE, TypeName, type_name) \
_BAIL_IMPLEMENT_FACTORY_CREATE_ACCESSIBLE (BAIL_TYPE, TYPE) \
_BAIL_IMPLEMENT_FACTORY_END (BAIL_TYPE, TypeName, type_name)

/* Implements a BatkObjectFactory creating accessibles of type
 * @BAIL_TYPE with creation func @create_accessible.
 */
#define BAIL_IMPLEMENT_FACTORY_WITH_FUNC(BAIL_TYPE, TypeName, type_name, create_accessible) \
_BAIL_IMPLEMENT_FACTORY_BEGIN (BAIL_TYPE, TypeName, type_name) \
{ return create_accessible (BTK_WIDGET (object)); } \
_BAIL_IMPLEMENT_FACTORY_END (BAIL_TYPE, TypeName, type_name)

#define BAIL_IMPLEMENT_FACTORY_WITH_FUNC_DUMMY(BAIL_TYPE, TypeName, type_name, TYPE, create_accessible) \
_BAIL_IMPLEMENT_FACTORY_BEGIN (BAIL_TYPE, TypeName, type_name) \
{ \
  g_return_val_if_fail (B_TYPE_CHECK_INSTANCE_TYPE (object, TYPE), NULL);\
  return create_accessible (); \
} \
_BAIL_IMPLEMENT_FACTORY_END (BAIL_TYPE, TypeName, type_name)

#define BAIL_WIDGET_SET_FACTORY(widget_type, type_as_function)			\
	batk_registry_set_factory_type (batk_get_default_registry (),		\
				       widget_type,				\
				       type_as_function ## _factory_get_type ())

#endif /* _BAIL_FACTORY_H__ */
