/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2001 Sun Microsystems Inc.
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

#include <stdio.h>

#undef BTK_DISABLE_DEPRECATED

#include <btk/btk.h>
#include "bailclist.h"
#include "bailclistcell.h"
#include "bailcellparent.h"

/* Copied from btkclist.c */
/* this defigns the base grid spacing */
#define CELL_SPACING 1

/* added the horizontal space at the beginning and end of a row*/
#define COLUMN_INSET 3


/* gives the top pixel of the given row in context of
 * the clist's voffset */
#define ROW_TOP_YPIXEL(clist, row) (((clist)->row_height * (row)) + \
                                    (((row) + 1) * CELL_SPACING) + \
                                    (clist)->voffset)

/* returns the row index from a y pixel location in the
 * context of the clist's voffset */
#define ROW_FROM_YPIXEL(clist, y)  (((y) - (clist)->voffset) / \
                                    ((clist)->row_height + CELL_SPACING))
/* gives the left pixel of the given column in context of
 * the clist's hoffset */
#define COLUMN_LEFT_XPIXEL(clist, colnum)  ((clist)->column[(colnum)].area.x + \
                                            (clist)->hoffset)

/* returns the column index from a x pixel location in the
 * context of the clist's hoffset */
static inline bint
COLUMN_FROM_XPIXEL (BtkCList * clist,
                    bint x)
{
  bint i, cx;

  for (i = 0; i < clist->columns; i++)
    if (clist->column[i].visible)
      {
        cx = clist->column[i].area.x + clist->hoffset;

        if (x >= (cx - (COLUMN_INSET + CELL_SPACING)) &&
            x <= (cx + clist->column[i].area.width + COLUMN_INSET))
          return i;
      }

  /* no match */
  return -1;
}

/* returns the top pixel of the given row in the context of
 * the list height */
#define ROW_TOP(clist, row)        (((clist)->row_height + CELL_SPACING) * (row))

/* returns the left pixel of the given column in the context of
 * the list width */
#define COLUMN_LEFT(clist, colnum) ((clist)->column[(colnum)].area.x)

/* returns the total height of the list */
#define LIST_HEIGHT(clist)         (((clist)->row_height * ((clist)->rows)) + \
                                    (CELL_SPACING * ((clist)->rows + 1)))

static inline bint
LIST_WIDTH (BtkCList * clist)
{
  bint last_column;

  for (last_column = clist->columns - 1;
       last_column >= 0 && !clist->column[last_column].visible; last_column--);

  if (last_column >= 0)
    return (clist->column[last_column].area.x +
            clist->column[last_column].area.width +
            COLUMN_INSET + CELL_SPACING);
  return 0;
}

/* returns the GList item for the nth row */
#define ROW_ELEMENT(clist, row) (((row) == (clist)->rows - 1) ? \
                                 (clist)->row_list_end : \
                                 g_list_nth ((clist)->row_list, (row)))

typedef struct _BailCListRow        BailCListRow;
typedef struct _BailCListCellData   BailCListCellData;


static void       bail_clist_class_init            (BailCListClass    *klass);
static void       bail_clist_init                  (BailCList         *clist);
static void       bail_clist_real_initialize       (BatkObject         *obj,
                                                    bpointer          data);
static void       bail_clist_finalize              (BObject           *object);

static bint       bail_clist_get_n_children        (BatkObject         *obj);
static BatkObject* bail_clist_ref_child             (BatkObject         *obj,
                                                    bint              i);
static BatkStateSet* bail_clist_ref_state_set       (BatkObject         *obj);


static void       batk_selection_interface_init     (BatkSelectionIface *iface);
static bboolean   bail_clist_clear_selection       (BatkSelection   *selection);

static BatkObject* bail_clist_ref_selection         (BatkSelection   *selection,
                                                    bint           i);
static bint       bail_clist_get_selection_count   (BatkSelection   *selection);
static bboolean   bail_clist_is_child_selected     (BatkSelection   *selection,
                                                    bint           i);
static bboolean   bail_clist_select_all_selection  (BatkSelection   *selection);

static void       batk_table_interface_init         (BatkTableIface     *iface);
static bint       bail_clist_get_index_at          (BatkTable      *table,
                                                    bint          row,
                                                    bint          column);
static bint       bail_clist_get_column_at_index   (BatkTable      *table,
                                                    bint          index);
static bint       bail_clist_get_row_at_index      (BatkTable      *table,
                                                    bint          index);
static BatkObject* bail_clist_ref_at                (BatkTable      *table,
                                                    bint          row,
                                                    bint          column);
static BatkObject* bail_clist_ref_at_actual         (BatkTable      *table,
                                                    bint          row,
                                                    bint          column);
static BatkObject*
                  bail_clist_get_caption           (BatkTable      *table);

static bint       bail_clist_get_n_columns         (BatkTable      *table);
static bint       bail_clist_get_n_actual_columns  (BtkCList      *clist);

static const bchar*
                  bail_clist_get_column_description(BatkTable      *table,
                                                    bint          column);
static BatkObject*  bail_clist_get_column_header     (BatkTable      *table,
                                                    bint          column);
static bint       bail_clist_get_n_rows            (BatkTable      *table);
static const bchar*
                  bail_clist_get_row_description   (BatkTable      *table,
                                                    bint          row);
static BatkObject*  bail_clist_get_row_header        (BatkTable      *table,
                                                    bint          row);
static BatkObject* bail_clist_get_summary           (BatkTable      *table);
static bboolean   bail_clist_add_row_selection     (BatkTable      *table,
                                                    bint          row);
