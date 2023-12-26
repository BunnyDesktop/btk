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
#define BTK_TABLE(obj)			(B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TABLE, BtkTable))
#define BTK_TABLE_CLASS(klass)		(B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TABLE, BtkTableClass))
#define BTK_IS_TABLE(obj)		(B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TABLE))
#define BTK_IS_TABLE_CLASS(klass)	(B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TABLE))
#define BTK_TABLE_GET_CLASS(obj)        (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TABLE, BtkTableClass))


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
  buint16 GSEAL (nrows);
  buint16 GSEAL (ncols);
  buint16 GSEAL (column_spacing);
  buint16 GSEAL (row_spacing);
  buint GSEAL (homogeneous) : 1;
};

struct _BtkTableClass
{
  BtkContainerClass parent_class;
};

struct _BtkTableChild
{
  BtkWidget *widget;
  buint16 left_attach;
  buint16 right_attach;
  buint16 top_attach;
  buint16 bottom_attach;
  buint16 xpadding;
  buint16 ypadding;
  buint xexpand : 1;
  buint yexpand : 1;
  buint xshrink : 1;
  buint yshrink : 1;
  buint xfill : 1;
  buint yfill : 1;
};

struct _BtkTableRowCol
{
  buint16 requisition;
  buint16 allocation;
  buint16 spacing;
  buint need_expand : 1;
  buint need_shrink : 1;
  buint expand : 1;
  buint shrink : 1;
  buint empty : 1;
};


GType	   btk_table_get_type	      (void) B_GNUC_CONST;
BtkWidget* btk_table_new	      (buint		rows,
				       buint		columns,
				       bboolean		homogeneous);
void	   btk_table_resize	      (BtkTable	       *table,
				       buint            rows,
				       buint            columns);
void	   btk_table_attach	      (BtkTable	       *table,
				       BtkWidget       *child,
				       buint		left_attach,
				       buint		right_attach,
				       buint		top_attach,
				       buint		bottom_attach,
				       BtkAttachOptions xoptions,
				       BtkAttachOptions yoptions,
				       buint		xpadding,
				       buint		ypadding);
void	   btk_table_attach_defaults  (BtkTable	       *table,
				       BtkWidget       *widget,
				       buint		left_attach,
				       buint		right_attach,
				       buint		top_attach,
				       buint		bottom_attach);
void	   btk_table_set_row_spacing  (BtkTable	       *table,
				       buint		row,
				       buint		spacing);
buint      btk_table_get_row_spacing  (BtkTable        *table,
				       buint            row);
void	   btk_table_set_col_spacing  (BtkTable	       *table,
				       buint		column,
				       buint		spacing);
buint      btk_table_get_col_spacing  (BtkTable        *table,
				       buint            column);
void	   btk_table_set_row_spacings (BtkTable	       *table,
				       buint		spacing);
buint      btk_table_get_default_row_spacing (BtkTable        *table);
void	   btk_table_set_col_spacings (BtkTable	       *table,
				       buint		spacing);
buint      btk_table_get_default_col_spacing (BtkTable        *table);
void	   btk_table_set_homogeneous  (BtkTable	       *table,
				       bboolean		homogeneous);
bboolean   btk_table_get_homogeneous  (BtkTable        *table);
void       btk_table_get_size         (BtkTable        *table,
                                       buint           *rows,
                                       buint           *columns);


B_END_DECLS

#endif /* __BTK_TABLE_H__ */
