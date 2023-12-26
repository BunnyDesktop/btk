/* btkrichtext.h
 *
 * Copyright (C) 2006 Imendio AB
 * Contact: Michael Natterer <mitch@imendio.com>
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

#ifndef __BTK_TEXT_BUFFER_RICH_TEXT_H__
#define __BTK_TEXT_BUFFER_RICH_TEXT_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btktextbuffer.h>

B_BEGIN_DECLS

typedef buint8 * (* BtkTextBufferSerializeFunc)   (BtkTextBuffer     *register_buffer,
                                                   BtkTextBuffer     *content_buffer,
                                                   const BtkTextIter *start,
                                                   const BtkTextIter *end,
                                                   bsize             *length,
                                                   bpointer           user_data);
typedef bboolean (* BtkTextBufferDeserializeFunc) (BtkTextBuffer     *register_buffer,
                                                   BtkTextBuffer     *content_buffer,
                                                   BtkTextIter       *iter,
                                                   const buint8      *data,
                                                   bsize              length,
                                                   bboolean           create_tags,
                                                   bpointer           user_data,
                                                   GError           **error);

BdkAtom   btk_text_buffer_register_serialize_format   (BtkTextBuffer                *buffer,
                                                       const bchar                  *mime_type,
                                                       BtkTextBufferSerializeFunc    function,
                                                       bpointer                      user_data,
                                                       GDestroyNotify                user_data_destroy);
BdkAtom   btk_text_buffer_register_serialize_tagset   (BtkTextBuffer                *buffer,
                                                       const bchar                  *tagset_name);

BdkAtom   btk_text_buffer_register_deserialize_format (BtkTextBuffer                *buffer,
                                                       const bchar                  *mime_type,
                                                       BtkTextBufferDeserializeFunc  function,
                                                       bpointer                      user_data,
                                                       GDestroyNotify                user_data_destroy);
BdkAtom   btk_text_buffer_register_deserialize_tagset (BtkTextBuffer                *buffer,
                                                       const bchar                  *tagset_name);

void    btk_text_buffer_unregister_serialize_format   (BtkTextBuffer                *buffer,
                                                       BdkAtom                       format);
void    btk_text_buffer_unregister_deserialize_format (BtkTextBuffer                *buffer,
                                                       BdkAtom                       format);

void     btk_text_buffer_deserialize_set_can_create_tags (BtkTextBuffer             *buffer,
                                                          BdkAtom                    format,
                                                          bboolean                   can_create_tags);
bboolean btk_text_buffer_deserialize_get_can_create_tags (BtkTextBuffer             *buffer,
                                                          BdkAtom                    format);

BdkAtom * btk_text_buffer_get_serialize_formats       (BtkTextBuffer                *buffer,
                                                       bint                         *n_formats);
BdkAtom * btk_text_buffer_get_deserialize_formats     (BtkTextBuffer                *buffer,
                                                       bint                         *n_formats);

buint8  * btk_text_buffer_serialize                   (BtkTextBuffer                *register_buffer,
                                                       BtkTextBuffer                *content_buffer,
                                                       BdkAtom                       format,
                                                       const BtkTextIter            *start,
                                                       const BtkTextIter            *end,
                                                       bsize                        *length);
bboolean  btk_text_buffer_deserialize                 (BtkTextBuffer                *register_buffer,
                                                       BtkTextBuffer                *content_buffer,
                                                       BdkAtom                       format,
                                                       BtkTextIter                  *iter,
                                                       const buint8                 *data,
                                                       bsize                         length,
                                                       GError                      **error);

B_END_DECLS

#endif /* __BTK_TEXT_BUFFER_RICH_TEXT_H__ */
