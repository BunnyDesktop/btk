/* BtkCellRendererCombo
 * Copyright (C) 2004 Lorenzo Gil Sanchez
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
#include <string.h>

#include "btkintl.h"
#include "btkbin.h"
#include "btkentry.h"
#include "btkcelllayout.h"
#include "btkcellrenderercombo.h"
#include "btkcellrenderertext.h"
#include "btkcombobox.h"
#include "btkmarshalers.h"
#include "btkprivate.h"
#include "btkalias.h"


#define BTK_CELL_RENDERER_COMBO_GET_PRIVATE(obj) (B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_CELL_RENDERER_COMBO, BtkCellRendererComboPrivate))

typedef struct _BtkCellRendererComboPrivate BtkCellRendererComboPrivate;
struct _BtkCellRendererComboPrivate
{
  BtkWidget *combo;
};


static void btk_cell_renderer_combo_class_init (BtkCellRendererComboClass *klass);
static void btk_cell_renderer_combo_init       (BtkCellRendererCombo      *self);
static void btk_cell_renderer_combo_finalize     (BObject      *object);
static void btk_cell_renderer_combo_get_property (BObject      *object,
						  buint         prop_id,
						  BValue       *value,
						  BParamSpec   *pspec);

static void btk_cell_renderer_combo_set_property (BObject      *object,
						  buint         prop_id,
						  const BValue *value,
						  BParamSpec   *pspec);

static BtkCellEditable *btk_cell_renderer_combo_start_editing (BtkCellRenderer     *cell,
							       BdkEvent            *event,
							       BtkWidget           *widget,
							       const bchar         *path,
							       BdkRectangle        *background_area,
							       BdkRectangle        *cell_area,
							       BtkCellRendererState flags);

enum {
  PROP_0,
  PROP_MODEL,
  PROP_TEXT_COLUMN,
  PROP_HAS_ENTRY
};

enum {
  CHANGED,
  LAST_SIGNAL
};

static buint cell_renderer_combo_signals[LAST_SIGNAL] = { 0, };

#define BTK_CELL_RENDERER_COMBO_PATH "btk-cell-renderer-combo-path"

G_DEFINE_TYPE (BtkCellRendererCombo, btk_cell_renderer_combo, BTK_TYPE_CELL_RENDERER_TEXT)

static void
btk_cell_renderer_combo_class_init (BtkCellRendererComboClass *klass)
{
  BObjectClass *object_class = B_OBJECT_CLASS (klass);
  BtkCellRendererClass *cell_class = BTK_CELL_RENDERER_CLASS (klass);

  object_class->finalize = btk_cell_renderer_combo_finalize;
  object_class->get_property = btk_cell_renderer_combo_get_property;
  object_class->set_property = btk_cell_renderer_combo_set_property;

  cell_class->start_editing = btk_cell_renderer_combo_start_editing;

  /**
   * BtkCellRendererCombo:model:
   *
   * Holds a tree model containing the possible values for the combo box. 
   * Use the text_column property to specify the column holding the values.
   * 
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
				   PROP_MODEL,
				   g_param_spec_object ("model",
							P_("Model"),
							P_("The model containing the possible values for the combo box"),
							BTK_TYPE_TREE_MODEL,
							BTK_PARAM_READWRITE));

  /**
   * BtkCellRendererCombo:text-column:
   *
   * Specifies the model column which holds the possible values for the 
   * combo box. 
   *
   * Note that this refers to the model specified in the model property, 
   * <emphasis>not</emphasis> the model backing the tree view to which 
   * this cell renderer is attached.
   * 
   * #BtkCellRendererCombo automatically adds a text cell renderer for 
   * this column to its combo box.
   *
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
                                   PROP_TEXT_COLUMN,
                                   g_param_spec_int ("text-column",
                                                     P_("Text Column"),
                                                     P_("A column in the data source model to get the strings from"),
                                                     -1,
                                                     B_MAXINT,
                                                     -1,
                                                     BTK_PARAM_READWRITE));

  /** 
   * BtkCellRendererCombo:has-entry:
   *
   * If %TRUE, the cell renderer will include an entry and allow to enter 
   * values other than the ones in the popup list. 
   *
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
                                   PROP_HAS_ENTRY,
                                   g_param_spec_boolean ("has-entry",
							 P_("Has Entry"),
							 P_("If FALSE, don't allow to enter strings other than the chosen ones"),
							 TRUE,
							 BTK_PARAM_READWRITE));


  /**
   * BtkCellRendererCombo::changed:
   * @combo: the object on which the signal is emitted
   * @path_string: a string of the path identifying the edited cell
   *               (relative to the tree view model)
   * @new_iter: the new iter selected in the combo box
   *            (relative to the combo box model)
   *
   * This signal is emitted each time after the user selected an item in
   * the combo box, either by using the mouse or the arrow keys.  Contrary
   * to BtkComboBox, BtkCellRendererCombo::changed is not emitted for
   * changes made to a selected item in the entry.  The argument @new_iter
   * corresponds to the newly selected item in the combo box and it is relative
   * to the BtkTreeModel set via the model property on BtkCellRendererCombo.
   *
   * Note that as soon as you change the model displayed in the tree view,
   * the tree view will immediately cease the editing operating.  This
   * means that you most probably want to refrain from changing the model
   * until the combo cell renderer emits the edited or editing_canceled signal.
   *
   * Since: 2.14
   */
  cell_renderer_combo_signals[CHANGED] =
    g_signal_new (I_("changed"),
		  B_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  0,
		  NULL, NULL,
		  _btk_marshal_VOID__STRING_BOXED,
		  B_TYPE_NONE, 2,
		  B_TYPE_STRING,
		  BTK_TYPE_TREE_ITER);

  g_type_class_add_private (klass, sizeof (BtkCellRendererComboPrivate));
}

