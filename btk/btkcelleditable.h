/* btkcelleditable.h
 * Copyright (C) 2001  Red Hat, Inc.
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

#ifndef __BTK_CELL_EDITABLE_H__
#define __BTK_CELL_EDITABLE_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkwidget.h>

G_BEGIN_DECLS

#define BTK_TYPE_CELL_EDITABLE            (btk_cell_editable_get_type ())
#define BTK_CELL_EDITABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_CELL_EDITABLE, BtkCellEditable))
#define BTK_CELL_EDITABLE_CLASS(obj)      (G_TYPE_CHECK_CLASS_CAST ((obj), BTK_TYPE_CELL_EDITABLE, BtkCellEditableIface))
#define BTK_IS_CELL_EDITABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_CELL_EDITABLE))
#define BTK_CELL_EDITABLE_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BTK_TYPE_CELL_EDITABLE, BtkCellEditableIface))

typedef struct _BtkCellEditable      BtkCellEditable; /* Dummy typedef */
typedef struct _BtkCellEditableIface BtkCellEditableIface;

struct _BtkCellEditableIface
{
  GTypeInterface g_iface;

  /* signals */
  void (* editing_done)  (BtkCellEditable *cell_editable);
  void (* remove_widget) (BtkCellEditable *cell_editable);

  /* virtual table */
  void (* start_editing) (BtkCellEditable *cell_editable,
			  BdkEvent        *event);
};


GType btk_cell_editable_get_type      (void) G_GNUC_CONST;

void  btk_cell_editable_start_editing (BtkCellEditable *cell_editable,
				       BdkEvent        *event);
void  btk_cell_editable_editing_done  (BtkCellEditable *cell_editable);
void  btk_cell_editable_remove_widget (BtkCellEditable *cell_editable);


G_END_DECLS

#endif /* __BTK_CELL_EDITABLE_H__ */
