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

#include "config.h"
#include "btktexttagtable.h"
#include "btkbuildable.h"
#include "btkmarshalers.h"
#include "btktextbuffer.h" /* just for the lame notify_will_remove_tag hack */
#include "btkintl.h"
#include "btkalias.h"

#include <stdlib.h>

/**
 * SECTION:btktexttagtable
 * @Short_description: Collection of tags that can be used together
 * @Title: BtkTextTagTable
 *
 * You may wish to begin by reading the <link linkend="TextWidget">text widget
 * conceptual overview</link> which gives an overview of all the objects and data
 * types related to the text widget and how they work together.
 *
 * <refsect2 id="BtkTextTagTable-BUILDER-UI">
 * <title>BtkTextTagTables as BtkBuildable</title>
 * <para>
 * The BtkTextTagTable implementation of the BtkBuildable interface
 * supports adding tags by specifying "tag" as the "type"
 * attribute of a &lt;child&gt; element.
 * </para>
 * <example>
 * <title>A UI definition fragment specifying tags</title>
 * <programlisting><![CDATA[
 * <object class="BtkTextTagTable">
 *  <child type="tag">
 *    <object class="BtkTextTag"/>
 *  </child>
 * </object>
 * ]]></programlisting>
 * </example>
 * </refsect2>
 */

enum {
  TAG_CHANGED,
  TAG_ADDED,
  TAG_REMOVED,
  LAST_SIGNAL
};

enum {
  LAST_ARG
};

static void btk_text_tag_table_finalize     (BObject              *object);
static void btk_text_tag_table_set_property (BObject              *object,
                                             buint                 prop_id,
                                             const BValue         *value,
                                             BParamSpec           *pspec);
static void btk_text_tag_table_get_property (BObject              *object,
                                             buint                 prop_id,
                                             BValue               *value,
                                             BParamSpec           *pspec);

static void btk_text_tag_table_buildable_interface_init (BtkBuildableIface   *iface);
static void btk_text_tag_table_buildable_add_child      (BtkBuildable        *buildable,
							 BtkBuilder          *builder,
							 BObject             *child,
							 const bchar         *type);

static buint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (BtkTextTagTable, btk_text_tag_table, B_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
                                                btk_text_tag_table_buildable_interface_init))

static void
btk_text_tag_table_class_init (BtkTextTagTableClass *klass)
{
  BObjectClass *object_class = B_OBJECT_CLASS (klass);

  object_class->set_property = btk_text_tag_table_set_property;
  object_class->get_property = btk_text_tag_table_get_property;
  
  object_class->finalize = btk_text_tag_table_finalize;
  
  signals[TAG_CHANGED] =
    g_signal_new (I_("tag-changed"),
                  B_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkTextTagTableClass, tag_changed),
                  NULL, NULL,
                  _btk_marshal_VOID__OBJECT_BOOLEAN,
                  B_TYPE_NONE,
                  2,
                  BTK_TYPE_TEXT_TAG,
                  B_TYPE_BOOLEAN);  

  signals[TAG_ADDED] =
    g_signal_new (I_("tag-added"),
                  B_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkTextTagTableClass, tag_added),
                  NULL, NULL,
                  _btk_marshal_VOID__OBJECT,
                  B_TYPE_NONE,
                  1,
                  BTK_TYPE_TEXT_TAG);

  signals[TAG_REMOVED] =
    g_signal_new (I_("tag-removed"),  
                  B_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkTextTagTableClass, tag_removed),
                  NULL, NULL,
                  _btk_marshal_VOID__OBJECT,
                  B_TYPE_NONE,
                  1,
                  BTK_TYPE_TEXT_TAG);
}

static void
btk_text_tag_table_init (BtkTextTagTable *table)
{
  table->hash = g_hash_table_new (g_str_hash, g_str_equal);
}

/**
 * btk_text_tag_table_new:
 * 
 * Creates a new #BtkTextTagTable. The table contains no tags by
 * default.
 * 
 * Return value: a new #BtkTextTagTable
 **/
BtkTextTagTable*
btk_text_tag_table_new (void)
{
  BtkTextTagTable *table;

  table = g_object_new (BTK_TYPE_TEXT_TAG_TABLE, NULL);

  return table;
}

