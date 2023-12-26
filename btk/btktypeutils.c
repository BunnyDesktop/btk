/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#undef BTK_DISABLE_DEPRECATED

#include "config.h"
#include <string.h> /* strcmp */

#include "btktypeutils.h"
#include "btkobject.h"
#include "btkintl.h"
#include "btkalias.h"


/* --- functions --- */
BtkType
btk_type_unique (BtkType            parent_type,
		 const BtkTypeInfo *btkinfo)
{
  GTypeInfo tinfo = { 0, };

  g_return_val_if_fail (BTK_TYPE_IS_OBJECT (parent_type), 0);
  g_return_val_if_fail (btkinfo != NULL, 0);
  g_return_val_if_fail (btkinfo->type_name != NULL, 0);
  g_return_val_if_fail (g_type_from_name (btkinfo->type_name) == 0, 0);

  tinfo.class_size = btkinfo->class_size;
  tinfo.base_init = btkinfo->base_class_init_func;
  tinfo.base_finalize = NULL;
  tinfo.class_init = (GClassInitFunc) btkinfo->class_init_func;
  tinfo.class_finalize = NULL;
  tinfo.class_data = NULL;
  tinfo.instance_size = btkinfo->object_size;
  tinfo.n_preallocs = 0;
  tinfo.instance_init = btkinfo->object_init_func;

  return g_type_register_static (parent_type, btkinfo->type_name, &tinfo, 0);
}

/**
 * btk_type_class
 * @type: a #BtkType.
 *
 * Returns a pointer pointing to the class of @type or %NULL if there
 * was any trouble identifying @type.  Initializes the class if
 * necessary.
 *
 * Returns: pointer to the class.
 *
 * Deprecated: 2.14: Use g_type_class_peek() or g_type_class_ref() instead.
 **/
bpointer
btk_type_class (BtkType type)
{
  static GQuark quark_static_class = 0;
  bpointer class;

  if (!B_TYPE_IS_ENUM (type) && !B_TYPE_IS_FLAGS (type))
    g_return_val_if_fail (B_TYPE_IS_OBJECT (type), NULL);

  /* ok, this is a bit ugly, GLib reference counts classes,
   * and btk_type_class() used to always return static classes.
   * while we coud be faster with just peeking the bunnylib class
   * for the normal code path, we can't be sure that that
   * class stays around (someone else might be holding the
   * reference count and is going to drop it later). so to
   * ensure that Btk actually holds a static reference count
   * on the class, we use GType qdata to store referenced
   * classes, and only return those.
   */

  class = g_type_get_qdata (type, quark_static_class);
  if (!class)
    {
      if (!quark_static_class)
	quark_static_class = g_quark_from_static_string ("BtkStaticTypeClass");

      class = g_type_class_ref (type);
      g_assert (class != NULL);
      g_type_set_qdata (type, quark_static_class, class);
    }

  return class;
}

bpointer
btk_type_new (BtkType type)
{
  bpointer object;

  g_return_val_if_fail (BTK_TYPE_IS_OBJECT (type), NULL);

  object = g_object_new (type, NULL);

  return object;
}

void
btk_type_init (GTypeDebugFlags debug_flags)
{
  static bboolean initialized = FALSE;
  
  if (!initialized)
    {
      GType unused;

      initialized = TRUE;

      /* initialize GLib type system
       */
      g_type_init_with_debug_flags (debug_flags);
      
      /* BTK_TYPE_OBJECT
       */
      unused = btk_object_get_type ();
    }
}

GType
btk_identifier_get_type (void)
{
  static GType our_type = 0;
  
  if (our_type == 0)
    {
      GTypeInfo tinfo = { 0, };
      our_type = g_type_register_static (B_TYPE_STRING, I_("BtkIdentifier"), &tinfo, 0);
    }

  return our_type;
}

BtkEnumValue*
btk_type_enum_get_values (BtkType enum_type)
{
  GEnumClass *class;

  g_return_val_if_fail (B_TYPE_IS_ENUM (enum_type), NULL);
  
  class = btk_type_class (enum_type);
  
  return class->values;
}

BtkFlagValue*
btk_type_flags_get_values (BtkType flags_type)
{
  GFlagsClass *class;

  g_return_val_if_fail (B_TYPE_IS_FLAGS (flags_type), NULL);

  class = btk_type_class (flags_type);

  return class->values;
}

BtkEnumValue*
btk_type_enum_find_value (BtkType      enum_type,
			  const bchar *value_name)
{
  BtkEnumValue *value;
  GEnumClass *class;

  g_return_val_if_fail (B_TYPE_IS_ENUM (enum_type), NULL);
  g_return_val_if_fail (value_name != NULL, NULL);

  class = btk_type_class (enum_type);
  value = g_enum_get_value_by_name (class, value_name);
  if (!value)
    value = g_enum_get_value_by_nick (class, value_name);

  return value;
}

BtkFlagValue*
btk_type_flags_find_value (BtkType      flags_type,
			   const bchar *value_name)
{
  BtkFlagValue *value;
  GFlagsClass *class;

  g_return_val_if_fail (B_TYPE_IS_FLAGS (flags_type), NULL);
  g_return_val_if_fail (value_name != NULL, NULL);

  class = btk_type_class (flags_type);
  value = g_flags_get_value_by_name (class, value_name);
  if (!value)
    value = g_flags_get_value_by_nick (class, value_name);

  return value;
}


#define __BTK_TYPE_UTILS_C__
#include "btkaliasdef.c"
