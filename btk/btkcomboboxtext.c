/* BTK - The GIMP Toolkit
 *
 * Copyright (C) 2010 Christian Dywan
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
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>

#include "config.h"

#include "btkcomboboxtext.h"
#include "btkcombobox.h"
#include "btkcellrenderertext.h"
#include "btkcelllayout.h"
#include "btkbuildable.h"
#include "btkbuilderprivate.h"
#include "btkalias.h"

/**
 * SECTION:btkcomboboxtext
 * @Short_description: A simple, text-only combo box
 * @Title: BtkComboBoxText
 * @See_also: @BtkComboBox
 *
 * A BtkComboBoxText is a simple variant of #BtkComboBox that hides
 * the model-view complexity for simple text-only use cases.
 *
 * To create a BtkComboBoxText, use btk_combo_box_text_new() or
 * btk_combo_box_text_new_with_entry().
 *
 * You can add items to a BtkComboBoxText with
 * btk_combo_box_text_append_text(), btk_combo_box_text_insert_text()
 * or btk_combo_box_text_prepend_text() and remove options with
 * btk_combo_box_text_remove().
 *
 * If the BtkComboBoxText contains an entry (via the 'has-entry' property),
 * its contents can be retrieved using btk_combo_box_text_get_active_text().
 * The entry itself can be accessed by calling btk_bin_get_child() on the
 * combo box.
 *
 * <refsect2 id="BtkComboBoxText-BUILDER-UI">
 * <title>BtkComboBoxText as BtkBuildable</title>
 * <para>
 * The BtkComboBoxText implementation of the BtkBuildable interface
 * supports adding items directly using the &lt;items&gt element
 * and specifying &lt;item&gt; elements for each item. Each &lt;item&gt;
 * element supports the regular translation attributes "translatable",
 * "context" and "comments".
 * </para>
 * <example>
 * <title>A UI definition fragment specifying BtkComboBoxText items</title>
 * <programlisting><![CDATA[
 * <object class="BtkComboBoxText">
 *   <items>
 *     <item translatable="yes">Factory</item>
 *     <item translatable="yes">Home</item>
 *     <item translatable="yes">Subway</item>
 *   </items>
 * </object>
 * ]]></programlisting>
 * </example>
 * </refsect2>
 */

static void     btk_combo_box_text_buildable_interface_init     (BtkBuildableIface *iface);
static bboolean btk_combo_box_text_buildable_custom_tag_start   (BtkBuildable     *buildable,
								 BtkBuilder       *builder,
								 BObject          *child,
								 const bchar      *tagname,
								 GMarkupParser    *parser,
								 bpointer         *data);

static void     btk_combo_box_text_buildable_custom_finished    (BtkBuildable     *buildable,
								 BtkBuilder       *builder,
								 BObject          *child,
								 const bchar      *tagname,
								 bpointer          user_data);

static BtkBuildableIface *buildable_parent_iface = NULL;

G_DEFINE_TYPE_WITH_CODE (BtkComboBoxText, btk_combo_box_text, BTK_TYPE_COMBO_BOX,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
						btk_combo_box_text_buildable_interface_init));

static BObject *
btk_combo_box_text_constructor (GType                  type,
                                buint                  n_construct_properties,
                                BObjectConstructParam *construct_properties)
{
  BObject            *object;

  object = B_OBJECT_CLASS (btk_combo_box_text_parent_class)->constructor
    (type, n_construct_properties, construct_properties);

  if (!btk_combo_box_get_has_entry (BTK_COMBO_BOX (object)))
    {
      BtkCellRenderer *cell;

      cell = btk_cell_renderer_text_new ();
      btk_cell_layout_pack_start (BTK_CELL_LAYOUT (object), cell, TRUE);
      btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (object), cell,
                                      "text", 0,
                                      NULL);
    }

  return object;
}

static void
btk_combo_box_text_init (BtkComboBoxText *combo_box)
{
  BtkListStore *store;

  store = btk_list_store_new (1, B_TYPE_STRING);
  btk_combo_box_set_model (BTK_COMBO_BOX (combo_box), BTK_TREE_MODEL (store));
  g_object_unref (store);
}