static bboolean   bail_clist_remove_row_selection  (BatkTable      *table,
                                                    bint          row);
static bint       bail_clist_get_selected_rows     (BatkTable      *table,
                                                    bint          **rows_selected);
static bboolean   bail_clist_is_row_selected       (BatkTable      *table,
                                                    bint          row);
static bboolean   bail_clist_is_selected           (BatkTable      *table,
                                                    bint          row,
                                                    bint          column);
static void       bail_clist_set_caption           (BatkTable      *table,
                                                    BatkObject     *caption);
static void       bail_clist_set_column_description(BatkTable      *table,
                                                    bint          column,
                                                    const bchar   *description);
static void       bail_clist_set_column_header     (BatkTable      *table,
                                                    bint          column,
                                                    BatkObject     *header);
static void       bail_clist_set_row_description   (BatkTable      *table,
                                                    bint          row,
                                                    const bchar   *description);
static void       bail_clist_set_row_header        (BatkTable      *table,
                                                    bint          row,
                                                    BatkObject     *header);
static void       bail_clist_set_summary           (BatkTable      *table,
                                                    BatkObject     *accessible);

/* bailcellparent.h */

static void       bail_cell_parent_interface_init  (BailCellParentIface *iface);
static void       bail_clist_get_cell_extents      (BailCellParent      *parent,
                                                    BailCell            *cell,
                                                    bint                *x,
                                                    bint                *y,
                                                    bint                *width,
                                                    bint                *height,
                                                    BatkCoordType        coord_type);

static void       bail_clist_get_cell_area         (BailCellParent      *parent,
                                                    BailCell            *cell,
                                                    BdkRectangle        *cell_rect);

static void       bail_clist_select_row_btk        (BtkCList      *clist,
                                                    int           row,
                                                    int           column,
                                                    BdkEvent      *event,
                                                    bpointer      data);
static void       bail_clist_unselect_row_btk      (BtkCList      *clist,
                                                    int           row,
                                                    int           column,
                                                    BdkEvent      *event,
                                                    bpointer      data);
static bint       bail_clist_get_visible_column    (BatkTable      *table,
                                                    int           column);
static bint       bail_clist_get_actual_column     (BatkTable      *table,
                                                    int           visible_column);
static void       bail_clist_set_row_data          (BatkTable      *table,
                                                    bint          row,
                                                    const bchar   *description,
                                                    BatkObject     *header,
                                                    bboolean      is_header);
static BailCListRow*
                  bail_clist_get_row_data          (BatkTable      *table,
                                                    bint          row);
static void       bail_clist_get_visible_rect      (BtkCList      *clist,
                                                    BdkRectangle  *clist_rect);
static bboolean   bail_clist_is_cell_visible       (BdkRectangle  *cell_rect,
                                                    BdkRectangle  *visible_rect);
static void       bail_clist_cell_data_new         (BailCList     *clist,
                                                    BailCell      *cell,
                                                    bint          column,
                                                    bint          row);
static void       bail_clist_cell_destroyed        (bpointer      data);
static void       bail_clist_cell_data_remove      (BailCList     *clist,
                                                    BailCell      *cell);
static BailCell*  bail_clist_find_cell             (BailCList     *clist,
                                                    bint          index);
static void       bail_clist_adjustment_changed    (BtkAdjustment *adjustment,
                                                    BtkCList      *clist);

struct _BailCListColumn
{
  bchar *description;
  BatkObject *header;
};

struct _BailCListRow
{
  BtkCListRow *row_data;
  int row_number;
  bchar *description;
  BatkObject *header;
};

struct _BailCListCellData
{
  BtkCell *btk_cell;
  BailCell *bail_cell;
  int row_number;
  int column_number;
};

G_DEFINE_TYPE_WITH_CODE (BailCList, bail_clist, BAIL_TYPE_CONTAINER,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_TABLE, batk_table_interface_init)
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_SELECTION, batk_selection_interface_init)
                         G_IMPLEMENT_INTERFACE (BAIL_TYPE_CELL_PARENT, bail_cell_parent_interface_init))

static void
bail_clist_class_init (BailCListClass *klass)
{
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);

  class->get_n_children = bail_clist_get_n_children;
  class->ref_child = bail_clist_ref_child;
  class->ref_state_set = bail_clist_ref_state_set;
  class->initialize = bail_clist_real_initialize;

  bobject_class->finalize = bail_clist_finalize;
}

static void
bail_clist_init (BailCList *clist)
{
}

static void
bail_clist_real_initialize (BatkObject *obj,
                            bpointer  data)
{
  BailCList *clist;
  BtkCList *btk_clist;
  bint i;

  BATK_OBJECT_CLASS (bail_clist_parent_class)->initialize (obj, data);

  obj->role = BATK_ROLE_TABLE;

  clist = BAIL_CLIST (obj);

  clist->caption = NULL;
  clist->summary = NULL;
  clist->row_data = NULL;
  clist->cell_data = NULL;
  clist->previous_selected_cell = NULL;

  btk_clist = BTK_CLIST (data);
 
  clist->n_cols = btk_clist->columns;
  clist->columns = g_new (BailCListColumn, btk_clist->columns);
  for (i = 0; i < btk_clist->columns; i++)
    {
      clist->columns[i].description = NULL;
      clist->columns[i].header = NULL;
    }
  /*
   * Set up signal handlers for select-row and unselect-row
   */
  g_signal_connect (btk_clist,
                    "select-row",
                    G_CALLBACK (bail_clist_select_row_btk),
                    obj);
  g_signal_connect (btk_clist,
                    "unselect-row",
                    G_CALLBACK (bail_clist_unselect_row_btk),
                    obj);
  /*
   * Adjustment callbacks
   */
  if (btk_clist->hadjustment)
    {
      g_signal_connect (btk_clist->hadjustment,
                        "value_changed",
                        G_CALLBACK (bail_clist_adjustment_changed),
                        btk_clist);
    }
  if (btk_clist->vadjustment)
    {
      g_signal_connect (btk_clist->vadjustment,
                        "value_changed",
                        G_CALLBACK (bail_clist_adjustment_changed),
                        btk_clist);
    }
}

