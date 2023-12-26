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

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#ifndef __BTK_SOCKET_PRIVATE_H__
#define __BTK_SOCKET_PRIVATE_H__

typedef struct _BtkSocketPrivate BtkSocketPrivate;

struct _BtkSocketPrivate
{
  gint resize_count;
};

/* In btksocket.c: */
BtkSocketPrivate *_btk_socket_get_private (BtkSocket *socket);

void _btk_socket_add_grabbed_key  (BtkSocket        *socket,
				   guint             keyval,
				   BdkModifierType   modifiers);
void _btk_socket_remove_grabbed_key (BtkSocket      *socket,
				     guint           keyval,
				     BdkModifierType modifiers);
void _btk_socket_claim_focus 	  (BtkSocket        *socket,
			     	   gboolean          send_event);
void _btk_socket_add_window  	  (BtkSocket        *socket,
			     	   BdkNativeWindow   xid,
			     	   gboolean          need_reparent);
void _btk_socket_end_embedding    (BtkSocket        *socket);

void _btk_socket_handle_map_request     (BtkSocket        *socket);
void _btk_socket_unmap_notify           (BtkSocket        *socket);
void _btk_socket_advance_toplevel_focus (BtkSocket        *socket,
					 BtkDirectionType  direction);

/* In backend-specific file: */

/*
 * _btk_socket_windowing_get_id:
 *
 * @socket: a #BtkSocket
 *
 * Returns the native windowing system identifier for the plug's window.
 */
BdkNativeWindow _btk_socket_windowing_get_id (BtkSocket *socket);

/*
 * _btk_socket_windowing_realize_window:
 *
 */
void _btk_socket_windowing_realize_window (BtkSocket *socket);

/*
 * _btk_socket_windowing_end_embedding_toplevel:
 *
 */
void _btk_socket_windowing_end_embedding_toplevel (BtkSocket *socket);

/*
 * _btk_socket_windowing_size_request:
 *
 */
void _btk_socket_windowing_size_request (BtkSocket *socket);

/*
 * _btk_socket_windowing_send_key_event:
 *
 */
void _btk_socket_windowing_send_key_event (BtkSocket *socket,
					   BdkEvent  *bdk_event,
					   gboolean   mask_key_presses);

/*
 * _btk_socket_windowing_focus_change:
 *
 */
void _btk_socket_windowing_focus_change (BtkSocket *socket,
					 gboolean   focus_in);

/*
 * _btk_socket_windowing_update_active:
 *
 */
void _btk_socket_windowing_update_active (BtkSocket *socket,
					  gboolean   active);

/*
 * _btk_socket_windowing_update_modality:
 *
 */
void _btk_socket_windowing_update_modality (BtkSocket *socket,
					    gboolean   modality);

/*
 * _btk_socket_windowing_focus:
 *
 */
void _btk_socket_windowing_focus (BtkSocket *socket,
				  BtkDirectionType direction);

/*
 * _btk_socket_windowing_send_configure_event:
 *
 */
void _btk_socket_windowing_send_configure_event (BtkSocket *socket);

/*
 * _btk_socket_windowing_select_plug_window_input:
 *
 * Asks the windowing system to send necessary events related to the
 * plug window to the socket window. Called only for out-of-process
 * embedding.
 */
void _btk_socket_windowing_select_plug_window_input (BtkSocket *socket);

/*
 * _btk_socket_windowing_embed_get_info:
 *
 * Gets whatever information necessary about an out-of-process plug
 * window.
 */
void _btk_socket_windowing_embed_get_info (BtkSocket *socket);

/*
 * _btk_socket_windowing_embed_notify:
 *
 */
void _btk_socket_windowing_embed_notify (BtkSocket *socket);

/*
 * _btk_socket_windowing_embed_get_focus_wrapped:
 *
 */
gboolean _btk_socket_windowing_embed_get_focus_wrapped (void);

/*
 * _btk_socket_windowing_embed_set_focus_wrapped:
 *
 */
void _btk_socket_windowing_embed_set_focus_wrapped (void);

/*
 * _btk_socket_windowing_filter_func:
 *
 */
BdkFilterReturn _btk_socket_windowing_filter_func (BdkXEvent *bdk_xevent,
						   BdkEvent  *event,
						   gpointer   data);

#endif /* __BTK_SOCKET_PRIVATE_H__ */
