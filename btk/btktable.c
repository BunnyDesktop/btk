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
#include "btktable.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

enum
{
  PROP_0,
  PROP_N_ROWS,
  PROP_N_COLUMNS,
  PROP_COLUMN_SPACING,
  PROP_ROW_SPACING,
  PROP_HOMOGENEOUS
};

enum
{
  CHILD_PROP_0,
  CHILD_PROP_LEFT_ATTACH,
  CHILD_PROP_RIGHT_ATTACH,
  CHILD_PROP_TOP_ATTACH,
  CHILD_PROP_BOTTOM_ATTACH,
  CHILD_PROP_X_OPTIONS,
  CHILD_PROP_Y_OPTIONS,
  CHILD_PROP_X_PADDING,
  CHILD_PROP_Y_PADDING
};
  

static void btk_table_finalize	    (BObject	    *object);
static void btk_table_size_request  (BtkWidget	    *widget,
				     BtkRequisition *requisition);
static void btk_table_size_allocate (BtkWidget	    *widget,
				     BtkAllocation  *allocation);
static void btk_table_add	    (BtkContainer   *container,
				     BtkWidget	    *widget);
static void btk_table_remove	    (BtkContainer   *container,
				     BtkWidget	    *widget);
static void btk_table_forall	    (BtkContainer   *container,
				     bboolean	     include_internals,
				     BtkCallback     callback,
				     bpointer	     callback_data);
static void btk_table_get_property  (BObject         *object,
				     buint            prop_id,
				     BValue          *value,
				     BParamSpec      *pspec);
static void btk_table_set_property  (BObject         *object,
				     buint            prop_id,
				     const BValue    *value,
				     BParamSpec      *pspec);
static void btk_table_set_child_property (BtkContainer    *container,
					  BtkWidget       *child,
					  buint            property_id,
					  const BValue    *value,
					  BParamSpec      *pspec);
static void btk_table_get_child_property (BtkContainer    *container,
					  BtkWidget       *child,
					  buint            property_id,
					  BValue          *value,
					  BParamSpec      *pspec);
static GType btk_table_child_type   (BtkContainer   *container);


static void btk_table_size_request_init	 (BtkTable *table);
static void btk_table_size_request_pass1 (BtkTable *table);
static void btk_table_size_request_pass2 (BtkTable *table);
static void btk_table_size_request_pass3 (BtkTable *table);

static void btk_table_size_allocate_init  (BtkTable *table);
static void btk_table_size_allocate_pass1 (BtkTable *table);
static void btk_table_size_allocate_pass2 (BtkTable *table);


G_DEFINE_TYPE (BtkTable, btk_table, BTK_TYPE_CONTAINER)

static void
btk_table_class_init (BtkTableClass *class)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (class);
  BtkWidgetClass *widget_class = BTK_WIDGET_CLASS (class);
  BtkContainerClass *container_class = BTK_CONTAINER_CLASS (class);
  
  bobject_class->finalize = btk_table_finalize;

  bobject_class->get_property = btk_table_get_property;
  bobject_class->set_property = btk_table_set_property;
  
  widget_class->size_request = btk_table_size_request;
  widget_class->size_allocate = btk_table_size_allocate;
  
  container_class->add = btk_table_add;
  container_class->remove = btk_table_remove;
  container_class->forall = btk_table_forall;
  container_class->child_type = btk_table_child_type;
  container_class->set_child_property = btk_table_set_child_property;
  container_class->get_child_property = btk_table_get_child_property;
  

  g_object_class_install_property (bobject_class,
                                   PROP_N_ROWS,
                                   g_param_spec_uint ("n-rows",
						     P_("Rows"),
						     P_("The number of rows in the table"),
						     1,
						     65535,
						     1,
						     BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_N_COLUMNS,
                                   g_param_spec_uint ("n-columns",
						     P_("Columns"),
						     P_("The number of columns in the table"),
						     1,
						     65535,
						     1,
						     BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_ROW_SPACING,
                                   g_param_spec_uint ("row-spacing",
						     P_("Row spacing"),
						     P_("The amount of space between two consecutive rows"),
						     0,
						     65535,
						     0,
						     BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_COLUMN_SPACING,
                                   g_param_spec_uint ("column-spacing",
						     P_("Column spacing"),
						     P_("The amount of space between two consecutive columns"),
						     0,
						     65535,
						     0,
						     BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_HOMOGENEOUS,
                                   g_param_spec_boolean ("homogeneous",
							 P_("Homogeneous"),
							 P_("If TRUE, the table cells are all the same width/height"),
							 FALSE,
							 BTK_PARAM_READWRITE));

  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_LEFT_ATTACH,
					      g_param_spec_uint ("left-attach", 
								 P_("Left attachment"), 
								 P_("The column number to attach the left side of the child to"),
								 0, 65535, 0,
								 BTK_PARAM_READWRITE));
  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_RIGHT_ATTACH,
					      g_param_spec_uint ("right-attach", 
								 P_("Right attachment"), 
								 P_("The column number to attach the right side of a child widget to"),
								 1, 65535, 1,
								 BTK_PARAM_READWRITE));
  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_TOP_ATTACH,
					      g_param_spec_uint ("top-attach", 
								 P_("Top attachment"), 
								 P_("The row number to attach the top of a child widget to"),
								 0, 65535, 0,
								 BTK_PARAM_READWRITE));
  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_BOTTOM_ATTACH,
					      g_param_spec_uint ("bottom-attach",
								 P_("Bottom attachment"), 
								 P_("The row number to attach the bottom of the child to"),
								 1, 65535, 1,
								 BTK_PARAM_READWRITE));
  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_X_OPTIONS,
					      g_param_spec_flags ("x-options", 
								  P_("Horizontal options"), 
								  P_("Options specifying the horizontal behaviour of the child"),
								  BTK_TYPE_ATTACH_OPTIONS, BTK_EXPAND | BTK_FILL,
								  BTK_PARAM_READWRITE));
  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_Y_OPTIONS,
					      g_param_spec_flags ("y-options", 
								  P_("Vertical options"), 
								  P_("Options specifying the vertical behaviour of the child"),
								  BTK_TYPE_ATTACH_OPTIONS, BTK_EXPAND | BTK_FILL,
								  BTK_PARAM_READWRITE));
  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_X_PADDING,
					      g_param_spec_uint ("x-padding", 
								 P_("Horizontal padding"), 
								 P_("Extra space to put between the child and its left and right neighbors, in pixels"),
								 0, 65535, 0,
								 BTK_PARAM_READWRITE));
  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_Y_PADDING,
					      g_param_spec_uint ("y-padding", 
								 P_("Vertical padding"), 
								 P_("Extra space to put between the child and its upper and lower neighbors, in pixels"),
								 0, 65535, 0,
								 BTK_PARAM_READWRITE));
}