static void
btk_combo_box_text_class_init (BtkComboBoxTextClass *klass)
{
  BObjectClass *object_class;

  object_class = (BObjectClass *)klass;
  object_class->constructor = btk_combo_box_text_constructor;
}

static void
btk_combo_box_text_buildable_interface_init (BtkBuildableIface *iface)
{
  buildable_parent_iface = g_type_interface_peek_parent (iface);

  iface->custom_tag_start = btk_combo_box_text_buildable_custom_tag_start;
  iface->custom_finished = btk_combo_box_text_buildable_custom_finished;
}

typedef struct {
  BtkBuilder    *builder;
  BObject       *object;
  const bchar   *domain;

  bchar         *context;
  bchar         *string;
  buint          translatable : 1;

  buint          is_text : 1;
} ItemParserData;

static void
item_start_element (GMarkupParseContext *context,
		    const bchar         *element_name,
		    const bchar        **names,
		    const bchar        **values,
		    bpointer             user_data,
		    GError             **error)
{
  ItemParserData *data = (ItemParserData*)user_data;
  buint i;

  if (strcmp (element_name, "item") == 0)
    {
      data->is_text = TRUE;

      for (i = 0; names[i]; i++)
	{
	  if (strcmp (names[i], "translatable") == 0)
	    {
	      bboolean bval;

	      if (!_btk_builder_boolean_from_string (values[i], &bval,
						     error))
		return;

	      data->translatable = bval;
	    }
	  else if (strcmp (names[i], "comments") == 0)
	    {
	      /* do nothing, comments are for translators */
	    }
	  else if (strcmp (names[i], "context") == 0) 
	    data->context = g_strdup (values[i]);
	  else
	    g_warning ("Unknown custom combo box item attribute: %s", names[i]);
	}
    }
}

static void
item_text (GMarkupParseContext *context,
	   const bchar         *text,
	   bsize                text_len,
	   bpointer             user_data,
	   GError             **error)
{
  ItemParserData *data = (ItemParserData*)user_data;
  bchar *string;

  if (!data->is_text)
    return;

  string = g_strndup (text, text_len);

  if (data->translatable && text_len)
    {
      bchar *translated;

      /* FIXME: This will not use the domain set in the .ui file,
       * since the parser is not telling the builder about the domain.
       * However, it will work for btk_builder_set_translation_domain() calls.
       */
      translated = _btk_builder_parser_translate (data->domain,
						  data->context,
						  string);
      g_free (string);
      string = translated;
    }

  data->string = string;
}

static void
item_end_element (GMarkupParseContext *context,
		  const bchar         *element_name,
		  bpointer             user_data,
		  GError             **error)
{
  ItemParserData *data = (ItemParserData*)user_data;

  /* Append the translated strings */
  if (data->string)
    btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (data->object), data->string);

  data->translatable = FALSE;
  g_free (data->context);
  g_free (data->string);
  data->context = NULL;
  data->string = NULL;
  data->is_text = FALSE;
}

static const GMarkupParser item_parser =
  {
    item_start_element,
    item_end_element,
    item_text
  };

static bboolean
btk_combo_box_text_buildable_custom_tag_start (BtkBuildable     *buildable,
					       BtkBuilder       *builder,
					       BObject          *child,
					       const bchar      *tagname,
					       GMarkupParser    *parser,
					       bpointer         *data)
{
  if (buildable_parent_iface->custom_tag_start (buildable, builder, child, 
						tagname, parser, data))
    return TRUE;

  if (strcmp (tagname, "items") == 0)
    {
      ItemParserData *parser_data;

      parser_data = g_slice_new0 (ItemParserData);
      parser_data->builder = g_object_ref (builder);
      parser_data->object = g_object_ref (buildable);
      parser_data->domain = btk_builder_get_translation_domain (builder);
      *parser = item_parser;
      *data = parser_data;
      return TRUE;
    }
  return FALSE;
}

