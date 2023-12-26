/* BTK - The GIMP Toolkit
 * bdkasync.h: Utility functions using the Xlib asynchronous interfaces
 * Copyright (C) 2003, Red Hat, Inc.
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

#ifndef __BDK_ASYNC_H__
#define __BDK_ASYNC_H__

#include <X11/Xlib.h>
#include "bdk.h"

G_BEGIN_DECLS

typedef struct _BdkChildInfoX11 BdkChildInfoX11;

typedef void (*BdkSendXEventCallback) (Window   window,
				       gboolean success,
				       gpointer data);
typedef void (*BdkRoundTripCallback)  (BdkDisplay *display,
				       gpointer data,
				       gulong serial);

struct _BdkChildInfoX11
{
  Window window;
  gint x;
  gint y;
  gint width;
  gint height;
  guint is_mapped : 1;
  guint has_wm_state : 1;
  guint window_class : 2;
};

void _bdk_x11_send_client_message_async (BdkDisplay            *display,
					 Window                 window,
					 gboolean               propagate,
					 glong                  event_mask,
					 XClientMessageEvent   *event_send,
					 BdkSendXEventCallback  callback,
					 gpointer               data);
void _bdk_x11_set_input_focus_safe      (BdkDisplay            *display,
					 Window                 window,
					 int                    revert_to,
					 Time                   time);

gboolean _bdk_x11_get_window_child_info (BdkDisplay       *display,
					 Window            window,
					 gboolean          get_wm_state,
					 gboolean         *win_has_wm_state,
					 BdkChildInfoX11 **children,
					 guint            *nchildren);

void _bdk_x11_roundtrip_async           (BdkDisplay           *display, 
					 BdkRoundTripCallback callback,
					 gpointer              data);

G_END_DECLS

#endif /* __BDK_ASYNC_H__ */
