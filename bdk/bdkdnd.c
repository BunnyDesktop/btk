/* BDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1999 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"
#include <bdkdnd.h>
#include <bdkdrawable.h>
#include <bdkdisplay.h>
#include "bdkalias.h"

/**
 * bdk_drag_find_window:
 * @context: a #BdkDragContext.
 * @drag_window: a window which may be at the pointer position, but
 *      should be ignored, since it is put up by the drag source as an icon.
 * @x_root: the x position of the pointer in root coordinates.
 * @y_root: the y position of the pointer in root coordinates.
 * @dest_window: (out): location to store the destination window in.
 * @protocol: (out): location to store the DND protocol in.
 *
 * Finds the destination window and DND protocol to use at the
 * given pointer position.
 *
 * This function is called by the drag source to obtain the 
 * @dest_window and @protocol parameters for bdk_drag_motion().
 *
 * Deprecated: 2.24: Use bdk_drag_find_window_for_screen() instead.
 **/
void
bdk_drag_find_window (BdkDragContext  *context,
		      BdkWindow       *drag_window,
		      bint             x_root,
		      bint             y_root,
		      BdkWindow      **dest_window,
		      BdkDragProtocol *protocol)
{
  bdk_drag_find_window_for_screen (context, drag_window,
				   bdk_drawable_get_screen (context->source_window),
				   x_root, y_root, dest_window, protocol);
}

/**
 * bdk_drag_get_protocol:
 * @xid: the windowing system id of the destination window.
 * @protocol: location where the supported DND protocol is returned.
 * 
 * Finds out the DND protocol supported by a window.
 * 
 * Return value: the windowing system specific id for the window where
 *    the drop should happen. This may be @xid or the id of a proxy
 *    window, or zero if @xid doesn't support Drag and Drop.
 *
 * Deprecated: 2.24: Use bdk_drag_get_protocol_for_display() instead
 **/
BdkNativeWindow
bdk_drag_get_protocol (BdkNativeWindow  xid,
		       BdkDragProtocol *protocol)
{
  return bdk_drag_get_protocol_for_display (bdk_display_get_default (), xid, protocol);
}

/**
 * bdk_drag_context_list_targets:
 * @context: a #BdkDragContext
 *
 * Retrieves the list of targets of the context.
 *
 * Return value: (transfer none) (element-type BdkAtom): a #GList of targets
 *
 * Since: 2.22
 **/
GList *
bdk_drag_context_list_targets (BdkDragContext *context)
{
  g_return_val_if_fail (BDK_IS_DRAG_CONTEXT (context), NULL);

  return context->targets;
}

/**
 * bdk_drag_context_get_actions:
 * @context: a #BdkDragContext
 *
 * Determines the bitmask of actions proposed by the source if
 * bdk_drag_context_suggested_action() returns BDK_ACTION_ASK.
 *
 * Return value: the #BdkDragAction flags
 *
 * Since: 2.22
 **/
BdkDragAction
bdk_drag_context_get_actions (BdkDragContext *context)
{
  g_return_val_if_fail (BDK_IS_DRAG_CONTEXT (context), BDK_ACTION_DEFAULT);

  return context->actions;
}

/**
 * bdk_drag_context_get_suggested_action:
 * @context: a #BdkDragContext
 *
 * Determines the suggested drag action of the context.
 *
 * Return value: a #BdkDragAction value
 *
 * Since: 2.22
 **/
BdkDragAction
bdk_drag_context_get_suggested_action (BdkDragContext *context)
{
  g_return_val_if_fail (BDK_IS_DRAG_CONTEXT (context), 0);

  return context->suggested_action;
}

/**
 * bdk_drag_context_get_selected_action:
 * @context: a #BdkDragContext
 *
 * Determines the action chosen by the drag destination.
 *
 * Return value: a #BdkDragAction value
 *
 * Since: 2.22
 **/
BdkDragAction
bdk_drag_context_get_selected_action (BdkDragContext *context)
{
  g_return_val_if_fail (BDK_IS_DRAG_CONTEXT (context), 0);

  return context->action;
}

/**
 * bdk_drag_context_get_source_window:
 * @context: a #BdkDragContext
 *
 * Returns the #BdkWindow where the DND operation started.
 *
 * Return value: (transfer none): a #BdkWindow
 *
 * Since: 2.22
 **/
BdkWindow *
bdk_drag_context_get_source_window (BdkDragContext *context)
{
  g_return_val_if_fail (BDK_IS_DRAG_CONTEXT (context), NULL);

  return context->source_window;
}

/**
 * bdk_drag_context_get_dest_window:
 * @context: a #BdkDragContext
 *
 * Returns the destination windw for the DND operation.
 *
 * Return value: (transfer none): a #BdkWindow
 *
 * Since: 2.24
 */
BdkWindow *
bdk_drag_context_get_dest_window (BdkDragContext *context)
{
  g_return_val_if_fail (BDK_IS_DRAG_CONTEXT (context), NULL);

  return context->dest_window;
}

/**
 * bdk_drag_context_get_protocol:
 * @context: a #BdkDragContext
 *
 * Returns the drag protocol thats used by this context.
 *
 * Returns: the drag protocol
 *
 * Since: 2.24
 */
BdkDragProtocol
bdk_drag_context_get_protocol (BdkDragContext *context)
{
  g_return_val_if_fail (BDK_IS_DRAG_CONTEXT (context), BDK_DRAG_PROTO_NONE);

  return context->protocol;
}

#define __BDK_DND_C__
#include "bdkaliasdef.c"