static void
btk_combo_box_text_buildable_custom_finished (BtkBuildable *buildable,
					      BtkBuilder   *builder,
					      BObject      *child,
					      const bchar  *tagname,
					      bpointer      user_data)
{
  ItemParserData *data;

  buildable_parent_iface->custom_finished (buildable, builder, child, 
					   tagname, user_data);

  if (strcmp (tagname, "items") == 0)
    {
      data = (ItemParserData*)user_data;

      g_object_unref (data->object);
      g_object_unref (data->builder);
      g_slice_free (ItemParserData, data);
    }
}

/**
 * btk_combo_box_text_new:
 *
 * Creates a new #BtkComboBoxText, which is a #BtkComboBox just displaying
 * strings. See btk_combo_box_entry_new_with_text().
 *
 * Return value: A new #BtkComboBoxText
 *
 * Since: 2.24
 */
BtkWidget *
btk_combo_box_text_new (void)
{
  return g_object_new (BTK_TYPE_COMBO_BOX_TEXT,
                       "entry-text-column", 0,
                       NULL);
}

/**
 * btk_combo_box_text_new_with_entry:
 *
 * Creates a new #BtkComboBoxText, which is a #BtkComboBox just displaying
 * strings. The combo box created by this function has an entry.
 *
 * Return value: a new #BtkComboBoxText
 *
 * Since: 2.24
 */
BtkWidget *
btk_combo_box_text_new_with_entry (void)
{
  return g_object_new (BTK_TYPE_COMBO_BOX_TEXT,
                       "has-entry", TRUE,
                       "entry-text-column", 0,
                       NULL);
}

/**
 * btk_combo_box_text_append_text:
 * @combo_box: A #BtkComboBoxText
 * @text: A string
 *
 * Appends @string to the list of strings stored in @combo_box.
 *
 * Since: 2.24
 */
void
btk_combo_box_text_append_text (BtkComboBoxText *combo_box,
                                const bchar     *text)
{
  BtkListStore *store;
  BtkTreeIter iter;
  bint text_column;
  bint column_type;

  g_return_if_fail (BTK_IS_COMBO_BOX_TEXT (combo_box));
  g_return_if_fail (text != NULL);

  store = BTK_LIST_STORE (btk_combo_box_get_model (BTK_COMBO_BOX (combo_box)));
  g_return_if_fail (BTK_IS_LIST_STORE (store));

  text_column = btk_combo_box_get_entry_text_column (BTK_COMBO_BOX (combo_box));
  if (btk_combo_box_get_has_entry (BTK_COMBO_BOX (combo_box)))
    g_return_if_fail (text_column >= 0);
  else if (text_column < 0)
    text_column = 0;

  column_type = btk_tree_model_get_column_type (BTK_TREE_MODEL (store), text_column);
  g_return_if_fail (column_type == B_TYPE_STRING);

  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, text_column, text, -1);
}

/**
 * btk_combo_box_text_insert_text:
 * @combo_box: A #BtkComboBoxText
 * @position: An index to insert @text
 * @text: A string
 *
 * Inserts @string at @position in the list of strings stored in @combo_box.
 *
 * Since: 2.24
 */
void
btk_combo_box_text_insert_text (BtkComboBoxText *combo_box,
                                bint             position,
                                const bchar     *text)
{
  BtkListStore *store;
  BtkTreeIter iter;
  bint text_column;
  bint column_type;

  g_return_if_fail (BTK_IS_COMBO_BOX_TEXT (combo_box));
  g_return_if_fail (position >= 0);
  g_return_if_fail (text != NULL);

  store = BTK_LIST_STORE (btk_combo_box_get_model (BTK_COMBO_BOX (combo_box)));
  g_return_if_fail (BTK_IS_LIST_STORE (store));
  text_column = btk_combo_box_get_entry_text_column (BTK_COMBO_BOX (combo_box));
  column_type = btk_tree_model_get_column_type (BTK_TREE_MODEL (store), text_column);
  g_return_if_fail (column_type == B_TYPE_STRING);

  btk_list_store_insert (store, &iter, position);
  btk_list_store_set (store, &iter, text_column, text, -1);
}

