/* btkbuildable.c
 * Copyright (C) 2006-2007 Async Open Source,
 *                         Johan Dahlin <jdahlin@async.com.br>
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

/**
 * SECTION:btkbuildable
 * @Short_description: Interface for objects that can be built by BtkBuilder
 * @Title: BtkBuildable
 *
 * In order to allow construction from a <link linkend="BUILDER-UI">BtkBuilder
 * UI description</link>, an object class must implement the
 * BtkBuildable interface. The interface includes methods for setting
 * names and properties of objects, parsing custom tags, constructing
 * child objects.
 *
 * The BtkBuildable interface is implemented by all widgets and
 * many of the non-widget objects that are provided by BTK+. The
 * main user of this interface is #BtkBuilder, there should be
 * very little need for applications to call any
 * <function>btk_buildable_...</function> functions.
 */

#include "config.h"
#include "btkbuildable.h"
#include "btktypeutils.h"
#include "btkintl.h"
#include "btkalias.h"


typedef BtkBuildableIface BtkBuildableInterface;
G_DEFINE_INTERFACE (BtkBuildable, btk_buildable, B_TYPE_OBJECT)

static void
btk_buildable_default_init (BtkBuildableInterface *iface)
{
}

/**
 * btk_buildable_set_name:
 * @buildable: a #BtkBuildable
 * @name: name to set
 *
 * Sets the name of the @buildable object.
 *
 * Since: 2.12
 **/
void
btk_buildable_set_name (BtkBuildable *buildable,
                        const gchar  *name)
{
  BtkBuildableIface *iface;

  g_return_if_fail (BTK_IS_BUILDABLE (buildable));
  g_return_if_fail (name != NULL);

  iface = BTK_BUILDABLE_GET_IFACE (buildable);

  if (iface->set_name)
    (* iface->set_name) (buildable, name);
  else
    g_object_set_data_full (B_OBJECT (buildable),
			    "btk-builder-name",
			    g_strdup (name),
			    g_free);
}

/**
 * btk_buildable_get_name:
 * @buildable: a #BtkBuildable
 *
 * Gets the name of the @buildable object. 
 * 
 * #BtkBuilder sets the name based on the the 
 * <link linkend="BUILDER-UI">BtkBuilder UI definition</link> 
 * used to construct the @buildable.
 *
 * Returns: the name set with btk_buildable_set_name()
 *
 * Since: 2.12
 **/
const gchar *
btk_buildable_get_name (BtkBuildable *buildable)
{
  BtkBuildableIface *iface;

  g_return_val_if_fail (BTK_IS_BUILDABLE (buildable), NULL);

  iface = BTK_BUILDABLE_GET_IFACE (buildable);

  if (iface->get_name)
    return (* iface->get_name) (buildable);
  else
    return (const gchar*)g_object_get_data (B_OBJECT (buildable),
					    "btk-builder-name");
}

/**
 * btk_buildable_add_child:
 * @buildable: a #BtkBuildable
 * @builder: a #BtkBuilder
 * @child: child to add
 * @type: (allow-none): kind of child or %NULL
 *
 * Adds a child to @buildable. @type is an optional string
 * describing how the child should be added.
 *
 * Since: 2.12
 **/
void
btk_buildable_add_child (BtkBuildable *buildable,
			 BtkBuilder   *builder,
			 BObject      *child,
			 const gchar  *type)
{
  BtkBuildableIface *iface;

  g_return_if_fail (BTK_IS_BUILDABLE (buildable));
  g_return_if_fail (BTK_IS_BUILDER (builder));

  iface = BTK_BUILDABLE_GET_IFACE (buildable);
  g_return_if_fail (iface->add_child != NULL);

  (* iface->add_child) (buildable, builder, child, type);
}

/**
 * btk_buildable_set_buildable_property:
 * @buildable: a #BtkBuildable
 * @builder: a #BtkBuilder
 * @name: name of property
 * @value: value of property
 *
 * Sets the property name @name to @value on the @buildable object.
 *
 * Since: 2.12
 **/
void
btk_buildable_set_buildable_property (BtkBuildable *buildable,
				      BtkBuilder   *builder,
				      const gchar  *name,
				      const BValue *value)
{
  BtkBuildableIface *iface;

  g_return_if_fail (BTK_IS_BUILDABLE (buildable));
  g_return_if_fail (BTK_IS_BUILDER (builder));
  g_return_if_fail (name != NULL);
  g_return_if_fail (value != NULL);

  iface = BTK_BUILDABLE_GET_IFACE (buildable);
  if (iface->set_buildable_property)
    (* iface->set_buildable_property) (buildable, builder, name, value);
  else
    g_object_set_property (B_OBJECT (buildable), name, value);
}

/**
 * btk_buildable_parser_finished:
 * @buildable: a #BtkBuildable
 * @builder: a #BtkBuilder
 *
 * Called when the builder finishes the parsing of a 
 * <link linkend="BUILDER-UI">BtkBuilder UI definition</link>. 
 * Note that this will be called once for each time 
 * btk_builder_add_from_file() or btk_builder_add_from_string() 
 * is called on a builder.
 *
 * Since: 2.12
 **/
void
btk_buildable_parser_finished (BtkBuildable *buildable,
			       BtkBuilder   *builder)
{
  BtkBuildableIface *iface;

  g_return_if_fail (BTK_IS_BUILDABLE (buildable));
  g_return_if_fail (BTK_IS_BUILDER (builder));

  iface = BTK_BUILDABLE_GET_IFACE (buildable);
  if (iface->parser_finished)
    (* iface->parser_finished) (buildable, builder);
}