static void
bail_clist_finalize (BObject            *object)
{
  BailCList *clist = BAIL_CLIST (object);
  bint i;
  GArray *array;

  if (clist->caption)
    g_object_unref (clist->caption);
  if (clist->summary)
    g_object_unref (clist->summary);

  for (i = 0; i < clist->n_cols; i++)
    {
      g_free (clist->columns[i].description);
      if (clist->columns[i].header)
        g_object_unref (clist->columns[i].header);
    }
  g_free (clist->columns);

  array = clist->row_data;

  if (clist->previous_selected_cell)
    g_object_unref (clist->previous_selected_cell);

  if (array)
    {
      for (i = 0; i < array->len; i++)
        {
          BailCListRow *row_data;

          row_data = g_array_index (array, BailCListRow*, i);

          if (row_data->header)
            g_object_unref (row_data->header);
          g_free (row_data->description);
        }
    }

  if (clist->cell_data)
    {
      GList *temp_list;

      for (temp_list = clist->cell_data; temp_list; temp_list = temp_list->next)
        {
          g_list_free (temp_list->data);
        }
      g_list_free (clist->cell_data);
    }

  B_OBJECT_CLASS (bail_clist_parent_class)->finalize (object);
}

static bint
bail_clist_get_n_children (BatkObject *obj)
{
  BtkWidget *widget;
  bint row, col;

  g_return_val_if_fail (BAIL_IS_CLIST (obj), 0);

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return 0;

  row = bail_clist_get_n_rows (BATK_TABLE (obj));
  col = bail_clist_get_n_actual_columns (BTK_CLIST (widget));
  return (row * col);
}

static BatkObject*
bail_clist_ref_child (BatkObject *obj,
                      bint      i)
{
  BtkWidget *widget;
  bint row, col;
  bint n_columns;

  g_return_val_if_fail (BAIL_IS_CLIST (obj), NULL);
  g_return_val_if_fail (i >= 0, NULL);

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  n_columns = bail_clist_get_n_actual_columns (BTK_CLIST (widget));
  if (!n_columns)
    return NULL;

  row = i / n_columns;
  col = i % n_columns;
  return bail_clist_ref_at_actual (BATK_TABLE (obj), row, col);
}

static BatkStateSet*
bail_clist_ref_state_set (BatkObject *obj)
{
  BatkStateSet *state_set;
  BtkWidget *widget;

  state_set = BATK_OBJECT_CLASS (bail_clist_parent_class)->ref_state_set (obj);
  widget = BTK_ACCESSIBLE (obj)->widget;

  if (widget != NULL)
    batk_state_set_add_state (state_set, BATK_STATE_MANAGES_DESCENDANTS);

  return state_set;
}

static void
batk_selection_interface_init (BatkSelectionIface *iface)
{
  iface->clear_selection = bail_clist_clear_selection;
  iface->ref_selection = bail_clist_ref_selection;
  iface->get_selection_count = bail_clist_get_selection_count;
  iface->is_child_selected = bail_clist_is_child_selected;
  iface->select_all_selection = bail_clist_select_all_selection;
}

static bboolean
bail_clist_clear_selection (BatkSelection   *selection)
{
  BtkCList *clist;
  BtkWidget *widget;
  
  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;
  
  clist = BTK_CLIST (widget);
  btk_clist_unselect_all(clist);
  return TRUE;
}

static BatkObject*
bail_clist_ref_selection (BatkSelection   *selection,
                          bint           i)
{
  bint visible_columns;
  bint selected_row;
  bint selected_column;
  bint *selected_rows;

  if ( i < 0 && i >= bail_clist_get_selection_count (selection))
    return NULL;

  visible_columns = bail_clist_get_n_columns (BATK_TABLE (selection));
  bail_clist_get_selected_rows (BATK_TABLE (selection), &selected_rows);
  selected_row = selected_rows[i / visible_columns];
  g_free (selected_rows);
  selected_column = bail_clist_get_actual_column (BATK_TABLE (selection), 
                                                  i % visible_columns);

  return bail_clist_ref_at (BATK_TABLE (selection), selected_row, 
                            selected_column);
}

static bint
bail_clist_get_selection_count (BatkSelection   *selection)
{
  bint n_rows_selected;

  n_rows_selected = bail_clist_get_selected_rows (BATK_TABLE (selection), NULL);

  if (n_rows_selected > 0)
    /*
     * The number of cells selected is the number of columns
     * times the number of selected rows
     */
    return bail_clist_get_n_columns (BATK_TABLE (selection)) * n_rows_selected;
  return 0;
}

static bboolean
bail_clist_is_child_selected (BatkSelection   *selection,
                              bint           i)
{
  bint row;

  row = batk_table_get_row_at_index (BATK_TABLE (selection), i);

  if (row == 0 && i >= bail_clist_get_n_columns (BATK_TABLE (selection)))
    return FALSE;
  return bail_clist_is_row_selected (BATK_TABLE (selection), row);
}

