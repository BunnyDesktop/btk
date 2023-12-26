/* btkentrybuffer.h
 * Copyright (C) 2009  Stefan Walter <stef@memberwebs.com>
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

#ifndef __BTK_ENTRY_BUFFER_H__
#define __BTK_ENTRY_BUFFER_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bunnylib-object.h>

B_BEGIN_DECLS

/* Maximum size of text buffer, in bytes */
#define BTK_ENTRY_BUFFER_MAX_SIZE        G_MAXUSHORT

#define BTK_TYPE_ENTRY_BUFFER            (btk_entry_buffer_get_type ())
#define BTK_ENTRY_BUFFER(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_ENTRY_BUFFER, BtkEntryBuffer))
#define BTK_ENTRY_BUFFER_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_ENTRY_BUFFER, BtkEntryBufferClass))
#define BTK_IS_ENTRY_BUFFER(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_ENTRY_BUFFER))
#define BTK_IS_ENTRY_BUFFER_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_ENTRY_BUFFER))
#define BTK_ENTRY_BUFFER_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_ENTRY_BUFFER, BtkEntryBufferClass))

typedef struct _BtkEntryBuffer            BtkEntryBuffer;
typedef struct _BtkEntryBufferClass       BtkEntryBufferClass;
typedef struct _BtkEntryBufferPrivate     BtkEntryBufferPrivate;

struct _BtkEntryBuffer
{
  BObject parent_instance;

  /*< private >*/
  BtkEntryBufferPrivate *priv;
};

struct _BtkEntryBufferClass
{
  BObjectClass parent_class;

  /* Signals */

  void         (*inserted_text)          (BtkEntryBuffer *buffer,
                                          guint           position,
                                          const gchar    *chars,
                                          guint           n_chars);

  void         (*deleted_text)           (BtkEntryBuffer *buffer,
                                          guint           position,
                                          guint           n_chars);

  /* Virtual Methods */

  const gchar* (*get_text)               (BtkEntryBuffer *buffer,
                                          gsize          *n_bytes);

  guint        (*get_length)             (BtkEntryBuffer *buffer);

  guint        (*insert_text)            (BtkEntryBuffer *buffer,
                                          guint           position,
                                          const gchar    *chars,
                                          guint           n_chars);

  guint        (*delete_text)            (BtkEntryBuffer *buffer,
                                          guint           position,
                                          guint           n_chars);

  /* Padding for future expansion */
  void (*_btk_reserved0) (void);
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
  void (*_btk_reserved5) (void);
};

GType                     btk_entry_buffer_get_type               (void) B_GNUC_CONST;

BtkEntryBuffer*           btk_entry_buffer_new                    (const gchar     *initial_chars,
                                                                   gint             n_initial_chars);

gsize                     btk_entry_buffer_get_bytes              (BtkEntryBuffer  *buffer);

guint                     btk_entry_buffer_get_length             (BtkEntryBuffer  *buffer);

const gchar*              btk_entry_buffer_get_text               (BtkEntryBuffer  *buffer);

void                      btk_entry_buffer_set_text               (BtkEntryBuffer  *buffer,
                                                                   const gchar     *chars,
                                                                   gint             n_chars);

void                      btk_entry_buffer_set_max_length         (BtkEntryBuffer  *buffer,
                                                                   gint             max_length);

gint                      btk_entry_buffer_get_max_length         (BtkEntryBuffer  *buffer);

guint                     btk_entry_buffer_insert_text            (BtkEntryBuffer  *buffer,
                                                                   guint            position,
                                                                   const gchar     *chars,
                                                                   gint             n_chars);

guint                     btk_entry_buffer_delete_text            (BtkEntryBuffer  *buffer,
                                                                   guint            position,
                                                                   gint             n_chars);

void                      btk_entry_buffer_emit_inserted_text     (BtkEntryBuffer  *buffer,
                                                                   guint            position,
                                                                   const gchar     *chars,
                                                                   guint            n_chars);

void                      btk_entry_buffer_emit_deleted_text      (BtkEntryBuffer  *buffer,
                                                                   guint            position,
                                                                   guint            n_chars);

B_END_DECLS

#endif /* __BTK_ENTRY_BUFFER_H__ */
