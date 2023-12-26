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

B_BEGIN_DECLS

typedef struct _BdkChildInfoX11 BdkChildInfoX11;

typedef void (*BdkSendXEventCallback) (Window   window,
				       bboolean success,
				       bpointer data);
typedef void (*BdkRoundTripCallback)  (BdkDisplay *display,
				       bpointer data,
				       bulong serial);

struct _BdkChildInfoX11
{
  Window window;
  bint x;
  bint y;
  bint width;
  bint height;
  buint is_mapped : 1;
  buint has_wm_state : 1;
  buint window_class : 2;
};

void _bdk_x11_send_client_message_async (BdkDisplay            *display,
					 Window                 window,
					 bboolean               propagate,
					 blong                  event_mask,
					 XClientMessageEvent   *event_send,
					 BdkSendXEventCallback  callback,
					 bpointer               data);
void _bdk_x11_set_input_focus_safe      (BdkDisplay            *display,
					 Window                 window,
					 int                    revert_to,
					 Time                   time);

bboolean _bdk_x11_get_window_child_info (BdkDisplay       *display,
					 Window            window,
					 bboolean          get_wm_state,
					 bboolean         *win_has_wm_state,
					 BdkChildInfoX11 **children,
					 buint            *nchildren);

void _bdk_x11_roundtrip_async           (BdkDisplay           *display, 
					 BdkRoundTripCallback callback,
					 bpointer              data);

B_END_DECLS

#endif /* __BDK_ASYNC_H__ */