static bboolean
bail_clist_select_all_selection (BatkSelection   *selection)
{
  BtkCList *clist;
  BtkWidget *widget;
  /* BtkArg arg; */
  
  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;

  clist = BTK_CLIST (widget);
  btk_clist_select_all(clist);

  return TRUE;
}

static void
batk_table_interface_init (BatkTableIface *iface)
{
  iface->ref_at = bail_clist_ref_at;
  iface->get_index_at = bail_clist_get_index_at;
  iface->get_column_at_index = bail_clist_get_column_at_index;
  iface->get_row_at_index = bail_clist_get_row_at_index;
  iface->get_caption = bail_clist_get_caption;
  iface->get_n_columns = bail_clist_get_n_columns;
  iface->get_column_description = bail_clist_get_column_description;
  iface->get_column_header = bail_clist_get_column_header;
  iface->get_n_rows = bail_clist_get_n_rows;
  iface->get_row_description = bail_clist_get_row_description;
  iface->get_row_header = bail_clist_get_row_header;
  iface->get_summary = bail_clist_get_summary;
  iface->add_row_selection = bail_clist_add_row_selection;
  iface->remove_row_selection = bail_clist_remove_row_selection;
  iface->get_selected_rows = bail_clist_get_selected_rows;
  iface->is_row_selected = bail_clist_is_row_selected;
  iface->is_selected = bail_clist_is_selected;
  iface->set_caption = bail_clist_set_caption;
  iface->set_column_description = bail_clist_set_column_description;
  iface->set_column_header = bail_clist_set_column_header;
  iface->set_row_description = bail_clist_set_row_description;
  iface->set_row_header = bail_clist_set_row_header;
  iface->set_summary = bail_clist_set_summary;
}

static BatkObject*
bail_clist_ref_at (BatkTable *table,
                   bint     row,
                   bint     column)
{
  BtkWidget *widget;
  bint actual_column;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  actual_column = bail_clist_get_actual_column (table, column);
  return bail_clist_ref_at_actual (table, row, actual_column);
}


static BatkObject*
bail_clist_ref_at_actual (BatkTable      *table,
                          bint          row,
                          bint          column)
{
  /*
   * The column number pased to this function is the actual column number
   * whereas the column number passed to bail_clist_ref_at is the
   * visible column number
   */
  BtkCList *clist;
  BtkWidget *widget;
  BtkCellType cellType;
  BatkObject *return_object;
  bint n_rows, n_columns;
  bint index;
  BailCell *cell;

  g_return_val_if_fail (BTK_IS_ACCESSIBLE (table), NULL);
  
  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  clist = BTK_CLIST (widget);
  n_rows = bail_clist_get_n_rows (table); 
  n_columns = bail_clist_get_n_actual_columns (clist); 

  if (row < 0 || row >= n_rows)
    return NULL;
  if (column < 0 || column >= n_columns)
    return NULL;

  /*
   * Check whether the child is cached
   */
  index =  column + row * n_columns;
  cell = bail_clist_find_cell (BAIL_CLIST (table), index);
  if (cell)
    {
      g_object_ref (cell);
      return BATK_OBJECT (cell);
    }
  cellType = btk_clist_get_cell_type(clist, row, column);
  switch (cellType) 
    {
    case BTK_CELL_TEXT:
    case BTK_CELL_PIXTEXT:
      return_object = bail_clist_cell_new ();
      break;
    case BTK_CELL_PIXMAP:
      return_object = NULL;
      break;
    default:
      /* Don't handle BTK_CELL_EMPTY or BTK_CELL_WIDGET, return NULL */
      return_object = NULL;
      break;
    }
  if (return_object)
    {
      cell = BAIL_CELL (return_object);

      g_return_val_if_fail (BATK_IS_OBJECT (table), NULL);

      bail_cell_initialise (cell, widget, BATK_OBJECT (table),
                            index);
      /*
       * Store the cell in a cache
       */
      bail_clist_cell_data_new (BAIL_CLIST (table), cell, column, row);
      /*
       * If the column is visible, sets the cell's state
       */
      if (clist->column[column].visible)
        {
          BdkRectangle cell_rect, visible_rect;
  
          bail_clist_get_cell_area (BAIL_CELL_PARENT (table), cell, &cell_rect);
          bail_clist_get_visible_rect (clist, &visible_rect);
          bail_cell_add_state (cell, BATK_STATE_VISIBLE, FALSE);
          if (bail_clist_is_cell_visible (&cell_rect, &visible_rect))
            bail_cell_add_state (cell, BATK_STATE_SHOWING, FALSE);
        }
      /*
       * If a row is selected, all cells in the row are selected
       */
      if (bail_clist_is_row_selected (table, row))
        {
          bail_cell_add_state (cell, BATK_STATE_SELECTED, FALSE);
          if (clist->columns == 1)
            bail_cell_add_state (cell, BATK_STATE_FOCUSED, FALSE);
        }
    }

  return return_object; 
}

static bint
bail_clist_get_index_at (BatkTable *table,
                         bint     row,
                         bint     column)
{
  bint n_cols, n_rows;

  n_cols = batk_table_get_n_columns (table);
  n_rows = batk_table_get_n_rows (table);

  g_return_val_if_fail (row < n_rows, 0);
  g_return_val_if_fail (column < n_cols, 0);

  return row * n_cols + column;
}

static bint
bail_clist_get_column_at_index (BatkTable *table,
                                bint     index)
{
  bint n_cols;

  n_cols = batk_table_get_n_columns (table);

  if (n_cols == 0)
    return 0;
  else
    return (bint) (index % n_cols);
}

