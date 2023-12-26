/* btktextbufferserialize.h
 *
 * Copyright (C) 2004 Nokia Corporation.
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

#ifndef __BTK_TEXT_BUFFER_SERIALIZE_H__
#define __BTK_TEXT_BUFFER_SERIALIZE_H__

#include <btk/btktextbuffer.h>

buint8 * _btk_text_buffer_serialize_rich_text   (BtkTextBuffer     *register_buffer,
                                                 BtkTextBuffer     *content_buffer,
                                                 const BtkTextIter *start,
                                                 const BtkTextIter *end,
                                                 bsize             *length,
                                                 bpointer           user_data);

bboolean _btk_text_buffer_deserialize_rich_text (BtkTextBuffer     *register_buffer,
                                                 BtkTextBuffer     *content_buffer,
                                                 BtkTextIter       *iter,
                                                 const buint8      *data,
                                                 bsize              length,
                                                 bboolean           create_tags,
                                                 bpointer           user_data,
                                                 GError           **error);


#endif /* __BTK_TEXT_BUFFER_SERIALIZE_H__ */
