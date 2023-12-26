/* btkcelleditable.c
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
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
#include "btkcelleditable.h"
#include "btkmarshalers.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

typedef BtkCellEditableIface BtkCellEditableInterface;
G_DEFINE_INTERFACE(BtkCellEditable, btk_cell_editable, BTK_TYPE_WIDGET)

static void
btk_cell_editable_default_init (BtkCellEditableInterface *iface)
{
  /**
   * BtkCellEditable:editing-canceled:
   *
   * Indicates whether editing on the cell has been canceled.
   *
   * Since: 2.20
   */
  g_object_interface_install_property (iface,
                                       g_param_spec_boolean ("editing-canceled",
                                       P_("Editing Canceled"),
                                       P_("Indicates that editing has been canceled"),
                                       FALSE,
                                       BTK_PARAM_READWRITE));

  /**
   * BtkCellEditable::editing-done:
   * @cell_editable: the object on which the signal was emitted
   *
   * This signal is a sign for the cell renderer to update its
   * value from the @cell_editable.
   *
   * Implementations of #BtkCellEditable are responsible for
   * emitting this signal when they are done editing, e.g.
   * #BtkEntry is emitting it when the user presses Enter.
   *
   * btk_cell_editable_editing_done() is a convenience method
   * for emitting BtkCellEditable::editing-done.
   */
  g_signal_new (I_("editing-done"),
                BTK_TYPE_CELL_EDITABLE,
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (BtkCellEditableIface, editing_done),
                NULL, NULL,
                _btk_marshal_VOID__VOID,
                B_TYPE_NONE, 0);

  /**
   * BtkCellEditable::remove-widget:
   * @cell_editable: the object on which the signal was emitted
   *
   * This signal is meant to indicate that the cell is finished
   * editing, and the widget may now be destroyed.
   *
   * Implementations of #BtkCellEditable are responsible for
   * emitting this signal when they are done editing. It must
   * be emitted after the #BtkCellEditable::editing-done signal,
   * to give the cell renderer a chance to update the cell's value
   * before the widget is removed.
   *
   * btk_cell_editable_remove_widget() is a convenience method
   * for emitting BtkCellEditable::remove-widget.
   */
  g_signal_new (I_("remove-widget"),
                BTK_TYPE_CELL_EDITABLE,
                G_SIGNAL_RUN_LAST,
                G_STRUCT_OFFSET (BtkCellEditableIface, remove_widget),
                NULL, NULL,
                _btk_marshal_VOID__VOID,
                B_TYPE_NONE, 0);
}

/**
 * btk_cell_editable_start_editing:
 * @cell_editable: A #BtkCellEditable
 * @event: (allow-none): A #BdkEvent, or %NULL
 * 
 * Begins editing on a @cell_editable. @event is the #BdkEvent that began 
 * the editing process. It may be %NULL, in the instance that editing was 
 * initiated through programatic means.
 **/
void
btk_cell_editable_start_editing (BtkCellEditable *cell_editable,
				 BdkEvent        *event)
{
  g_return_if_fail (BTK_IS_CELL_EDITABLE (cell_editable));

  (* BTK_CELL_EDITABLE_GET_IFACE (cell_editable)->start_editing) (cell_editable, event);
}

/**
 * btk_cell_editable_editing_done:
 * @cell_editable: A #BtkTreeEditable
 * 
 * Emits the #BtkCellEditable::editing-done signal. 
 **/
void
btk_cell_editable_editing_done (BtkCellEditable *cell_editable)
{
  g_return_if_fail (BTK_IS_CELL_EDITABLE (cell_editable));

  g_signal_emit_by_name (cell_editable, "editing-done");
}

/**
 * btk_cell_editable_remove_widget:
 * @cell_editable: A #BtkTreeEditable
 * 
 * Emits the #BtkCellEditable::remove-widget signal.  
 **/
void
btk_cell_editable_remove_widget (BtkCellEditable *cell_editable)
{
  g_return_if_fail (BTK_IS_CELL_EDITABLE (cell_editable));

  g_signal_emit_by_name (cell_editable, "remove-widget");
}

#define __BTK_CELL_EDITABLE_C__
#include "btkaliasdef.c"