static bint
bail_clist_get_row_at_index (BatkTable *table,
                             bint     index)
{
  bint n_cols;

  n_cols = batk_table_get_n_columns (table);

  if (n_cols == 0)
    return 0;
  else
    return (bint) (index / n_cols);
}

static BatkObject*
bail_clist_get_caption (BatkTable      *table)
{
  BailCList* obj = BAIL_CLIST (table);

  return obj->caption;
}

static bint
bail_clist_get_n_columns (BatkTable      *table)
{
  BtkWidget *widget;
  BtkCList *clist;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return 0;

  clist = BTK_CLIST (widget);

  return bail_clist_get_visible_column (table, 
                                  bail_clist_get_n_actual_columns (clist)); 
}

static bint
bail_clist_get_n_actual_columns (BtkCList *clist)
{
  return clist->columns;
}

static const bchar*
bail_clist_get_column_description (BatkTable      *table,
                                   bint          column)
{
  BailCList *clist = BAIL_CLIST (table);
  BtkWidget *widget;
  bint actual_column;

  if (column < 0 || column >= bail_clist_get_n_columns (table))
    return NULL;

  actual_column = bail_clist_get_actual_column (table, column);
  if (clist->columns[actual_column].description)
    return (clist->columns[actual_column].description);

  widget = BTK_ACCESSIBLE (clist)->widget;
  if (widget == NULL)
    return NULL;

  return btk_clist_get_column_title (BTK_CLIST (widget), actual_column);
}

static BatkObject*
bail_clist_get_column_header (BatkTable      *table,
                              bint          column)
{
  BailCList *clist = BAIL_CLIST (table);
  BtkWidget *widget;
  BtkWidget *return_widget;
  bint actual_column;

  if (column < 0 || column >= bail_clist_get_n_columns (table))
    return NULL;

  actual_column = bail_clist_get_actual_column (table, column);

  if (clist->columns[actual_column].header)
    return (clist->columns[actual_column].header);

  widget = BTK_ACCESSIBLE (clist)->widget;
  if (widget == NULL)
    return NULL;

  return_widget = btk_clist_get_column_widget (BTK_CLIST (widget), 
                                               actual_column);
  if (return_widget == NULL)
    return NULL;

  g_return_val_if_fail (BTK_IS_BIN (return_widget), NULL);
  return_widget = btk_bin_get_child (BTK_BIN(return_widget));

  return btk_widget_get_accessible (return_widget);
}

static bint
bail_clist_get_n_rows (BatkTable      *table)
{
  BtkWidget *widget;
  BtkCList *clist;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return 0;

  clist = BTK_CLIST (widget);
  return clist->rows;
}

static const bchar*
bail_clist_get_row_description (BatkTable      *table,
                                bint          row)
{
  BailCListRow* row_data;

  row_data = bail_clist_get_row_data (table, row);
  if (row_data == NULL)
    return NULL;
  return row_data->description;
}

static BatkObject*
bail_clist_get_row_header (BatkTable      *table,
                           bint          row)
{
  BailCListRow* row_data;

  row_data = bail_clist_get_row_data (table, row);
  if (row_data == NULL)
    return NULL;
  return row_data->header;
}

static BatkObject*
bail_clist_get_summary (BatkTable      *table)
{
  BailCList* obj = BAIL_CLIST (table);

  return obj->summary;
}

static bboolean
bail_clist_add_row_selection (BatkTable      *table,
                              bint          row)
{
  BtkWidget *widget;
  BtkCList *clist;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return 0;

  clist = BTK_CLIST (widget);
  btk_clist_select_row (clist, row, -1);
  if (bail_clist_is_row_selected (table, row))
    return TRUE;
  
  return FALSE;
}

static bboolean
bail_clist_remove_row_selection (BatkTable      *table,
                                 bint          row)
{
  BtkWidget *widget;
  BtkCList *clist;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return 0;

  clist = BTK_CLIST (widget);
  if (bail_clist_is_row_selected (table, row))
  {
    btk_clist_select_row (clist, row, -1);
    return TRUE;
  }
  return FALSE;
}

static bint
bail_clist_get_selected_rows (BatkTable *table,
                              bint     **rows_selected)
{
  BtkWidget *widget;
  BtkCList *clist;
  GList *list;
  bint n_selected;
  bint i;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return 0;

  clist = BTK_CLIST (widget);
 
  n_selected = g_list_length (clist->selection);

  if (n_selected == 0) 
    return 0;

  if (rows_selected)
    {
      bint *selected_rows;

      selected_rows = (bint*) g_malloc (sizeof (bint) * n_selected);
      list = clist->selection;

      i = 0;
      while (list)
        {
          selected_rows[i++] = BPOINTER_TO_INT (list->data);
          list = list->next;
        }
      *rows_selected = selected_rows;
    }
  return n_selected;
}

static bboolean
bail_clist_is_row_selected (BatkTable      *table,
                            bint          row)
{
  GList *elem;
  BtkWidget *widget;
  BtkCList *clist;
  BtkCListRow *clist_row;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
	return FALSE;

  clist = BTK_CLIST (widget);
      
  if (row < 0 || row >= clist->rows)
    return FALSE;

  elem = ROW_ELEMENT (clist, row);
  if (!elem)
    return FALSE;
  clist_row = elem->data;

  return (clist_row->state == BTK_STATE_SELECTED);
}

static bboolean
bail_clist_is_selected (BatkTable      *table,
                        bint          row,
                        bint          column)
{
  return bail_clist_is_row_selected (table, row);
}

