/* BTK - The GIMP Toolkit
 * btkxembed.c: Utilities for the XEMBED protocol
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

#ifndef __BTK_XEMBED_H__
#define __BTK_XEMBED_H__

#include "xembed.h"
#include "x11/bdkx.h"

B_BEGIN_DECLS

/* Latest version we implement */
#define BTK_XEMBED_PROTOCOL_VERSION 1

void _btk_xembed_send_message       (BdkWindow         *recipient,
				     XEmbedMessageType  message,
				     blong              detail,
				     blong              data1,
				     blong              data2);
void _btk_xembed_send_focus_message (BdkWindow         *recipient,
				     XEmbedMessageType  message,
				     blong              detail);

void        _btk_xembed_push_message       (XEvent    *xevent);
void        _btk_xembed_pop_message        (void);
void        _btk_xembed_set_focus_wrapped  (void);
bboolean    _btk_xembed_get_focus_wrapped  (void);
const char *_btk_xembed_message_name       (XEmbedMessageType message);

B_END_DECLS

#endif /*  __BTK_XEMBED_H__ */
