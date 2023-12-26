/* btkcomboboxentry.c
 * Copyright (C) 2002, 2003  Kristian Rietveld <kris@btk.org>
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

#include "config.h"
#include <string.h>

#undef BTK_DISABLE_DEPRECATED

#include "btkcomboboxentry.h"
#include "btkcelllayout.h"

#include "btkentry.h"
#include "btkcellrenderertext.h"

#include "btkprivate.h"
#include "btkintl.h"
#include "btkbuildable.h"
#include "btkalias.h"

#define BTK_COMBO_BOX_ENTRY_GET_PRIVATE(obj) (B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_COMBO_BOX_ENTRY, BtkComboBoxEntryPrivate))

struct _BtkComboBoxEntryPrivate
{
  BtkCellRenderer *text_renderer;
  bint text_column;
};

static void btk_combo_box_entry_set_property     (BObject               *object,
                                                  buint                  prop_id,
                                                  const BValue          *value,
                                                  BParamSpec            *pspec);
static void btk_combo_box_entry_get_property     (BObject               *object,
                                                  buint                  prop_id,
                                                  BValue                *value,
                                                  BParamSpec            *pspec);
static void btk_combo_box_entry_add              (BtkContainer          *container,
						  BtkWidget             *child);
static void btk_combo_box_entry_remove           (BtkContainer          *container,
						  BtkWidget             *child);

static bchar *btk_combo_box_entry_get_active_text (BtkComboBox *combo_box);
static void btk_combo_box_entry_active_changed   (BtkComboBox           *combo_box,
                                                  bpointer               user_data);
static void btk_combo_box_entry_contents_changed (BtkEntry              *entry,
                                                  bpointer               user_data);
static bboolean btk_combo_box_entry_mnemonic_activate (BtkWidget        *entry,
						       bboolean          group_cycling);
static void btk_combo_box_entry_grab_focus       (BtkWidget *widget);
static void has_frame_changed                    (BtkComboBoxEntry      *entry_box,
						  BParamSpec            *pspec,
						  bpointer               data);
static void btk_combo_box_entry_buildable_interface_init     (BtkBuildableIface *iface);
static BObject * btk_combo_box_entry_buildable_get_internal_child (BtkBuildable *buildable,
						     BtkBuilder   *builder,
						     const bchar  *childname);

enum
{
  PROP_0,
  PROP_TEXT_COLUMN
};

G_DEFINE_TYPE_WITH_CODE (BtkComboBoxEntry, btk_combo_box_entry, BTK_TYPE_COMBO_BOX,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
						btk_combo_box_entry_buildable_interface_init))

static void
btk_combo_box_entry_class_init (BtkComboBoxEntryClass *klass)
{
  BObjectClass *object_class;
  BtkWidgetClass *widget_class;
  BtkContainerClass *container_class;
  BtkComboBoxClass *combo_class;
  
  object_class = (BObjectClass *)klass;
  object_class->set_property = btk_combo_box_entry_set_property;
  object_class->get_property = btk_combo_box_entry_get_property;

  widget_class = (BtkWidgetClass *)klass;
  widget_class->mnemonic_activate = btk_combo_box_entry_mnemonic_activate;
  widget_class->grab_focus = btk_combo_box_entry_grab_focus;

  container_class = (BtkContainerClass *)klass;
  container_class->add = btk_combo_box_entry_add;
  container_class->remove = btk_combo_box_entry_remove;

  combo_class = (BtkComboBoxClass *)klass;
  combo_class->get_active_text = btk_combo_box_entry_get_active_text;
  
  g_object_class_install_property (object_class,
                                   PROP_TEXT_COLUMN,
                                   g_param_spec_int ("text-column",
                                                     P_("Text Column"),
                                                     P_("A column in the data source model to get the strings from"),
                                                     -1,
                                                     B_MAXINT,
                                                     -1,
                                                     BTK_PARAM_READWRITE));

  g_type_class_add_private ((BObjectClass *) klass,
                            sizeof (BtkComboBoxEntryPrivate));
}

static void
btk_combo_box_entry_init (BtkComboBoxEntry *entry_box)
{
  BtkWidget *entry;

  entry_box->priv = BTK_COMBO_BOX_ENTRY_GET_PRIVATE (entry_box);
  entry_box->priv->text_column = -1;

  entry = btk_entry_new ();
  btk_widget_show (entry);
  btk_container_add (BTK_CONTAINER (entry_box), entry);

  entry_box->priv->text_renderer = btk_cell_renderer_text_new ();
  btk_cell_layout_pack_start (BTK_CELL_LAYOUT (entry_box),
                              entry_box->priv->text_renderer, TRUE);

  btk_combo_box_set_active (BTK_COMBO_BOX (entry_box), -1);

  g_signal_connect (entry_box, "changed",
                    G_CALLBACK (btk_combo_box_entry_active_changed), NULL);
  g_signal_connect (entry_box, "notify::has-frame", G_CALLBACK (has_frame_changed), NULL);
}

static void
btk_combo_box_entry_buildable_interface_init (BtkBuildableIface *iface)
{
  iface->get_internal_child = btk_combo_box_entry_buildable_get_internal_child;
}

static BObject *
btk_combo_box_entry_buildable_get_internal_child (BtkBuildable *buildable,
						  BtkBuilder   *builder,
						  const bchar  *childname)
{
    if (strcmp (childname, "entry") == 0)
      return B_OBJECT (btk_bin_get_child (BTK_BIN (buildable)));

    return NULL;
}

static void
btk_combo_box_entry_set_property (BObject      *object,
                                  buint         prop_id,
                                  const BValue *value,
                                  BParamSpec   *pspec)
{
  BtkComboBoxEntry *entry_box = BTK_COMBO_BOX_ENTRY (object);

  switch (prop_id)
    {
      case PROP_TEXT_COLUMN:
        btk_combo_box_entry_set_text_column (entry_box,
                                             b_value_get_int (value));
        break;

      default:
	B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
btk_combo_box_entry_get_property (BObject    *object,
                                  buint       prop_id,
                                  BValue     *value,
                                  BParamSpec *pspec)
{
  BtkComboBoxEntry *entry_box = BTK_COMBO_BOX_ENTRY (object);

  switch (prop_id)
    {
      case PROP_TEXT_COLUMN:
        b_value_set_int (value, entry_box->priv->text_column);
        break;

      default:
        B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
btk_combo_box_entry_add (BtkContainer *container,
			 BtkWidget    *child)
{
  BtkComboBoxEntry *entry_box = BTK_COMBO_BOX_ENTRY (container);

  if (!BTK_IS_ENTRY (child))
    {
      g_warning ("Attempting to add a widget with type %s to a BtkComboBoxEntry "
		 "(need an instance of BtkEntry or of a subclass)",
                 B_OBJECT_TYPE_NAME (child));
      return;
    }

  BTK_CONTAINER_CLASS (btk_combo_box_entry_parent_class)->add (container, child);

  /* this flag is a hack to tell the entry to fill its allocation.
   */
  BTK_ENTRY (child)->is_cell_renderer = TRUE;

  g_signal_connect (child, "changed",
		    G_CALLBACK (btk_combo_box_entry_contents_changed),
		    entry_box);
  has_frame_changed (entry_box, NULL, NULL);
}