static void
bail_clist_set_caption (BatkTable      *table,
                        BatkObject     *caption)
{
  BailCList* obj = BAIL_CLIST (table);
  BatkPropertyValues values = { NULL };
  BatkObject *old_caption;

  old_caption = obj->caption;
  obj->caption = caption;
  if (obj->caption)
    g_object_ref (obj->caption);

  b_value_init (&values.old_value, B_TYPE_POINTER);
  b_value_set_pointer (&values.old_value, old_caption);
  b_value_init (&values.new_value, B_TYPE_POINTER);
  b_value_set_pointer (&values.new_value, obj->caption);

  values.property_name = "accessible-table-caption";
  g_signal_emit_by_name (table, 
                         "property_change::accessible-table-caption", 
                         &values, NULL);
  if (old_caption)
    g_object_unref (old_caption);
}

static void
bail_clist_set_column_description (BatkTable      *table,
                                   bint          column,
                                   const bchar   *description)
{
  BailCList *clist = BAIL_CLIST (table);
  BatkPropertyValues values = { NULL };
  bint actual_column;

  if (column < 0 || column >= bail_clist_get_n_columns (table))
    return;

  if (description == NULL)
    return;

  actual_column = bail_clist_get_actual_column (table, column);
  g_free (clist->columns[actual_column].description);
  clist->columns[actual_column].description = g_strdup (description);

  b_value_init (&values.new_value, B_TYPE_INT);
  b_value_set_int (&values.new_value, column);

  values.property_name = "accessible-table-column-description";
  g_signal_emit_by_name (table, 
                         "property_change::accessible-table-column-description",
                          &values, NULL);

}

static void
bail_clist_set_column_header (BatkTable      *table,
                              bint          column,
                              BatkObject     *header)
{
  BailCList *clist = BAIL_CLIST (table);
  BatkPropertyValues values = { NULL };
  bint actual_column;

  if (column < 0 || column >= bail_clist_get_n_columns (table))
    return;

  actual_column = bail_clist_get_actual_column (table, column);
  if (clist->columns[actual_column].header)
    g_object_unref (clist->columns[actual_column].header);
  if (header)
    g_object_ref (header);
  clist->columns[actual_column].header = header;

  b_value_init (&values.new_value, B_TYPE_INT);
  b_value_set_int (&values.new_value, column);

  values.property_name = "accessible-table-column-header";
  g_signal_emit_by_name (table, 
                         "property_change::accessible-table-column-header",
                         &values, NULL);
}

static void
bail_clist_set_row_description (BatkTable      *table,
                                bint          row,
                                const bchar   *description)
{
  bail_clist_set_row_data (table, row, description, NULL, FALSE);
}

static void
bail_clist_set_row_header (BatkTable      *table,
                           bint          row,
                           BatkObject     *header)
{
  bail_clist_set_row_data (table, row, NULL, header, TRUE);
}

static void
bail_clist_set_summary (BatkTable      *table,
                        BatkObject     *accessible)
{
  BailCList* obj = BAIL_CLIST (table);
  BatkPropertyValues values = { 0, };
  BatkObject *old_summary;

  old_summary = obj->summary;
  obj->summary = accessible;
  if (obj->summary)
    g_object_ref (obj->summary);

  b_value_init (&values.old_value, B_TYPE_POINTER);
  b_value_set_pointer (&values.old_value, old_summary);
  b_value_init (&values.new_value, B_TYPE_POINTER);
  b_value_set_pointer (&values.new_value, obj->summary);

  values.property_name = "accessible-table-summary";
  g_signal_emit_by_name (table, 
                         "property_change::accessible-table-summary", 
                         &values, NULL);
  if (old_summary)
    g_object_unref (old_summary);
}


static void bail_cell_parent_interface_init (BailCellParentIface *iface)
{
  iface->get_cell_extents = bail_clist_get_cell_extents;
  iface->get_cell_area = bail_clist_get_cell_area;
}

static void
bail_clist_get_cell_extents (BailCellParent *parent,
                             BailCell       *cell,
                             bint           *x,
                             bint           *y,
                             bint           *width,
                             bint           *height,
                             BatkCoordType   coord_type)
{
  BtkWidget* widget;
  BtkCList *clist;
  bint widget_x, widget_y, widget_width, widget_height;
  BdkRectangle cell_rect;
  BdkRectangle visible_rect;

  widget = BTK_ACCESSIBLE (parent)->widget;
  if (widget == NULL)
    return;
  clist = BTK_CLIST (widget);

  batk_component_get_extents (BATK_COMPONENT (parent), &widget_x, &widget_y,
                             &widget_width, &widget_height,
                             coord_type);

  bail_clist_get_cell_area (parent, cell, &cell_rect);
  *width = cell_rect.width;
  *height = cell_rect.height;
  bail_clist_get_visible_rect (clist, &visible_rect);
  if (bail_clist_is_cell_visible (&cell_rect, &visible_rect))
    {
      *x = cell_rect.x + widget_x;
      *y = cell_rect.y + widget_y;
    }
  else
    {
      *x = B_MININT;
      *y = B_MININT;
    }
}

static void
bail_clist_get_cell_area (BailCellParent *parent,
                          BailCell       *cell,
                          BdkRectangle   *cell_rect)
{
  BtkWidget* widget;
  BtkCList *clist;
  bint column, row, n_columns;

  widget = BTK_ACCESSIBLE (parent)->widget;
  if (widget == NULL)
    return;
  clist = BTK_CLIST (widget);

  n_columns = bail_clist_get_n_actual_columns (clist);
  g_return_if_fail (n_columns > 0);
  column = cell->index % n_columns;
  row = cell->index / n_columns; 
  cell_rect->x = COLUMN_LEFT (clist, column);
  cell_rect->y = ROW_TOP (clist, row);
  cell_rect->width = clist->column[column].area.width;
  cell_rect->height = clist->row_height;
}