static void
btk_cell_renderer_combo_init (BtkCellRendererCombo *self)
{
  self->model = NULL;
  self->text_column = -1;
  self->has_entry = TRUE;
  self->focus_out_id = 0;
}

/**
 * btk_cell_renderer_combo_new: 
 * 
 * Creates a new #BtkCellRendererCombo. 
 * Adjust how text is drawn using object properties. 
 * Object properties can be set globally (with g_object_set()). 
 * Also, with #BtkTreeViewColumn, you can bind a property to a value 
 * in a #BtkTreeModel. For example, you can bind the "text" property 
 * on the cell renderer to a string value in the model, thus rendering 
 * a different string in each row of the #BtkTreeView.
 * 
 * Returns: the new cell renderer
 *
 * Since: 2.6
 */
BtkCellRenderer *
btk_cell_renderer_combo_new (void)
{
  return g_object_new (BTK_TYPE_CELL_RENDERER_COMBO, NULL); 
}

static void
btk_cell_renderer_combo_finalize (BObject *object)
{
  BtkCellRendererCombo *cell = BTK_CELL_RENDERER_COMBO (object);
  
  if (cell->model)
    {
      g_object_unref (cell->model);
      cell->model = NULL;
    }
  
  B_OBJECT_CLASS (btk_cell_renderer_combo_parent_class)->finalize (object);
}