static void
btk_combo_box_entry_remove (BtkContainer *container,
			    BtkWidget    *child)
{
  if (child && child == BTK_BIN (container)->child)
    {
      g_signal_handlers_disconnect_by_func (child,
					    btk_combo_box_entry_contents_changed,
					    container);
      BTK_ENTRY (child)->is_cell_renderer = FALSE;
    }

  BTK_CONTAINER_CLASS (btk_combo_box_entry_parent_class)->remove (container, child);
}

static void
btk_combo_box_entry_active_changed (BtkComboBox *combo_box,
                                    bpointer     user_data)
{
  BtkComboBoxEntry *entry_box = BTK_COMBO_BOX_ENTRY (combo_box);
  BtkTreeModel *model;
  BtkTreeIter iter;
  bchar *str = NULL;

  if (btk_combo_box_get_active_iter (combo_box, &iter))
    {
      BtkEntry *entry = BTK_ENTRY (BTK_BIN (combo_box)->child);

      if (entry)
	{
	  g_signal_handlers_block_by_func (entry,
					   btk_combo_box_entry_contents_changed,
					   combo_box);

	  model = btk_combo_box_get_model (combo_box);

	  btk_tree_model_get (model, &iter, 
			      entry_box->priv->text_column, &str, 
			      -1);
	  btk_entry_set_text (entry, str);
	  g_free (str);

	  g_signal_handlers_unblock_by_func (entry,
					     btk_combo_box_entry_contents_changed,
					     combo_box);
	}
    }
}

static void 
has_frame_changed (BtkComboBoxEntry *entry_box,
		   BParamSpec       *pspec,
		   bpointer          data)
{
  if (BTK_BIN (entry_box)->child)
    {
      bboolean has_frame;
  
      g_object_get (entry_box, "has-frame", &has_frame, NULL);

      btk_entry_set_has_frame (BTK_ENTRY (BTK_BIN (entry_box)->child), has_frame);
    }
}

static void
btk_combo_box_entry_contents_changed (BtkEntry *entry,
                                      bpointer  user_data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (user_data);

  /*
   *  Fixes regression reported in bug #574059. The old functionality relied on
   *  bug #572478.  As a bugfix, we now emit the "changed" signal ourselves
   *  when the selection was already set to -1. 
   */
  if (btk_combo_box_get_active(combo_box) == -1)
    g_signal_emit_by_name (combo_box, "changed");
  else 
    btk_combo_box_set_active (combo_box, -1);
}

/* public API */

/**
 * btk_combo_box_entry_new:
 *
 * Creates a new #BtkComboBoxEntry which has a #BtkEntry as child. After
 * construction, you should set a model using btk_combo_box_set_model() and a
 * text column using btk_combo_box_entry_set_text_column().
 *
 * Return value: A new #BtkComboBoxEntry.
 *
 * Since: 2.4
 *
 * Deprecated: 2.24: Use btk_combo_box_new_with_entry() instead
 */