static void
bail_clist_select_row_btk (BtkCList *clist,
                           bint      row,
                           bint      column,
                           BdkEvent *event,
                           bpointer data)
{
  BailCList *bail_clist;
  GList *temp_list;
  BatkObject *selected_cell;

  bail_clist = BAIL_CLIST (data);

  for (temp_list = bail_clist->cell_data; temp_list; temp_list = temp_list->next)
    {
      BailCListCellData *cell_data;

      cell_data = (BailCListCellData *) (temp_list->data);

      if (row == cell_data->row_number)
        {
          /*
           * Row is selected
           */
          bail_cell_add_state (cell_data->bail_cell, BATK_STATE_SELECTED, TRUE);
        }
    }
  if (clist->columns == 1)
    {
      selected_cell = bail_clist_ref_at (BATK_TABLE (data), row, 1);
      if (selected_cell)
        {
          if (bail_clist->previous_selected_cell)
            g_object_unref (bail_clist->previous_selected_cell);
          bail_clist->previous_selected_cell = selected_cell;
          bail_cell_add_state (BAIL_CELL (selected_cell), BATK_STATE_FOCUSED, FALSE);
          g_signal_emit_by_name (bail_clist,
                                 "active-descendant-changed",
                                  selected_cell);
       }
    }

  g_signal_emit_by_name (bail_clist, "selection_changed");
}

static void
bail_clist_unselect_row_btk (BtkCList *clist,
                             bint      row,
                             bint      column,
                             BdkEvent *event,
                             bpointer data)
{
  BailCList *bail_clist;
  GList *temp_list;

  bail_clist = BAIL_CLIST (data);

  for (temp_list = bail_clist->cell_data; temp_list; temp_list = temp_list->next)
    {
      BailCListCellData *cell_data;

      cell_data = (BailCListCellData *) (temp_list->data);

      if (row == cell_data->row_number)
        {
          /*
           * Row is unselected
           */
          bail_cell_add_state (cell_data->bail_cell, BATK_STATE_FOCUSED, FALSE);
          bail_cell_remove_state (cell_data->bail_cell, BATK_STATE_SELECTED, TRUE);
       }
    }

  g_signal_emit_by_name (bail_clist, "selection_changed");
}

/*
 * This function determines the number of visible columns
 * up to and including the specified column
 */
static bint
bail_clist_get_visible_column (BatkTable *table,
                               int      column)
{
  BtkWidget *widget;
  BtkCList *clist;
  bint i;
  bint vis_columns;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return 0;

  clist = BTK_CLIST (widget);
  for (i = 0, vis_columns = 0; i < column; i++)
    if (clist->column[i].visible)
      vis_columns++;

  return vis_columns;  
}

static bint
bail_clist_get_actual_column (BatkTable *table,
                              int      visible_column)
{
  BtkWidget *widget;
  BtkCList *clist;
  bint i;
  bint vis_columns;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return 0;

  clist = BTK_CLIST (widget);
  for (i = 0, vis_columns = 0; i < clist->columns; i++)
    {
      if (clist->column[i].visible)
        {
          if (visible_column == vis_columns)
            return i;
          vis_columns++;
        }
    }
  return 0;  
}

static void
bail_clist_set_row_data (BatkTable      *table,
                         bint          row,
                         const bchar   *description,
                         BatkObject     *header,
                         bboolean      is_header)
{
  BtkWidget *widget;
  BtkCList *btk_clist;
  BailCList *bail_clist;
  GArray *array;
  BailCListRow* row_data;
  bint i;
  bboolean found = FALSE;
  BatkPropertyValues values = { NULL };
  bchar *signal_name;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return;

  btk_clist = BTK_CLIST (widget);
  if (row < 0 || row >= btk_clist->rows)
    return;

  bail_clist = BAIL_CLIST (table);

  if (bail_clist->row_data == NULL)
    bail_clist->row_data = g_array_sized_new (FALSE, TRUE, 
                                              sizeof (BailCListRow *), 0);

  array = bail_clist->row_data;

  for (i = 0; i < array->len; i++)
    {
      row_data = g_array_index (array, BailCListRow*, i);

      if (row == row_data->row_number)
        {
          found = TRUE;
          if (is_header)
            {
              if (row_data->header)
                g_object_unref (row_data->header);
              row_data->header = header;
              if (row_data->header)
                g_object_ref (row_data->header);
            }
          else
            {
              g_free (row_data->description);
              row_data->description = g_strdup (row_data->description);
            }
          break;
        }
    } 
  if (!found)
    {
      GList *elem;

      elem = ROW_ELEMENT (btk_clist, row);
      g_return_if_fail (elem != NULL);

      row_data = g_new (BailCListRow, 1);
      row_data->row_number = row;
      row_data->row_data = elem->data;
      if (is_header)
        {
          row_data->header = header;
          if (row_data->header)
            g_object_ref (row_data->header);
          row_data->description = NULL;
        }
      else
        {
          row_data->description = g_strdup (row_data->description);
          row_data->header = NULL;
        }
      g_array_append_val (array, row_data);
    }

  b_value_init (&values.new_value, B_TYPE_INT);
  b_value_set_int (&values.new_value, row);

  if (is_header)
    {
      values.property_name = "accessible-table-row-header";
      signal_name = "property_change::accessible-table-row-header";
    }
  else
    {
      values.property_name = "accessible-table-row-description";
      signal_name = "property_change::accessible-table-row-description";
    }
  g_signal_emit_by_name (table, 
                         signal_name,
                         &values, NULL);

}