static void
btk_cell_renderer_combo_get_property (BObject    *object,
				      buint       prop_id,
				      BValue     *value,
				      BParamSpec *pspec)
{
  BtkCellRendererCombo *cell = BTK_CELL_RENDERER_COMBO (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      b_value_set_object (value, cell->model);
      break; 
    case PROP_TEXT_COLUMN:
      b_value_set_int (value, cell->text_column);
      break;
    case PROP_HAS_ENTRY:
      b_value_set_boolean (value, cell->has_entry);
      break;
   default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_cell_renderer_combo_set_property (BObject      *object,
				      buint         prop_id,
				      const BValue *value,
				      BParamSpec   *pspec)
{
  BtkCellRendererCombo *cell = BTK_CELL_RENDERER_COMBO (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      {
        BtkCellRendererComboPrivate *priv;

        priv = BTK_CELL_RENDERER_COMBO_GET_PRIVATE (cell);

        if (cell->model)
          g_object_unref (cell->model);
        cell->model = BTK_TREE_MODEL (b_value_get_object (value));
        if (cell->model)
          g_object_ref (cell->model);
        break;
      }
    case PROP_TEXT_COLUMN:
      cell->text_column = b_value_get_int (value);
      break;
    case PROP_HAS_ENTRY:
      cell->has_entry = b_value_get_boolean (value);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_cell_renderer_combo_changed (BtkComboBox *combo,
				 bpointer     data)
{
  BtkTreeIter iter;
  BtkCellRendererCombo *cell;

  cell = BTK_CELL_RENDERER_COMBO (data);

  if (btk_combo_box_get_active_iter (combo, &iter))
    {
      const char *path;

      path = g_object_get_data (B_OBJECT (combo), BTK_CELL_RENDERER_COMBO_PATH);
      g_signal_emit (cell, cell_renderer_combo_signals[CHANGED], 0,
		     path, &iter);
    }
}

static void
btk_cell_renderer_combo_editing_done (BtkCellEditable *combo,
				      bpointer         data)
{
  const bchar *path;
  bchar *new_text = NULL;
  BtkTreeModel *model;
  BtkTreeIter iter;
  BtkCellRendererCombo *cell;
  BtkEntry *entry;
  bboolean canceled;
  BtkCellRendererComboPrivate *priv;

  cell = BTK_CELL_RENDERER_COMBO (data);
  priv = BTK_CELL_RENDERER_COMBO_GET_PRIVATE (data);

  if (cell->focus_out_id > 0)
    {
      g_signal_handler_disconnect (combo, cell->focus_out_id);
      cell->focus_out_id = 0;
    }

  g_object_get (combo,
                "editing-canceled", &canceled,
                NULL);
  btk_cell_renderer_stop_editing (BTK_CELL_RENDERER (data), canceled);
  if (canceled)
    {
      priv->combo = NULL;
      return;
    }

  if (btk_combo_box_get_has_entry (BTK_COMBO_BOX (combo)))
    {
      entry = BTK_ENTRY (btk_bin_get_child (BTK_BIN (combo)));
      new_text = g_strdup (btk_entry_get_text (entry));
    }
  else 
    {
      model = btk_combo_box_get_model (BTK_COMBO_BOX (combo));

      if (model
          && btk_combo_box_get_active_iter (BTK_COMBO_BOX (combo), &iter))
        btk_tree_model_get (model, &iter, cell->text_column, &new_text, -1);
    }

  path = g_object_get_data (B_OBJECT (combo), BTK_CELL_RENDERER_COMBO_PATH);
  g_signal_emit_by_name (cell, "edited", path, new_text);

  priv->combo = NULL;

  g_free (new_text);
}

static bboolean
btk_cell_renderer_combo_focus_out_event (BtkWidget *widget,
					 BdkEvent  *event,
					 bpointer   data)
{
  
  btk_cell_renderer_combo_editing_done (BTK_CELL_EDITABLE (widget), data);

  return FALSE;
}

typedef struct 
{
  BtkCellRendererCombo *cell;
  bboolean found;
  BtkTreeIter iter;
} SearchData;

static bboolean 
find_text (BtkTreeModel *model, 
	   BtkTreePath  *path, 
	   BtkTreeIter  *iter, 
	   bpointer      data)
{
  SearchData *search_data = (SearchData *)data;
  bchar *text;
  
  btk_tree_model_get (model, iter, search_data->cell->text_column, &text, -1);
  if (text && BTK_CELL_RENDERER_TEXT (search_data->cell)->text &&
      strcmp (text, BTK_CELL_RENDERER_TEXT (search_data->cell)->text) == 0)
    {
      search_data->iter = *iter;
      search_data->found = TRUE;
    }

  g_free (text);
  
  return search_data->found;
}

static BtkCellEditable *
btk_cell_renderer_combo_start_editing (BtkCellRenderer     *cell,
				       BdkEvent            *event,
				       BtkWidget           *widget,
				       const bchar         *path,
				       BdkRectangle        *background_area,
				       BdkRectangle        *cell_area,
				       BtkCellRendererState flags)
{
  BtkCellRendererCombo *cell_combo;
  BtkCellRendererText *cell_text;
  BtkWidget *combo;
  SearchData search_data;
  BtkCellRendererComboPrivate *priv;

  cell_text = BTK_CELL_RENDERER_TEXT (cell);
  if (cell_text->editable == FALSE)
    return NULL;

  cell_combo = BTK_CELL_RENDERER_COMBO (cell);
  if (cell_combo->text_column < 0)
    return NULL;

  priv = BTK_CELL_RENDERER_COMBO_GET_PRIVATE (cell_combo);

  if (cell_combo->has_entry) 
    {
      combo = g_object_new (BTK_TYPE_COMBO_BOX, "has-entry", TRUE, NULL);

      if (cell_combo->model)
        btk_combo_box_set_model (BTK_COMBO_BOX (combo), cell_combo->model);
      btk_combo_box_set_entry_text_column (BTK_COMBO_BOX (combo),
                                           cell_combo->text_column);

      if (cell_text->text)
	btk_entry_set_text (BTK_ENTRY (BTK_BIN (combo)->child), 
			    cell_text->text);
    }
  else
    {
      cell = btk_cell_renderer_text_new ();

      combo = btk_combo_box_new ();
      if (cell_combo->model)
        btk_combo_box_set_model (BTK_COMBO_BOX (combo), cell_combo->model);

      btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combo), cell, TRUE);
      btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (combo), 
				      cell, "text", cell_combo->text_column, 
				      NULL);

      /* determine the current value */
      if (cell_combo->model)
        {
          search_data.cell = cell_combo;
          search_data.found = FALSE;
          btk_tree_model_foreach (cell_combo->model, find_text, &search_data);
          if (search_data.found)
            btk_combo_box_set_active_iter (BTK_COMBO_BOX (combo),
                                           &(search_data.iter));
        }
    }

  g_object_set (combo, "has-frame", FALSE, NULL);
  g_object_set_data_full (B_OBJECT (combo),
			  I_(BTK_CELL_RENDERER_COMBO_PATH),
			  g_strdup (path), g_free);

  btk_widget_show (combo);

  g_signal_connect (BTK_CELL_EDITABLE (combo), "editing-done",
		    G_CALLBACK (btk_cell_renderer_combo_editing_done),
		    cell_combo);
  g_signal_connect (BTK_CELL_EDITABLE (combo), "changed",
		    G_CALLBACK (btk_cell_renderer_combo_changed),
		    cell_combo);
  cell_combo->focus_out_id = 
    g_signal_connect (combo, "focus-out-event",
		      G_CALLBACK (btk_cell_renderer_combo_focus_out_event),
		      cell_combo);

  priv->combo = combo;

  return BTK_CELL_EDITABLE (combo);
}

#define __BTK_CELL_RENDERER_COMBO_C__
#include "btkaliasdef.c"