/**
 * btk_combo_box_text_prepend_text:
 * @combo_box: A #BtkComboBox
 * @text: A string
 *
 * Prepends @string to the list of strings stored in @combo_box.
 *
 * Since: 2.24
 */
void
btk_combo_box_text_prepend_text (BtkComboBoxText *combo_box,
                                 const bchar     *text)
{
  BtkListStore *store;
  BtkTreeIter iter;
  bint text_column;
  bint column_type;

  g_return_if_fail (BTK_IS_COMBO_BOX_TEXT (combo_box));
  g_return_if_fail (text != NULL);

  store = BTK_LIST_STORE (btk_combo_box_get_model (BTK_COMBO_BOX (combo_box)));
  g_return_if_fail (BTK_IS_LIST_STORE (store));

  text_column = btk_combo_box_get_entry_text_column (BTK_COMBO_BOX (combo_box));
  column_type = btk_tree_model_get_column_type (BTK_TREE_MODEL (store), text_column);
  g_return_if_fail (column_type == B_TYPE_STRING);

  btk_list_store_prepend (store, &iter);
  btk_list_store_set (store, &iter, text_column, text, -1);
}

/**
 * btk_combo_box_text_remove:
 * @combo_box: A #BtkComboBox
 * @position: Index of the item to remove
 *
 * Removes the string at @position from @combo_box.
 *
 * Since: 2.24
 */
void
btk_combo_box_text_remove (BtkComboBoxText *combo_box,
                           bint             position)
{
  BtkTreeModel *model;
  BtkListStore *store;
  BtkTreeIter iter;

  g_return_if_fail (BTK_IS_COMBO_BOX_TEXT (combo_box));
  g_return_if_fail (position >= 0);

  model = btk_combo_box_get_model (BTK_COMBO_BOX (combo_box));
  store = BTK_LIST_STORE (model);
  g_return_if_fail (BTK_IS_LIST_STORE (store));

  if (btk_tree_model_iter_nth_child (model, &iter, NULL, position))
    btk_list_store_remove (store, &iter);
}

/**
 * btk_combo_box_text_get_active_text:
 * @combo_box: A #BtkComboBoxText
 *
 * Returns the currently active string in @combo_box, or %NULL
 * if none is selected. If @combo_box contains an entry, this
 * function will return its contents (which will not necessarily
 * be an item from the list).
 *
 * Returns: a newly allocated string containing the currently
 *     active text. Must be freed with g_free().
 *
 * Since: 2.24
 */
bchar *
btk_combo_box_text_get_active_text (BtkComboBoxText *combo_box)
{
  BtkTreeIter iter;
  bchar *text = NULL;

  g_return_val_if_fail (BTK_IS_COMBO_BOX_TEXT (combo_box), NULL);

 if (btk_combo_box_get_has_entry (BTK_COMBO_BOX (combo_box)))
   {
     BtkWidget *entry;

     entry = btk_bin_get_child (BTK_BIN (combo_box));
     text = g_strdup (btk_entry_get_text (BTK_ENTRY (entry)));
   }
  else if (btk_combo_box_get_active_iter (BTK_COMBO_BOX (combo_box), &iter))
    {
      BtkTreeModel *model;
      bint text_column;
      bint column_type;

      model = btk_combo_box_get_model (BTK_COMBO_BOX (combo_box));
      g_return_val_if_fail (BTK_IS_LIST_STORE (model), NULL);
      text_column = btk_combo_box_get_entry_text_column (BTK_COMBO_BOX (combo_box));
      column_type = btk_tree_model_get_column_type (model, text_column);
      g_return_val_if_fail (column_type == B_TYPE_STRING, NULL);
      btk_tree_model_get (model, &iter, text_column, &text, -1);
    }

  return text;
}

#define __BTK_COMBO_BOX_TEXT_C__
#include "btkaliasdef.c"