static BailCListRow*
bail_clist_get_row_data (BatkTable      *table,
                         bint          row)
{
  BtkWidget *widget;
  BtkCList *clist;
  BailCList *obj;
  GArray *array;
  BailCListRow* row_data;
  bint i;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  clist = BTK_CLIST (widget);
  if (row < 0 || row >= clist->rows)
    return NULL;

  obj = BAIL_CLIST (table);

  if (obj->row_data == NULL)
    return NULL;

  array = obj->row_data;

  for (i = 0; i < array->len; i++)
    {
      row_data = g_array_index (array, BailCListRow*, i);

      if (row == row_data->row_number)
        return row_data;
    }
 
  return NULL;
}

static void
bail_clist_get_visible_rect (BtkCList      *clist,
                             BdkRectangle  *clist_rect)
{
  clist_rect->x = - clist->hoffset;
  clist_rect->y = - clist->voffset;
  clist_rect->width = clist->clist_window_width;
  clist_rect->height = clist->clist_window_height;
}

static bboolean
bail_clist_is_cell_visible (BdkRectangle  *cell_rect,
                            BdkRectangle  *visible_rect)
{
  /*
   * A cell is reported as visible if any part of the cell is visible
   */
  if (((cell_rect->x + cell_rect->width) < visible_rect->x) ||
     ((cell_rect->y + cell_rect->height) < visible_rect->y) ||
     (cell_rect->x > (visible_rect->x + visible_rect->width)) ||
     (cell_rect->y > (visible_rect->y + visible_rect->height)))
    return FALSE;
  else
    return TRUE;
}

static void
bail_clist_cell_data_new (BailCList     *clist,
                          BailCell      *cell,
                          bint          column,
                          bint          row)
{
  GList *elem;
  BailCListCellData *cell_data;
  BtkCList *btk_clist;
  BtkCListRow *clist_row;

  btk_clist = BTK_CLIST (BTK_ACCESSIBLE (clist)->widget);
  elem = g_list_nth (btk_clist->row_list, row);
  g_return_if_fail (elem != NULL);
  clist_row = (BtkCListRow *) elem->data;
  cell_data = g_new (BailCListCellData, 1);
  cell_data->bail_cell = cell;
  cell_data->btk_cell = &(clist_row->cell[column]);
  cell_data->column_number = column;
  cell_data->row_number = row;
  clist->cell_data = g_list_append (clist->cell_data, cell_data);

  g_object_weak_ref (B_OBJECT (cell),
                     (GWeakNotify) bail_clist_cell_destroyed,
                     cell);
}

static void
bail_clist_cell_destroyed (bpointer      data)
{
  BailCell *cell = BAIL_CELL (data);
  BatkObject* parent;

  parent = batk_object_get_parent (BATK_OBJECT (cell));

  bail_clist_cell_data_remove (BAIL_CLIST (parent), cell);
}

static void
bail_clist_cell_data_remove (BailCList *clist,
                             BailCell  *cell)
{
  GList *temp_list;

  for (temp_list = clist->cell_data; temp_list; temp_list = temp_list->next)
    {
      BailCListCellData *cell_data;

      cell_data = (BailCListCellData *) temp_list->data;
      if (cell_data->bail_cell == cell)
        {
          clist->cell_data = g_list_remove_link (clist->cell_data, temp_list);
          g_free (cell_data);
          return;
        }
    }
  g_warning ("No cell removed in bail_clist_cell_data_remove\n");
}

static BailCell*
bail_clist_find_cell (BailCList     *clist,
                      bint          index)
{
  GList *temp_list;
  bint n_cols;

  n_cols = clist->n_cols;

  for (temp_list = clist->cell_data; temp_list; temp_list = temp_list->next)
    {
      BailCListCellData *cell_data;
      bint real_index;

      cell_data = (BailCListCellData *) (temp_list->data);

      real_index = cell_data->column_number + n_cols * cell_data->row_number;
      if (real_index == index)
        return cell_data->bail_cell;
    }
  return NULL;
}

static void
bail_clist_adjustment_changed (BtkAdjustment *adjustment,
                               BtkCList      *clist)
{
  BatkObject *batk_obj;
  BdkRectangle visible_rect;
  BdkRectangle cell_rect;
  BailCList* obj;
  GList *temp_list;

  /*
   * The scrollbars have changed
   */
  batk_obj = btk_widget_get_accessible (BTK_WIDGET (clist));
  obj = BAIL_CLIST (batk_obj);

  /* Get the currently visible area */
  bail_clist_get_visible_rect (clist, &visible_rect);

  /* loop over the cells and report if they are visible or not. */
  /* Must loop through them all */
  for (temp_list = obj->cell_data; temp_list; temp_list = temp_list->next)
    {
      BailCell *cell;
      BailCListCellData *cell_data;

      cell_data = (BailCListCellData *) (temp_list->data);
      cell = cell_data->bail_cell;

      bail_clist_get_cell_area (BAIL_CELL_PARENT (batk_obj), 
                                cell, &cell_rect);
      if (bail_clist_is_cell_visible (&cell_rect, &visible_rect))
        bail_cell_add_state (cell, BATK_STATE_SHOWING, TRUE);
      else
        bail_cell_remove_state (cell, BATK_STATE_SHOWING, TRUE);
    }
  g_signal_emit_by_name (batk_obj, "visible_data_changed");
}