static void
foreach_unref (BtkTextTag *tag, bpointer data)
{
  GSList *tmp;
  
  /* We don't want to emit the remove signal here; so we just unparent
   * and unref the tag.
   */

  tmp = tag->table->buffers;
  while (tmp != NULL)
    {
      _btk_text_buffer_notify_will_remove_tag (BTK_TEXT_BUFFER (tmp->data),
                                               tag);
      
      tmp = tmp->next;
    }
  
  tag->table = NULL;
  g_object_unref (tag);
}

static void
btk_text_tag_table_finalize (BObject *object)
{
  BtkTextTagTable *table;

  table = BTK_TEXT_TAG_TABLE (object);
  
  btk_text_tag_table_foreach (table, foreach_unref, NULL);

  g_hash_table_destroy (table->hash);
  b_slist_free (table->anonymous);

  b_slist_free (table->buffers);

  B_OBJECT_CLASS (btk_text_tag_table_parent_class)->finalize (object);
}
static void
btk_text_tag_table_set_property (BObject      *object,
                                 buint         prop_id,
                                 const BValue *value,
                                 BParamSpec   *pspec)
{
  switch (prop_id)
    {

    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
btk_text_tag_table_get_property (BObject      *object,
                                 buint         prop_id,
                                 BValue       *value,
                                 BParamSpec   *pspec)
{
  switch (prop_id)
    {

    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_text_tag_table_buildable_interface_init (BtkBuildableIface   *iface)
{
  iface->add_child = btk_text_tag_table_buildable_add_child;
}

static void
btk_text_tag_table_buildable_add_child (BtkBuildable        *buildable,
					BtkBuilder          *builder,
					BObject             *child,
					const bchar         *type)
{
  if (type && strcmp (type, "tag") == 0)
    btk_text_tag_table_add (BTK_TEXT_TAG_TABLE (buildable),
			    BTK_TEXT_TAG (child));
}

/**
 * btk_text_tag_table_add:
 * @table: a #BtkTextTagTable
 * @tag: a #BtkTextTag
 *
 * Add a tag to the table. The tag is assigned the highest priority
 * in the table.
 *
 * @tag must not be in a tag table already, and may not have
 * the same name as an already-added tag.
 **/
void
btk_text_tag_table_add (BtkTextTagTable *table,
                        BtkTextTag      *tag)
{
  buint size;

  g_return_if_fail (BTK_IS_TEXT_TAG_TABLE (table));
  g_return_if_fail (BTK_IS_TEXT_TAG (tag));
  g_return_if_fail (tag->table == NULL);

  if (tag->name && g_hash_table_lookup (table->hash, tag->name))
    {
      g_warning ("A tag named '%s' is already in the tag table.",
                 tag->name);
      return;
    }
  
  g_object_ref (tag);

  if (tag->name)
    g_hash_table_insert (table->hash, tag->name, tag);
  else
    {
      table->anonymous = b_slist_prepend (table->anonymous, tag);
      table->anon_count += 1;
    }

  tag->table = table;

  /* We get the highest tag priority, as the most-recently-added
     tag. Note that we do NOT use btk_text_tag_set_priority,
     as it assumes the tag is already in the table. */
  size = btk_text_tag_table_get_size (table);
  g_assert (size > 0);
  tag->priority = size - 1;

  g_signal_emit (table, signals[TAG_ADDED], 0, tag);
}

/**
 * btk_text_tag_table_lookup:
 * @table: a #BtkTextTagTable 
 * @name: name of a tag
 * 
 * Look up a named tag.
 * 
 * Return value: (transfer none): The tag, or %NULL if none by that name is in the table.
 **/
BtkTextTag*
btk_text_tag_table_lookup (BtkTextTagTable *table,
                           const bchar     *name)
{
  g_return_val_if_fail (BTK_IS_TEXT_TAG_TABLE (table), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  return g_hash_table_lookup (table->hash, name);
}

/**
 * btk_text_tag_table_remove:
 * @table: a #BtkTextTagTable
 * @tag: a #BtkTextTag
 * 
 * Remove a tag from the table. This will remove the table's
 * reference to the tag, so be careful - the tag will end
 * up destroyed if you don't have a reference to it.
 **/
void
btk_text_tag_table_remove (BtkTextTagTable *table,
                           BtkTextTag      *tag)
{
  GSList *tmp;
  
  g_return_if_fail (BTK_IS_TEXT_TAG_TABLE (table));
  g_return_if_fail (BTK_IS_TEXT_TAG (tag));
  g_return_if_fail (tag->table == table);

  /* Our little bad hack to be sure buffers don't still have the tag
   * applied to text in the buffer
   */
  tmp = table->buffers;
  while (tmp != NULL)
    {
      _btk_text_buffer_notify_will_remove_tag (BTK_TEXT_BUFFER (tmp->data),
                                               tag);
      
      tmp = tmp->next;
    }
  
  /* Set ourselves to the highest priority; this means
     when we're removed, there won't be any gaps in the
     priorities of the tags in the table. */
  btk_text_tag_set_priority (tag, btk_text_tag_table_get_size (table) - 1);

  tag->table = NULL;

  if (tag->name)
    g_hash_table_remove (table->hash, tag->name);
  else
    {
      table->anonymous = b_slist_remove (table->anonymous, tag);
      table->anon_count -= 1;
    }

  g_signal_emit (table, signals[TAG_REMOVED], 0, tag);

  g_object_unref (tag);
}

struct ForeachData
{
  BtkTextTagTableForeach func;
  bpointer data;
};

static void
hash_foreach (bpointer key, bpointer value, bpointer data)
{
  struct ForeachData *fd = data;

  g_return_if_fail (BTK_IS_TEXT_TAG (value));

  (* fd->func) (value, fd->data);
}

static void
list_foreach (bpointer data, bpointer user_data)
{
  struct ForeachData *fd = user_data;

  g_return_if_fail (BTK_IS_TEXT_TAG (data));

  (* fd->func) (data, fd->data);
}

/**
 * btk_text_tag_table_foreach:
 * @table: a #BtkTextTagTable
 * @func: (scope call): a function to call on each tag
 * @data: user data
 *
 * Calls @func on each tag in @table, with user data @data.
 * Note that the table may not be modified while iterating 
 * over it (you can't add/remove tags).
 **/
void
btk_text_tag_table_foreach (BtkTextTagTable       *table,
                            BtkTextTagTableForeach func,
                            bpointer               data)
{
  struct ForeachData d;

  g_return_if_fail (BTK_IS_TEXT_TAG_TABLE (table));
  g_return_if_fail (func != NULL);

  d.func = func;
  d.data = data;

  g_hash_table_foreach (table->hash, hash_foreach, &d);
  b_slist_foreach (table->anonymous, list_foreach, &d);
}

/**
 * btk_text_tag_table_get_size:
 * @table: a #BtkTextTagTable
 * 
 * Returns the size of the table (number of tags)
 * 
 * Return value: number of tags in @table
 **/
bint
btk_text_tag_table_get_size (BtkTextTagTable *table)
{
  g_return_val_if_fail (BTK_IS_TEXT_TAG_TABLE (table), 0);

  return g_hash_table_size (table->hash) + table->anon_count;
}

void
_btk_text_tag_table_add_buffer (BtkTextTagTable *table,
                                bpointer         buffer)
{
  g_return_if_fail (BTK_IS_TEXT_TAG_TABLE (table));

  table->buffers = b_slist_prepend (table->buffers, buffer);
}

static void
foreach_remove_tag (BtkTextTag *tag, bpointer data)
{
  BtkTextBuffer *buffer;

  buffer = BTK_TEXT_BUFFER (data);

  _btk_text_buffer_notify_will_remove_tag (buffer, tag);
}

void
_btk_text_tag_table_remove_buffer (BtkTextTagTable *table,
                                   bpointer         buffer)
{
  g_return_if_fail (BTK_IS_TEXT_TAG_TABLE (table));

  btk_text_tag_table_foreach (table, foreach_remove_tag, buffer);
  
  table->buffers = b_slist_remove (table->buffers, buffer);
}

#define __BTK_TEXT_TAG_TABLE_C__
#include "btkaliasdef.c"
