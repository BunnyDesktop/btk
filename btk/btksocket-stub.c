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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* Stub implementation of backend-specific BtkPlug functions. */

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "btksocket.h"
#include "btksocketprivate.h"
#include "btkalias.h"

BdkNativeWindow
_btk_socket_windowing_get_id (BtkSocket *socket)
{
  return 0;
}

void
_btk_socket_windowing_realize_window (BtkSocket *socket)
{
}

void
_btk_socket_windowing_end_embedding_toplevel (BtkSocket *socket)
{
}

void
_btk_socket_windowing_size_request (BtkSocket *socket)
{
}

void
_btk_socket_windowing_send_key_event (BtkSocket *socket,
				      BdkEvent  *bdk_event,
				      gboolean   mask_key_presses)
{
}

void
_btk_socket_windowing_focus_change (BtkSocket *socket,
				    gboolean   focus_in)
{
}

void
_btk_socket_windowing_update_active (BtkSocket *socket,
				     gboolean   active)
{
}

void
_btk_socket_windowing_update_modality (BtkSocket *socket,
				       gboolean   modality)
{
}

void
_btk_socket_windowing_focus (BtkSocket       *socket,
			     BtkDirectionType direction)
{
}

void
_btk_socket_windowing_send_configure_event (BtkSocket *socket)
{
}

void
_btk_socket_windowing_select_plug_window_input (BtkSocket *socket)
{
}

void
_btk_socket_windowing_embed_get_info (BtkSocket *socket)
{
}

void
_btk_socket_windowing_embed_notify (BtkSocket *socket)
{
}

gboolean
_btk_socket_windowing_embed_get_focus_wrapped (void)
{
  return FALSE;
}

void
_btk_socket_windowing_embed_set_focus_wrapped (void)
{
}

BdkFilterReturn
_btk_socket_windowing_filter_func (BdkXEvent *bdk_xevent,
				   BdkEvent  *event,
				   gpointer   data)
{
  return BDK_FILTER_CONTINUE;
}