BtkWidget *
btk_combo_box_entry_new (void)
{
  return g_object_new (btk_combo_box_entry_get_type (), NULL);
}

/**
 * btk_combo_box_entry_new_with_model:
 * @model: A #BtkTreeModel.
 * @text_column: A column in @model to get the strings from.
 *
 * Creates a new #BtkComboBoxEntry which has a #BtkEntry as child and a list
 * of strings as popup. You can get the #BtkEntry from a #BtkComboBoxEntry
 * using BTK_ENTRY (BTK_BIN (combo_box_entry)->child). To add and remove
 * strings from the list, just modify @model using its data manipulation
 * API.
 *
 * Return value: A new #BtkComboBoxEntry.
 *
 * Since: 2.4
 *
 * Deprecated: 2.24: Use btk_combo_box_new_with_model_and_entry() instead
 */
BtkWidget *
btk_combo_box_entry_new_with_model (BtkTreeModel *model,
                                    bint          text_column)
{
  BtkWidget *ret;

  g_return_val_if_fail (BTK_IS_TREE_MODEL (model), NULL);
  g_return_val_if_fail (text_column >= 0, NULL);
  g_return_val_if_fail (text_column < btk_tree_model_get_n_columns (model), NULL);

  ret = g_object_new (btk_combo_box_entry_get_type (),
                      "model", model,
                      "text-column", text_column,
                      NULL);

  return ret;
}

/**
 * btk_combo_box_entry_set_text_column:
 * @entry_box: A #BtkComboBoxEntry.
 * @text_column: A column in @model to get the strings from.
 *
 * Sets the model column which @entry_box should use to get strings from
 * to be @text_column.
 *
 * Since: 2.4
 *
 * Deprecated: 2.24: Use btk_combo_box_set_entry_text_column() instead
 */
void
btk_combo_box_entry_set_text_column (BtkComboBoxEntry *entry_box,
                                     bint              text_column)
{
  BtkTreeModel *model = btk_combo_box_get_model (BTK_COMBO_BOX (entry_box));

  g_return_if_fail (text_column >= 0);
  g_return_if_fail (model == NULL || text_column < btk_tree_model_get_n_columns (model));

  entry_box->priv->text_column = text_column;

  btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (entry_box),
                                  entry_box->priv->text_renderer,
                                  "text", text_column,
                                  NULL);
}

/**
 * btk_combo_box_entry_get_text_column:
 * @entry_box: A #BtkComboBoxEntry.
 *
 * Returns the column which @entry_box is using to get the strings from.
 *
 * Return value: A column in the data source model of @entry_box.
 *
 * Since: 2.4
 *
 * Deprecated: 2.24: Use btk_combo_box_get_entry_text_column() instead
 */
bint
btk_combo_box_entry_get_text_column (BtkComboBoxEntry *entry_box)
{
  g_return_val_if_fail (BTK_IS_COMBO_BOX_ENTRY (entry_box), 0);

  return entry_box->priv->text_column;
}

static bboolean
btk_combo_box_entry_mnemonic_activate (BtkWidget *widget,
				       bboolean   group_cycling)
{
  BtkBin *entry_box = BTK_BIN (widget);

  if (entry_box->child)
    btk_widget_grab_focus (entry_box->child);

  return TRUE;
}

static void
btk_combo_box_entry_grab_focus (BtkWidget *widget)
{
  BtkBin *entry_box = BTK_BIN (widget);

  if (entry_box->child)
    btk_widget_grab_focus (entry_box->child);
}



/* convenience API for simple text combos */

/**
 * btk_combo_box_entry_new_text:
 *
 * Convenience function which constructs a new editable text combo box, which 
 * is a #BtkComboBoxEntry just displaying strings. If you use this function to
 * create a text combo box, you should only manipulate its data source with
 * the following convenience functions: btk_combo_box_append_text(),
 * btk_combo_box_insert_text(), btk_combo_box_prepend_text() and
 * btk_combo_box_remove_text().
 *
 * Return value: A new text #BtkComboBoxEntry.
 *
 * Since: 2.4
 */
BtkWidget *
btk_combo_box_entry_new_text (void)
{
  BtkWidget *entry_box;
  BtkListStore *store;

  store = btk_list_store_new (1, B_TYPE_STRING);
  entry_box = btk_combo_box_entry_new_with_model (BTK_TREE_MODEL (store), 0);
  g_object_unref (store);

  return entry_box;
}

static bchar *
btk_combo_box_entry_get_active_text (BtkComboBox *combo_box)
{
  BtkBin *combo = BTK_BIN (combo_box);

  if (combo->child)
    return g_strdup (btk_entry_get_text (BTK_ENTRY (combo->child)));

  return NULL;
}

#define __BTK_COMBO_BOX_ENTRY_C__
#include "btkaliasdef.c"
