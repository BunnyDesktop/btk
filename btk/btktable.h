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

#ifndef __BTK_TABLE_H__
#define __BTK_TABLE_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcontainer.h>


B_BEGIN_DECLS

#define BTK_TYPE_TABLE			(btk_table_get_type ())
#define BTK_TABLE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TABLE, BtkTable))
#define BTK_TABLE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TABLE, BtkTableClass))
#define BTK_IS_TABLE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TABLE))
#define BTK_IS_TABLE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TABLE))
#define BTK_TABLE_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TABLE, BtkTableClass))


typedef struct _BtkTable	BtkTable;
typedef struct _BtkTableClass	BtkTableClass;
typedef struct _BtkTableChild	BtkTableChild;
typedef struct _BtkTableRowCol	BtkTableRowCol;

struct _BtkTable
{
  BtkContainer container;

  GList *GSEAL (children);
  BtkTableRowCol *GSEAL (rows);
  BtkTableRowCol *GSEAL (cols);
  guint16 GSEAL (nrows);
  guint16 GSEAL (ncols);
  guint16 GSEAL (column_spacing);
  guint16 GSEAL (row_spacing);
  guint GSEAL (homogeneous) : 1;
};

struct _BtkTableClass
{
  BtkContainerClass parent_class;
};

struct _BtkTableChild
{
  BtkWidget *widget;
  guint16 left_attach;
  guint16 right_attach;
  guint16 top_attach;
  guint16 bottom_attach;
  guint16 xpadding;
  guint16 ypadding;
  guint xexpand : 1;
  guint yexpand : 1;
  guint xshrink : 1;
  guint yshrink : 1;
  guint xfill : 1;
  guint yfill : 1;
};

struct _BtkTableRowCol
{
  guint16 requisition;
  guint16 allocation;
  guint16 spacing;
  guint need_expand : 1;
  guint need_shrink : 1;
  guint expand : 1;
  guint shrink : 1;
  guint empty : 1;
};


GType	   btk_table_get_type	      (void) B_GNUC_CONST;
BtkWidget* btk_table_new	      (guint		rows,
				       guint		columns,
				       gboolean		homogeneous);
void	   btk_table_resize	      (BtkTable	       *table,
				       guint            rows,
				       guint            columns);
void	   btk_table_attach	      (BtkTable	       *table,
				       BtkWidget       *child,
				       guint		left_attach,
				       guint		right_attach,
				       guint		top_attach,
				       guint		bottom_attach,
				       BtkAttachOptions xoptions,
				       BtkAttachOptions yoptions,
				       guint		xpadding,
				       guint		ypadding);
void	   btk_table_attach_defaults  (BtkTable	       *table,
				       BtkWidget       *widget,
				       guint		left_attach,
				       guint		right_attach,
				       guint		top_attach,
				       guint		bottom_attach);
void	   btk_table_set_row_spacing  (BtkTable	       *table,
				       guint		row,
				       guint		spacing);
guint      btk_table_get_row_spacing  (BtkTable        *table,
				       guint            row);
void	   btk_table_set_col_spacing  (BtkTable	       *table,
				       guint		column,
				       guint		spacing);
guint      btk_table_get_col_spacing  (BtkTable        *table,
				       guint            column);
void	   btk_table_set_row_spacings (BtkTable	       *table,
				       guint		spacing);
guint      btk_table_get_default_row_spacing (BtkTable        *table);
void	   btk_table_set_col_spacings (BtkTable	       *table,
				       guint		spacing);
guint      btk_table_get_default_col_spacing (BtkTable        *table);
void	   btk_table_set_homogeneous  (BtkTable	       *table,
				       gboolean		homogeneous);
gboolean   btk_table_get_homogeneous  (BtkTable        *table);
void       btk_table_get_size         (BtkTable        *table,
                                       guint           *rows,
                                       guint           *columns);


B_END_DECLS

#endif /* __BTK_TABLE_H__ */