/**
 * btk_buildable_construct_child:
 * @buildable: A #BtkBuildable
 * @builder: #BtkBuilder used to construct this object
 * @name: name of child to construct
 *
 * Constructs a child of @buildable with the name @name.
 *
 * #BtkBuilder calls this function if a "constructor" has been
 * specified in the UI definition.
 *
 * Returns: (transfer full): the constructed child
 *
 * Since: 2.12
 **/
BObject *
btk_buildable_construct_child (BtkBuildable *buildable,
                               BtkBuilder   *builder,
                               const gchar  *name)
{
  BtkBuildableIface *iface;

  g_return_val_if_fail (BTK_IS_BUILDABLE (buildable), NULL);
  g_return_val_if_fail (BTK_IS_BUILDER (builder), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  iface = BTK_BUILDABLE_GET_IFACE (buildable);
  g_return_val_if_fail (iface->construct_child != NULL, NULL);

  return (* iface->construct_child) (buildable, builder, name);
}

/**
 * btk_buildable_custom_tag_start:
 * @buildable: a #BtkBuildable
 * @builder: a #BtkBuilder used to construct this object
 * @child: (allow-none): child object or %NULL for non-child tags
 * @tagname: name of tag
 * @parser: (out): a #GMarkupParser structure to fill in
 * @data: (out): return location for user data that will be passed in 
 *   to parser functions
 *
 * This is called for each unknown element under &lt;child&gt;.
 * 
 * Returns: %TRUE if a object has a custom implementation, %FALSE
 *          if it doesn't.
 *
 * Since: 2.12
 **/
gboolean
btk_buildable_custom_tag_start (BtkBuildable  *buildable,
                                BtkBuilder    *builder,
                                BObject       *child,
                                const gchar   *tagname,
                                GMarkupParser *parser,
                                gpointer      *data)
{
  BtkBuildableIface *iface;

  g_return_val_if_fail (BTK_IS_BUILDABLE (buildable), FALSE);
  g_return_val_if_fail (BTK_IS_BUILDER (builder), FALSE);
  g_return_val_if_fail (tagname != NULL, FALSE);

  iface = BTK_BUILDABLE_GET_IFACE (buildable);
  g_return_val_if_fail (iface->custom_tag_start != NULL, FALSE);

  return (* iface->custom_tag_start) (buildable, builder, child,
                                      tagname, parser, data);
}

/**
 * btk_buildable_custom_tag_end:
 * @buildable: A #BtkBuildable
 * @builder: #BtkBuilder used to construct this object
 * @child: (allow-none): child object or %NULL for non-child tags
 * @tagname: name of tag
 * @data: (type gpointer): user data that will be passed in to parser functions
 *
 * This is called at the end of each custom element handled by 
 * the buildable.
 *
 * Since: 2.12
 **/
void
btk_buildable_custom_tag_end (BtkBuildable  *buildable,
                              BtkBuilder    *builder,
                              BObject       *child,
                              const gchar   *tagname,
                              gpointer      *data)
{
  BtkBuildableIface *iface;

  g_return_if_fail (BTK_IS_BUILDABLE (buildable));
  g_return_if_fail (BTK_IS_BUILDER (builder));
  g_return_if_fail (tagname != NULL);

  iface = BTK_BUILDABLE_GET_IFACE (buildable);
  if (iface->custom_tag_end)
    (* iface->custom_tag_end) (buildable, builder, child, tagname, data);
}

/**
 * btk_buildable_custom_finished:
 * @buildable: a #BtkBuildable
 * @builder: a #BtkBuilder
 * @child: (allow-none): child object or %NULL for non-child tags
 * @tagname: the name of the tag
 * @data: user data created in custom_tag_start
 *
 * This is similar to btk_buildable_parser_finished() but is
 * called once for each custom tag handled by the @buildable.
 * 
 * Since: 2.12
 **/
void
btk_buildable_custom_finished (BtkBuildable  *buildable,
			       BtkBuilder    *builder,
			       BObject       *child,
			       const gchar   *tagname,
			       gpointer       data)
{
  BtkBuildableIface *iface;

  g_return_if_fail (BTK_IS_BUILDABLE (buildable));
  g_return_if_fail (BTK_IS_BUILDER (builder));

  iface = BTK_BUILDABLE_GET_IFACE (buildable);
  if (iface->custom_finished)
    (* iface->custom_finished) (buildable, builder, child, tagname, data);
}

/**
 * btk_buildable_get_internal_child:
 * @buildable: a #BtkBuildable
 * @builder: a #BtkBuilder
 * @childname: name of child
 *
 * Get the internal child called @childname of the @buildable object.
 *
 * Returns: (transfer none): the internal child of the buildable object
 *
 * Since: 2.12
 **/
BObject *
btk_buildable_get_internal_child (BtkBuildable *buildable,
                                  BtkBuilder   *builder,
                                  const gchar  *childname)
{
  BtkBuildableIface *iface;

  g_return_val_if_fail (BTK_IS_BUILDABLE (buildable), NULL);
  g_return_val_if_fail (BTK_IS_BUILDER (builder), NULL);
  g_return_val_if_fail (childname != NULL, NULL);

  iface = BTK_BUILDABLE_GET_IFACE (buildable);
  if (!iface->get_internal_child)
    return NULL;

  return (* iface->get_internal_child) (buildable, builder, childname);
}

#define __BTK_BUILDABLE_C__
#include "btkaliasdef.c"