static GType
btk_table_child_type (BtkContainer   *container)
{
  return BTK_TYPE_WIDGET;
}

static void
btk_table_get_property (BObject      *object,
			buint         prop_id,
			BValue       *value,
			BParamSpec   *pspec)
{
  BtkTable *table;

  table = BTK_TABLE (object);

  switch (prop_id)
    {
    case PROP_N_ROWS:
      b_value_set_uint (value, table->nrows);
      break;
    case PROP_N_COLUMNS:
      b_value_set_uint (value, table->ncols);
      break;
    case PROP_ROW_SPACING:
      b_value_set_uint (value, table->row_spacing);
      break;
    case PROP_COLUMN_SPACING:
      b_value_set_uint (value, table->column_spacing);
      break;
    case PROP_HOMOGENEOUS:
      b_value_set_boolean (value, table->homogeneous);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_table_set_property (BObject      *object,
			buint         prop_id,
			const BValue *value,
			BParamSpec   *pspec)
{
  BtkTable *table;

  table = BTK_TABLE (object);

  switch (prop_id)
    {
    case PROP_N_ROWS:
      btk_table_resize (table, b_value_get_uint (value), table->ncols);
      break;
    case PROP_N_COLUMNS:
      btk_table_resize (table, table->nrows, b_value_get_uint (value));
      break;
    case PROP_ROW_SPACING:
      btk_table_set_row_spacings (table, b_value_get_uint (value));
      break;
    case PROP_COLUMN_SPACING:
      btk_table_set_col_spacings (table, b_value_get_uint (value));
      break;
    case PROP_HOMOGENEOUS:
      btk_table_set_homogeneous (table, b_value_get_boolean (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_table_set_child_property (BtkContainer    *container,
			      BtkWidget       *child,
			      buint            property_id,
			      const BValue    *value,
			      BParamSpec      *pspec)
{
  BtkTable *table = BTK_TABLE (container);
  BtkTableChild *table_child;
  GList *list;

  table_child = NULL;
  for (list = table->children; list; list = list->next)
    {
      table_child = list->data;

      if (table_child->widget == child)
	break;
    }
  if (!list)
    {
      BTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      return;
    }

  switch (property_id)
    {
    case CHILD_PROP_LEFT_ATTACH:
      table_child->left_attach = b_value_get_uint (value);
      if (table_child->right_attach <= table_child->left_attach)
	table_child->right_attach = table_child->left_attach + 1;
      if (table_child->right_attach >= table->ncols)
	btk_table_resize (table, table->nrows, table_child->right_attach);
      break;
    case CHILD_PROP_RIGHT_ATTACH:
      table_child->right_attach = b_value_get_uint (value);
      if (table_child->right_attach <= table_child->left_attach)
	table_child->left_attach = table_child->right_attach - 1;
      if (table_child->right_attach >= table->ncols)
	btk_table_resize (table, table->nrows, table_child->right_attach);
      break;
    case CHILD_PROP_TOP_ATTACH:
      table_child->top_attach = b_value_get_uint (value);
      if (table_child->bottom_attach <= table_child->top_attach)
	table_child->bottom_attach = table_child->top_attach + 1;
      if (table_child->bottom_attach >= table->nrows)
	btk_table_resize (table, table_child->bottom_attach, table->ncols);
      break;
    case CHILD_PROP_BOTTOM_ATTACH:
      table_child->bottom_attach = b_value_get_uint (value);
      if (table_child->bottom_attach <= table_child->top_attach)
	table_child->top_attach = table_child->bottom_attach - 1;
      if (table_child->bottom_attach >= table->nrows)
	btk_table_resize (table, table_child->bottom_attach, table->ncols);
      break;
    case CHILD_PROP_X_OPTIONS:
      table_child->xexpand = (b_value_get_flags (value) & BTK_EXPAND) != 0;
      table_child->xshrink = (b_value_get_flags (value) & BTK_SHRINK) != 0;
      table_child->xfill = (b_value_get_flags (value) & BTK_FILL) != 0;
      break;
    case CHILD_PROP_Y_OPTIONS:
      table_child->yexpand = (b_value_get_flags (value) & BTK_EXPAND) != 0;
      table_child->yshrink = (b_value_get_flags (value) & BTK_SHRINK) != 0;
      table_child->yfill = (b_value_get_flags (value) & BTK_FILL) != 0;
      break;
    case CHILD_PROP_X_PADDING:
      table_child->xpadding = b_value_get_uint (value);
      break;
    case CHILD_PROP_Y_PADDING:
      table_child->ypadding = b_value_get_uint (value);
      break;
    default:
      BTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
  if (btk_widget_get_visible (child) &&
      btk_widget_get_visible (BTK_WIDGET (table)))
    btk_widget_queue_resize (child);
}

static void
btk_table_get_child_property (BtkContainer    *container,
			      BtkWidget       *child,
			      buint            property_id,
			      BValue          *value,
			      BParamSpec      *pspec)
{
  BtkTable *table = BTK_TABLE (container);
  BtkTableChild *table_child;
  GList *list;

  table_child = NULL;
  for (list = table->children; list; list = list->next)
    {
      table_child = list->data;

      if (table_child->widget == child)
	break;
    }
  if (!list)
    {
      BTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      return;
    }

  switch (property_id)
    {
    case CHILD_PROP_LEFT_ATTACH:
      b_value_set_uint (value, table_child->left_attach);
      break;
    case CHILD_PROP_RIGHT_ATTACH:
      b_value_set_uint (value, table_child->right_attach);
      break;
    case CHILD_PROP_TOP_ATTACH:
      b_value_set_uint (value, table_child->top_attach);
      break;
    case CHILD_PROP_BOTTOM_ATTACH:
      b_value_set_uint (value, table_child->bottom_attach);
      break;
    case CHILD_PROP_X_OPTIONS:
      b_value_set_flags (value, (table_child->xexpand * BTK_EXPAND |
				 table_child->xshrink * BTK_SHRINK |
				 table_child->xfill * BTK_FILL));
      break;
    case CHILD_PROP_Y_OPTIONS:
      b_value_set_flags (value, (table_child->yexpand * BTK_EXPAND |
				 table_child->yshrink * BTK_SHRINK |
				 table_child->yfill * BTK_FILL));
      break;
    case CHILD_PROP_X_PADDING:
      b_value_set_uint (value, table_child->xpadding);
      break;
    case CHILD_PROP_Y_PADDING:
      b_value_set_uint (value, table_child->ypadding);
      break;
    default:
      BTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
btk_table_init (BtkTable *table)
{
  btk_widget_set_has_window (BTK_WIDGET (table), FALSE);
  btk_widget_set_redraw_on_allocate (BTK_WIDGET (table), FALSE);
  
  table->children = NULL;
  table->rows = NULL;
  table->cols = NULL;
  table->nrows = 0;
  table->ncols = 0;
  table->column_spacing = 0;
  table->row_spacing = 0;
  table->homogeneous = FALSE;

  btk_table_resize (table, 1, 1);
}

BtkWidget*
btk_table_new (buint	rows,
	       buint	columns,
	       bboolean homogeneous)
{
  BtkTable *table;

  if (rows == 0)
    rows = 1;
  if (columns == 0)
    columns = 1;
  
  table = g_object_new (BTK_TYPE_TABLE, NULL);
  
  table->homogeneous = (homogeneous ? TRUE : FALSE);

  btk_table_resize (table, rows, columns);
  
  return BTK_WIDGET (table);
}

void
btk_table_resize (BtkTable *table,
		  buint     n_rows,
		  buint     n_cols)
{
  g_return_if_fail (BTK_IS_TABLE (table));
  g_return_if_fail (n_rows > 0 && n_rows <= 65535);
  g_return_if_fail (n_cols > 0 && n_cols <= 65535);

  n_rows = MAX (n_rows, 1);
  n_cols = MAX (n_cols, 1);

  if (n_rows != table->nrows ||
      n_cols != table->ncols)
    {
      GList *list;
      
      for (list = table->children; list; list = list->next)
	{
	  BtkTableChild *child;
	  
	  child = list->data;
	  
	  n_rows = MAX (n_rows, child->bottom_attach);
	  n_cols = MAX (n_cols, child->right_attach);
	}
      
      if (n_rows != table->nrows)
	{
	  buint i;

	  i = table->nrows;
	  table->nrows = n_rows;
	  table->rows = g_realloc (table->rows, table->nrows * sizeof (BtkTableRowCol));
	  
	  for (; i < table->nrows; i++)
	    {
	      table->rows[i].requisition = 0;
	      table->rows[i].allocation = 0;
	      table->rows[i].spacing = table->row_spacing;
	      table->rows[i].need_expand = 0;
	      table->rows[i].need_shrink = 0;
	      table->rows[i].expand = 0;
	      table->rows[i].shrink = 0;
	    }

	  g_object_notify (B_OBJECT (table), "n-rows");
	}

      if (n_cols != table->ncols)
	{
	  buint i;

	  i = table->ncols;
	  table->ncols = n_cols;
	  table->cols = g_realloc (table->cols, table->ncols * sizeof (BtkTableRowCol));
	  
	  for (; i < table->ncols; i++)
	    {
	      table->cols[i].requisition = 0;
	      table->cols[i].allocation = 0;
	      table->cols[i].spacing = table->column_spacing;
	      table->cols[i].need_expand = 0;
	      table->cols[i].need_shrink = 0;
	      table->cols[i].expand = 0;
	      table->cols[i].shrink = 0;
	    }

	  g_object_notify (B_OBJECT (table), "n-columns");
	}
    }
}

void
btk_table_attach (BtkTable	  *table,
		  BtkWidget	  *child,
		  buint		   left_attach,
		  buint		   right_attach,
		  buint		   top_attach,
		  buint		   bottom_attach,
		  BtkAttachOptions xoptions,
		  BtkAttachOptions yoptions,
		  buint		   xpadding,
		  buint		   ypadding)
{
  BtkTableChild *table_child;
  
  g_return_if_fail (BTK_IS_TABLE (table));
  g_return_if_fail (BTK_IS_WIDGET (child));
  g_return_if_fail (child->parent == NULL);
  
  /* g_return_if_fail (left_attach >= 0); */
  g_return_if_fail (left_attach < right_attach);
  /* g_return_if_fail (top_attach >= 0); */
  g_return_if_fail (top_attach < bottom_attach);
  
  if (right_attach >= table->ncols)
    btk_table_resize (table, table->nrows, right_attach);
  
  if (bottom_attach >= table->nrows)
    btk_table_resize (table, bottom_attach, table->ncols);
  
  table_child = g_new (BtkTableChild, 1);
  table_child->widget = child;
  table_child->left_attach = left_attach;
  table_child->right_attach = right_attach;
  table_child->top_attach = top_attach;
  table_child->bottom_attach = bottom_attach;
  table_child->xexpand = (xoptions & BTK_EXPAND) != 0;
  table_child->xshrink = (xoptions & BTK_SHRINK) != 0;
  table_child->xfill = (xoptions & BTK_FILL) != 0;
  table_child->xpadding = xpadding;
  table_child->yexpand = (yoptions & BTK_EXPAND) != 0;
  table_child->yshrink = (yoptions & BTK_SHRINK) != 0;
  table_child->yfill = (yoptions & BTK_FILL) != 0;
  table_child->ypadding = ypadding;
  
  table->children = g_list_prepend (table->children, table_child);
  
  btk_widget_set_parent (child, BTK_WIDGET (table));
}

void
btk_table_attach_defaults (BtkTable  *table,
			   BtkWidget *widget,
			   buint      left_attach,
			   buint      right_attach,
			   buint      top_attach,
			   buint      bottom_attach)
{
  btk_table_attach (table, widget,
		    left_attach, right_attach,
		    top_attach, bottom_attach,
		    BTK_EXPAND | BTK_FILL,
		    BTK_EXPAND | BTK_FILL,
		    0, 0);
}

void
btk_table_set_row_spacing (BtkTable *table,
			   buint     row,
			   buint     spacing)
{
  g_return_if_fail (BTK_IS_TABLE (table));
  g_return_if_fail (row < table->nrows);
  
  if (table->rows[row].spacing != spacing)
    {
      table->rows[row].spacing = spacing;
      
      if (btk_widget_get_visible (BTK_WIDGET (table)))
	btk_widget_queue_resize (BTK_WIDGET (table));
    }
}

/**
 * btk_table_get_row_spacing:
 * @table: a #BtkTable
 * @row: a row in the table, 0 indicates the first row
 *
 * Gets the amount of space between row @row, and
 * row @row + 1. See btk_table_set_row_spacing().
 *
 * Return value: the row spacing
 **/
buint
btk_table_get_row_spacing (BtkTable *table,
			   buint     row)
{
  g_return_val_if_fail (BTK_IS_TABLE (table), 0);
  g_return_val_if_fail (row < table->nrows - 1, 0);
 
  return table->rows[row].spacing;
}

void
btk_table_set_col_spacing (BtkTable *table,
			   buint     column,
			   buint     spacing)
{
  g_return_if_fail (BTK_IS_TABLE (table));
  g_return_if_fail (column < table->ncols);
  
  if (table->cols[column].spacing != spacing)
    {
      table->cols[column].spacing = spacing;
      
      if (btk_widget_get_visible (BTK_WIDGET (table)))
	btk_widget_queue_resize (BTK_WIDGET (table));
    }
}

/**
 * btk_table_get_col_spacing:
 * @table: a #BtkTable
 * @column: a column in the table, 0 indicates the first column
 *
 * Gets the amount of space between column @col, and
 * column @col + 1. See btk_table_set_col_spacing().
 *
 * Return value: the column spacing
 **/
buint
btk_table_get_col_spacing (BtkTable *table,
			   buint     column)
{
  g_return_val_if_fail (BTK_IS_TABLE (table), 0);
  g_return_val_if_fail (column < table->ncols, 0);

  return table->cols[column].spacing;
}

void
btk_table_set_row_spacings (BtkTable *table,
			    buint     spacing)
{
  buint row;
  
  g_return_if_fail (BTK_IS_TABLE (table));
  
  table->row_spacing = spacing;
  for (row = 0; row < table->nrows; row++)
    table->rows[row].spacing = spacing;
  
  if (btk_widget_get_visible (BTK_WIDGET (table)))
    btk_widget_queue_resize (BTK_WIDGET (table));

  g_object_notify (B_OBJECT (table), "row-spacing");
}

/**
 * btk_table_get_default_row_spacing:
 * @table: a #BtkTable
 *
 * Gets the default row spacing for the table. This is
 * the spacing that will be used for newly added rows.
 * (See btk_table_set_row_spacings())
 *
 * Return value: the default row spacing
 **/
buint
btk_table_get_default_row_spacing (BtkTable *table)
{
  g_return_val_if_fail (BTK_IS_TABLE (table), 0);

  return table->row_spacing;
}

void
btk_table_set_col_spacings (BtkTable *table,
			    buint     spacing)
{
  buint col;
  
  g_return_if_fail (BTK_IS_TABLE (table));
  
  table->column_spacing = spacing;
  for (col = 0; col < table->ncols; col++)
    table->cols[col].spacing = spacing;
  
  if (btk_widget_get_visible (BTK_WIDGET (table)))
    btk_widget_queue_resize (BTK_WIDGET (table));

  g_object_notify (B_OBJECT (table), "column-spacing");
}

/**
 * btk_table_get_default_col_spacing:
 * @table: a #BtkTable
 *
 * Gets the default column spacing for the table. This is
 * the spacing that will be used for newly added columns.
 * (See btk_table_set_col_spacings())
 *
 * Return value: the default column spacing
 **/
buint
btk_table_get_default_col_spacing (BtkTable *table)
{
  g_return_val_if_fail (BTK_IS_TABLE (table), 0);

  return table->column_spacing;
}

void
btk_table_set_homogeneous (BtkTable *table,
			   bboolean  homogeneous)
{
  g_return_if_fail (BTK_IS_TABLE (table));

  homogeneous = (homogeneous != 0);
  if (homogeneous != table->homogeneous)
    {
      table->homogeneous = homogeneous;
      
      if (btk_widget_get_visible (BTK_WIDGET (table)))
	btk_widget_queue_resize (BTK_WIDGET (table));

      g_object_notify (B_OBJECT (table), "homogeneous");
    }
}

/**
 * btk_table_get_homogeneous:
 * @table: a #BtkTable
 *
 * Returns whether the table cells are all constrained to the same
 * width and height. (See btk_table_set_homogenous ())
 *
 * Return value: %TRUE if the cells are all constrained to the same size
 **/
bboolean
btk_table_get_homogeneous (BtkTable *table)
{
  g_return_val_if_fail (BTK_IS_TABLE (table), FALSE);

  return table->homogeneous;
}

/**
 * btk_table_get_size:
 * @table: a #BtkTable
 * @rows: (out) (allow-none): return location for the number of
 *   rows, or %NULL
 * @columns: (out) (allow-none): return location for the number
 *   of columns, or %NULL
 *
 * Returns the number of rows and columns in the table.
 *
 * Since: 2.22
 **/
void
btk_table_get_size (BtkTable *table,
                    buint    *rows,
                    buint    *columns)
{
  g_return_if_fail (BTK_IS_TABLE (table));

  if (rows)
    *rows = table->nrows;

  if (columns)
    *columns = table->ncols;
}

static void
btk_table_finalize (BObject *object)
{
  BtkTable *table = BTK_TABLE (object);

  g_free (table->rows);
  g_free (table->cols);
  
  B_OBJECT_CLASS (btk_table_parent_class)->finalize (object);
}

static void
btk_table_size_request (BtkWidget      *widget,
			BtkRequisition *requisition)
{
  BtkTable *table = BTK_TABLE (widget);
  bint row, col;

  requisition->width = 0;
  requisition->height = 0;
  
  btk_table_size_request_init (table);
  btk_table_size_request_pass1 (table);
  btk_table_size_request_pass2 (table);
  btk_table_size_request_pass3 (table);
  btk_table_size_request_pass2 (table);
  
  for (col = 0; col < table->ncols; col++)
    requisition->width += table->cols[col].requisition;
  for (col = 0; col + 1 < table->ncols; col++)
    requisition->width += table->cols[col].spacing;
  
  for (row = 0; row < table->nrows; row++)
    requisition->height += table->rows[row].requisition;
  for (row = 0; row + 1 < table->nrows; row++)
    requisition->height += table->rows[row].spacing;
  
  requisition->width += BTK_CONTAINER (table)->border_width * 2;
  requisition->height += BTK_CONTAINER (table)->border_width * 2;
}

static void
btk_table_size_allocate (BtkWidget     *widget,
			 BtkAllocation *allocation)
{
  BtkTable *table = BTK_TABLE (widget);

  widget->allocation = *allocation;

  btk_table_size_allocate_init (table);
  btk_table_size_allocate_pass1 (table);
  btk_table_size_allocate_pass2 (table);
}

static void
btk_table_add (BtkContainer *container,
	       BtkWidget    *widget)
{
  btk_table_attach_defaults (BTK_TABLE (container), widget, 0, 1, 0, 1);
}

static void
btk_table_remove (BtkContainer *container,
		  BtkWidget    *widget)
{
  BtkTable *table = BTK_TABLE (container);
  BtkTableChild *child;
  BtkWidget *widget_container = BTK_WIDGET (container);
  GList *children;

  children = table->children;
  
  while (children)
    {
      child = children->data;
      children = children->next;
      
      if (child->widget == widget)
	{
	  bboolean was_visible = btk_widget_get_visible (widget);
	  
	  btk_widget_unparent (widget);
	  
	  table->children = g_list_remove (table->children, child);
	  g_free (child);
	  
	  if (was_visible && btk_widget_get_visible (widget_container))
	    btk_widget_queue_resize (widget_container);
	  break;
	}
    }
}

static void
btk_table_forall (BtkContainer *container,
		  bboolean	include_internals,
		  BtkCallback	callback,
		  bpointer	callback_data)
{
  BtkTable *table = BTK_TABLE (container);
  BtkTableChild *child;
  GList *children;

  children = table->children;
  
  while (children)
    {
      child = children->data;
      children = children->next;
      
      (* callback) (child->widget, callback_data);
    }
}

static void
btk_table_size_request_init (BtkTable *table)
{
  BtkTableChild *child;
  GList *children;
  bint row, col;
  
  for (row = 0; row < table->nrows; row++)
    {
      table->rows[row].requisition = 0;
      table->rows[row].expand = FALSE;
    }
  for (col = 0; col < table->ncols; col++)
    {
      table->cols[col].requisition = 0;
      table->cols[col].expand = FALSE;
    }
  
  children = table->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      
      if (btk_widget_get_visible (child->widget))
	btk_widget_size_request (child->widget, NULL);

      if (child->left_attach == (child->right_attach - 1) && child->xexpand)
	table->cols[child->left_attach].expand = TRUE;
      
      if (child->top_attach == (child->bottom_attach - 1) && child->yexpand)
	table->rows[child->top_attach].expand = TRUE;
    }
}

static void
btk_table_size_request_pass1 (BtkTable *table)
{
  BtkTableChild *child;
  GList *children;
  bint width;
  bint height;
  
  children = table->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      
      if (btk_widget_get_visible (child->widget))
	{
	  BtkRequisition child_requisition;
	  btk_widget_get_child_requisition (child->widget, &child_requisition);

	  /* Child spans a single column.
	   */
	  if (child->left_attach == (child->right_attach - 1))
	    {
	      width = child_requisition.width + child->xpadding * 2;
	      table->cols[child->left_attach].requisition = MAX (table->cols[child->left_attach].requisition, width);
	    }
	  
	  /* Child spans a single row.
	   */
	  if (child->top_attach == (child->bottom_attach - 1))
	    {
	      height = child_requisition.height + child->ypadding * 2;
	      table->rows[child->top_attach].requisition = MAX (table->rows[child->top_attach].requisition, height);
	    }
	}
    }
}

static void
btk_table_size_request_pass2 (BtkTable *table)
{
  bint max_width;
  bint max_height;
  bint row, col;
  
  if (table->homogeneous)
    {
      max_width = 0;
      max_height = 0;
      
      for (col = 0; col < table->ncols; col++)
	max_width = MAX (max_width, table->cols[col].requisition);
      for (row = 0; row < table->nrows; row++)
	max_height = MAX (max_height, table->rows[row].requisition);
      
      for (col = 0; col < table->ncols; col++)
	table->cols[col].requisition = max_width;
      for (row = 0; row < table->nrows; row++)
	table->rows[row].requisition = max_height;
    }
}

static void
btk_table_size_request_pass3 (BtkTable *table)
{
  BtkTableChild *child;
  GList *children;
  bint width, height;
  bint row, col;
  bint extra;
  
  children = table->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      
      if (btk_widget_get_visible (child->widget))
	{
	  /* Child spans multiple columns.
	   */
	  if (child->left_attach != (child->right_attach - 1))
	    {
	      BtkRequisition child_requisition;

	      btk_widget_get_child_requisition (child->widget, &child_requisition);
	      
	      /* Check and see if there is already enough space
	       *  for the child.
	       */
	      width = 0;
	      for (col = child->left_attach; col < child->right_attach; col++)
		{
		  width += table->cols[col].requisition;
		  if ((col + 1) < child->right_attach)
		    width += table->cols[col].spacing;
		}
	      
	      /* If we need to request more space for this child to fill
	       *  its requisition, then divide up the needed space amongst the
	       *  columns it spans, favoring expandable columns if any.
	       */
	      if (width < child_requisition.width + child->xpadding * 2)
		{
		  bint n_expand = 0;
		  bboolean force_expand = FALSE;
		  
		  width = child_requisition.width + child->xpadding * 2 - width;

		  for (col = child->left_attach; col < child->right_attach; col++)
		    if (table->cols[col].expand)
		      n_expand++;

		  if (n_expand == 0)
		    {
		      n_expand = (child->right_attach - child->left_attach);
		      force_expand = TRUE;
		    }
		    
		  for (col = child->left_attach; col < child->right_attach; col++)
		    if (force_expand || table->cols[col].expand)
		      {
			extra = width / n_expand;
			table->cols[col].requisition += extra;
			width -= extra;
			n_expand--;
		      }
		}
	    }
	  
	  /* Child spans multiple rows.
	   */
	  if (child->top_attach != (child->bottom_attach - 1))
	    {
	      BtkRequisition child_requisition;

	      btk_widget_get_child_requisition (child->widget, &child_requisition);

	      /* Check and see if there is already enough space
	       *  for the child.
	       */
	      height = 0;
	      for (row = child->top_attach; row < child->bottom_attach; row++)
		{
		  height += table->rows[row].requisition;
		  if ((row + 1) < child->bottom_attach)
		    height += table->rows[row].spacing;
		}
	      
	      /* If we need to request more space for this child to fill
	       *  its requisition, then divide up the needed space amongst the
	       *  rows it spans, favoring expandable rows if any.
	       */
	      if (height < child_requisition.height + child->ypadding * 2)
		{
		  bint n_expand = 0;
		  bboolean force_expand = FALSE;
		  
		  height = child_requisition.height + child->ypadding * 2 - height;
		  
		  for (row = child->top_attach; row < child->bottom_attach; row++)
		    {
		      if (table->rows[row].expand)
			n_expand++;
		    }

		  if (n_expand == 0)
		    {
		      n_expand = (child->bottom_attach - child->top_attach);
		      force_expand = TRUE;
		    }
		    
		  for (row = child->top_attach; row < child->bottom_attach; row++)
		    if (force_expand || table->rows[row].expand)
		      {
			extra = height / n_expand;
			table->rows[row].requisition += extra;
			height -= extra;
			n_expand--;
		      }
		}
	    }
	}
    }
}

static void
btk_table_size_allocate_init (BtkTable *table)
{
  BtkTableChild *child;
  GList *children;
  bint row, col;
  bint has_expand;
  bint has_shrink;
  
  /* Initialize the rows and cols.
   *  By default, rows and cols do not expand and do shrink.
   *  Those values are modified by the children that occupy
   *  the rows and cols.
   */
  for (col = 0; col < table->ncols; col++)
    {
      table->cols[col].allocation = table->cols[col].requisition;
      table->cols[col].need_expand = FALSE;
      table->cols[col].need_shrink = TRUE;
      table->cols[col].expand = FALSE;
      table->cols[col].shrink = TRUE;
      table->cols[col].empty = TRUE;
    }
  for (row = 0; row < table->nrows; row++)
    {
      table->rows[row].allocation = table->rows[row].requisition;
      table->rows[row].need_expand = FALSE;
      table->rows[row].need_shrink = TRUE;
      table->rows[row].expand = FALSE;
      table->rows[row].shrink = TRUE;
      table->rows[row].empty = TRUE;
    }
  
  /* Loop over all the children and adjust the row and col values
   *  based on whether the children want to be allowed to expand
   *  or shrink. This loop handles children that occupy a single
   *  row or column.
   */
  children = table->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      
      if (btk_widget_get_visible (child->widget))
	{
	  if (child->left_attach == (child->right_attach - 1))
	    {
	      if (child->xexpand)
		table->cols[child->left_attach].expand = TRUE;
	      
	      if (!child->xshrink)
		table->cols[child->left_attach].shrink = FALSE;
	      
	      table->cols[child->left_attach].empty = FALSE;
	    }
	  
	  if (child->top_attach == (child->bottom_attach - 1))
	    {
	      if (child->yexpand)
		table->rows[child->top_attach].expand = TRUE;
	      
	      if (!child->yshrink)
		table->rows[child->top_attach].shrink = FALSE;

	      table->rows[child->top_attach].empty = FALSE;
	    }
	}
    }
  
  /* Loop over all the children again and this time handle children
   *  which span multiple rows or columns.
   */
  children = table->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      
      if (btk_widget_get_visible (child->widget))
	{
	  if (child->left_attach != (child->right_attach - 1))
	    {
	      for (col = child->left_attach; col < child->right_attach; col++)
		table->cols[col].empty = FALSE;

	      if (child->xexpand)
		{
		  has_expand = FALSE;
		  for (col = child->left_attach; col < child->right_attach; col++)
		    if (table->cols[col].expand)
		      {
			has_expand = TRUE;
			break;
		      }
		  
		  if (!has_expand)
		    for (col = child->left_attach; col < child->right_attach; col++)
		      table->cols[col].need_expand = TRUE;
		}
	      
	      if (!child->xshrink)
		{
		  has_shrink = TRUE;
		  for (col = child->left_attach; col < child->right_attach; col++)
		    if (!table->cols[col].shrink)
		      {
			has_shrink = FALSE;
			break;
		      }
		  
		  if (has_shrink)
		    for (col = child->left_attach; col < child->right_attach; col++)
		      table->cols[col].need_shrink = FALSE;
		}
	    }
	  
	  if (child->top_attach != (child->bottom_attach - 1))
	    {
	      for (row = child->top_attach; row < child->bottom_attach; row++)
		table->rows[row].empty = FALSE;

	      if (child->yexpand)
		{
		  has_expand = FALSE;
		  for (row = child->top_attach; row < child->bottom_attach; row++)
		    if (table->rows[row].expand)
		      {
			has_expand = TRUE;
			break;
		      }
		  
		  if (!has_expand)
		    for (row = child->top_attach; row < child->bottom_attach; row++)
		      table->rows[row].need_expand = TRUE;
		}
	      
	      if (!child->yshrink)
		{
		  has_shrink = TRUE;
		  for (row = child->top_attach; row < child->bottom_attach; row++)
		    if (!table->rows[row].shrink)
		      {
			has_shrink = FALSE;
			break;
		      }
		  
		  if (has_shrink)
		    for (row = child->top_attach; row < child->bottom_attach; row++)
		      table->rows[row].need_shrink = FALSE;
		}
	    }
	}
    }
  
  /* Loop over the columns and set the expand and shrink values
   *  if the column can be expanded or shrunk.
   */
  for (col = 0; col < table->ncols; col++)
    {
      if (table->cols[col].empty)
	{
	  table->cols[col].expand = FALSE;
	  table->cols[col].shrink = FALSE;
	}
      else
	{
	  if (table->cols[col].need_expand)
	    table->cols[col].expand = TRUE;
	  if (!table->cols[col].need_shrink)
	    table->cols[col].shrink = FALSE;
	}
    }
  
  /* Loop over the rows and set the expand and shrink values
   *  if the row can be expanded or shrunk.
   */
  for (row = 0; row < table->nrows; row++)
    {
      if (table->rows[row].empty)
	{
	  table->rows[row].expand = FALSE;
	  table->rows[row].shrink = FALSE;
	}
      else
	{
	  if (table->rows[row].need_expand)
	    table->rows[row].expand = TRUE;
	  if (!table->rows[row].need_shrink)
	    table->rows[row].shrink = FALSE;
	}
    }
}

static void
btk_table_size_allocate_pass1 (BtkTable *table)
{
  bint real_width;
  bint real_height;
  bint width, height;
  bint row, col;
  bint nexpand;
  bint nshrink;
  bint extra;
  
  /* If we were allocated more space than we requested
   *  then we have to expand any expandable rows and columns
   *  to fill in the extra space.
   */
  
  real_width = BTK_WIDGET (table)->allocation.width - BTK_CONTAINER (table)->border_width * 2;
  real_height = BTK_WIDGET (table)->allocation.height - BTK_CONTAINER (table)->border_width * 2;
  
  if (table->homogeneous)
    {
      if (!table->children)
	nexpand = 1;
      else
	{
	  nexpand = 0;
	  for (col = 0; col < table->ncols; col++)
	    if (table->cols[col].expand)
	      {
		nexpand += 1;
		break;
	      }
	}
      if (nexpand)
	{
	  width = real_width;
	  for (col = 0; col + 1 < table->ncols; col++)
	    width -= table->cols[col].spacing;
	  
	  for (col = 0; col < table->ncols; col++)
	    {
	      extra = width / (table->ncols - col);
	      table->cols[col].allocation = MAX (1, extra);
	      width -= extra;
	    }
	}
    }
  else
    {
      width = 0;
      nexpand = 0;
      nshrink = 0;
      
      for (col = 0; col < table->ncols; col++)
	{
	  width += table->cols[col].requisition;
	  if (table->cols[col].expand)
	    nexpand += 1;
	  if (table->cols[col].shrink)
	    nshrink += 1;
	}
      for (col = 0; col + 1 < table->ncols; col++)
	width += table->cols[col].spacing;
      
      /* Check to see if we were allocated more width than we requested.
       */
      if ((width < real_width) && (nexpand >= 1))
	{
	  width = real_width - width;
	  
	  for (col = 0; col < table->ncols; col++)
	    if (table->cols[col].expand)
	      {
		extra = width / nexpand;
		table->cols[col].allocation += extra;
		
		width -= extra;
		nexpand -= 1;
	      }
	}
      
      /* Check to see if we were allocated less width than we requested,
       * then shrink until we fit the size give.
       */
      if (width > real_width)
	{
	  bint total_nshrink = nshrink;

	  extra = width - real_width;
	  while (total_nshrink > 0 && extra > 0)
	    {
	      nshrink = total_nshrink;
	      for (col = 0; col < table->ncols; col++)
		if (table->cols[col].shrink)
		  {
		    bint allocation = table->cols[col].allocation;

		    table->cols[col].allocation = MAX (1, (bint) table->cols[col].allocation - extra / nshrink);
		    extra -= allocation - table->cols[col].allocation;
		    nshrink -= 1;
		    if (table->cols[col].allocation < 2)
		      {
			total_nshrink -= 1;
			table->cols[col].shrink = FALSE;
		      }
		  }
	    }
	}
    }
  
  if (table->homogeneous)
    {
      if (!table->children)
	nexpand = 1;
      else
	{
	  nexpand = 0;
	  for (row = 0; row < table->nrows; row++)
	    if (table->rows[row].expand)
	      {
		nexpand += 1;
		break;
	      }
	}
      if (nexpand)
	{
	  height = real_height;
	  
	  for (row = 0; row + 1 < table->nrows; row++)
	    height -= table->rows[row].spacing;
	  
	  
	  for (row = 0; row < table->nrows; row++)
	    {
	      extra = height / (table->nrows - row);
	      table->rows[row].allocation = MAX (1, extra);
	      height -= extra;
	    }
	}
    }
  else
    {
      height = 0;
      nexpand = 0;
      nshrink = 0;
      
      for (row = 0; row < table->nrows; row++)
	{
	  height += table->rows[row].requisition;
	  if (table->rows[row].expand)
	    nexpand += 1;
	  if (table->rows[row].shrink)
	    nshrink += 1;
	}
      for (row = 0; row + 1 < table->nrows; row++)
	height += table->rows[row].spacing;
      
      /* Check to see if we were allocated more height than we requested.
       */
      if ((height < real_height) && (nexpand >= 1))
	{
	  height = real_height - height;
	  
	  for (row = 0; row < table->nrows; row++)
	    if (table->rows[row].expand)
	      {
		extra = height / nexpand;
		table->rows[row].allocation += extra;
		
		height -= extra;
		nexpand -= 1;
	      }
	}
      
      /* Check to see if we were allocated less height than we requested.
       * then shrink until we fit the size give.
       */
      if (height > real_height)
	{
	  bint total_nshrink = nshrink;
	  
	  extra = height - real_height;
	  while (total_nshrink > 0 && extra > 0)
	    {
	      nshrink = total_nshrink;
	      for (row = 0; row < table->nrows; row++)
		if (table->rows[row].shrink)
		  {
		    bint allocation = table->rows[row].allocation;
		    
		    table->rows[row].allocation = MAX (1, (bint) table->rows[row].allocation - extra / nshrink);
		    extra -= allocation - table->rows[row].allocation;
		    nshrink -= 1;
		    if (table->rows[row].allocation < 2)
		      {
			total_nshrink -= 1;
			table->rows[row].shrink = FALSE;
		      }
		  }
	    }
	}
    }
}

static void
btk_table_size_allocate_pass2 (BtkTable *table)
{
  BtkTableChild *child;
  GList *children;
  bint max_width;
  bint max_height;
  bint x, y;
  bint row, col;
  BtkAllocation allocation;
  BtkWidget *widget = BTK_WIDGET (table);
  
  children = table->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      
      if (btk_widget_get_visible (child->widget))
	{
	  BtkRequisition child_requisition;
	  btk_widget_get_child_requisition (child->widget, &child_requisition);

	  x = BTK_WIDGET (table)->allocation.x + BTK_CONTAINER (table)->border_width;
	  y = BTK_WIDGET (table)->allocation.y + BTK_CONTAINER (table)->border_width;
	  max_width = 0;
	  max_height = 0;
	  
	  for (col = 0; col < child->left_attach; col++)
	    {
	      x += table->cols[col].allocation;
	      x += table->cols[col].spacing;
	    }
	  
	  for (col = child->left_attach; col < child->right_attach; col++)
	    {
	      max_width += table->cols[col].allocation;
	      if ((col + 1) < child->right_attach)
		max_width += table->cols[col].spacing;
	    }
	  
	  for (row = 0; row < child->top_attach; row++)
	    {
	      y += table->rows[row].allocation;
	      y += table->rows[row].spacing;
	    }
	  
	  for (row = child->top_attach; row < child->bottom_attach; row++)
	    {
	      max_height += table->rows[row].allocation;
	      if ((row + 1) < child->bottom_attach)
		max_height += table->rows[row].spacing;
	    }
	  
	  if (child->xfill)
	    {
	      allocation.width = MAX (1, max_width - (bint)child->xpadding * 2);
	      allocation.x = x + (max_width - allocation.width) / 2;
	    }
	  else
	    {
	      allocation.width = child_requisition.width;
	      allocation.x = x + (max_width - allocation.width) / 2;
	    }
	  
	  if (child->yfill)
	    {
	      allocation.height = MAX (1, max_height - (bint)child->ypadding * 2);
	      allocation.y = y + (max_height - allocation.height) / 2;
	    }
	  else
	    {
	      allocation.height = child_requisition.height;
	      allocation.y = y + (max_height - allocation.height) / 2;
	    }

	  if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL)
	    allocation.x = widget->allocation.x + widget->allocation.width
	      - (allocation.x - widget->allocation.x) - allocation.width;
	  
	  btk_widget_size_allocate (child->widget, &allocation);
	}
    }
}

#define __BTK_TABLE_C__
#include "btkaliasdef.c"
