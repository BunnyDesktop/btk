/* btkentrybuffer.c
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

#include "config.h"

#include "btkentrybuffer.h"
#include "btkintl.h"
#include "btkmarshalers.h"
#include "btkprivate.h"
#include "btkwidget.h"
#include "btkalias.h"

#include <bdk/bdk.h>

#include <string.h>

/**
 * SECTION:btkentrybuffer
 * @title: BtkEntryBuffer
 * @short_description: Text buffer for BtkEntry
 *
 * The #BtkEntryBuffer class contains the actual text displayed in a
 * #BtkEntry widget.
 *
 * A single #BtkEntryBuffer object can be shared by multiple #BtkEntry
 * widgets which will then share the same text content, but not the cursor
 * position, visibility attributes, icon etc.
 *
 * #BtkEntryBuffer may be derived from. Such a derived class might allow
 * text to be stored in an alternate location, such as non-pageable memory,
 * useful in the case of important passwords. Or a derived class could 
 * integrate with an application's concept of undo/redo.
 *
 * Since: 2.18
 */

/* Initial size of buffer, in bytes */
#define MIN_SIZE 16

enum {
  PROP_0,
  PROP_TEXT,
  PROP_LENGTH,
  PROP_MAX_LENGTH,
};

enum {
  INSERTED_TEXT,
  DELETED_TEXT,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

struct _BtkEntryBufferPrivate
{
  gint  max_length;

  /* Only valid if this class is not derived */
  gchar *normal_text;
  gsize  normal_text_size;
  gsize  normal_text_bytes;
  guint  normal_text_chars;
};

G_DEFINE_TYPE (BtkEntryBuffer, btk_entry_buffer, G_TYPE_OBJECT);

/* --------------------------------------------------------------------------------
 * DEFAULT IMPLEMENTATIONS OF TEXT BUFFER
 *
 * These may be overridden by a derived class, behavior may be changed etc...
 * The normal_text and normal_text_xxxx fields may not be valid when
 * this class is derived from.
 */

/* Overwrite a memory that might contain sensitive information. */
static void
trash_area (gchar *area,
            gsize  len)
{
  volatile gchar *varea = (volatile gchar *)area;
  while (len-- > 0)
    *varea++ = 0;
}

static const gchar*
btk_entry_buffer_normal_get_text (BtkEntryBuffer *buffer,
                                  gsize          *n_bytes)
{
  if (n_bytes)
    *n_bytes = buffer->priv->normal_text_bytes;
  if (!buffer->priv->normal_text)
      return "";
  return buffer->priv->normal_text;
}

static guint
btk_entry_buffer_normal_get_length (BtkEntryBuffer *buffer)
{
  return buffer->priv->normal_text_chars;
}

static guint
btk_entry_buffer_normal_insert_text (BtkEntryBuffer *buffer,
                                     guint           position,
                                     const gchar    *chars,
                                     guint           n_chars)
{
  BtkEntryBufferPrivate *pv = buffer->priv;
  gsize prev_size;
  gsize n_bytes;
  gsize at;

  n_bytes = g_utf8_offset_to_pointer (chars, n_chars) - chars;

  /* Need more memory */
  if (n_bytes + pv->normal_text_bytes + 1 > pv->normal_text_size)
    {
      gchar *et_new;

      prev_size = pv->normal_text_size;

      /* Calculate our new buffer size */
      while (n_bytes + pv->normal_text_bytes + 1 > pv->normal_text_size)
        {
          if (pv->normal_text_size == 0)
            pv->normal_text_size = MIN_SIZE;
          else
            {
              if (2 * pv->normal_text_size < BTK_ENTRY_BUFFER_MAX_SIZE)
                pv->normal_text_size *= 2;
              else
                {
                  pv->normal_text_size = BTK_ENTRY_BUFFER_MAX_SIZE;
                  if (n_bytes > pv->normal_text_size - pv->normal_text_bytes - 1)
                    {
                      n_bytes = pv->normal_text_size - pv->normal_text_bytes - 1;
                      n_bytes = g_utf8_find_prev_char (chars, chars + n_bytes + 1) - chars;
                      n_chars = g_utf8_strlen (chars, n_bytes);
                    }
                  break;
                }
            }
        }

      /* Could be a password, so can't leave stuff in memory. */
      et_new = g_malloc (pv->normal_text_size);
      memcpy (et_new, pv->normal_text, MIN (prev_size, pv->normal_text_size));
      trash_area (pv->normal_text, prev_size);
      g_free (pv->normal_text);
      pv->normal_text = et_new;
    }

  /* Actual text insertion */
  at = g_utf8_offset_to_pointer (pv->normal_text, position) - pv->normal_text;
  g_memmove (pv->normal_text + at + n_bytes, pv->normal_text + at, pv->normal_text_bytes - at);
  memcpy (pv->normal_text + at, chars, n_bytes);

  /* Book keeping */
  pv->normal_text_bytes += n_bytes;
  pv->normal_text_chars += n_chars;
  pv->normal_text[pv->normal_text_bytes] = '\0';

  btk_entry_buffer_emit_inserted_text (buffer, position, chars, n_chars);
  return n_chars;
}

static guint
btk_entry_buffer_normal_delete_text (BtkEntryBuffer *buffer,
                                     guint           position,
                                     guint           n_chars)
{
  BtkEntryBufferPrivate *pv = buffer->priv;
  gsize start, end;

  if (position > pv->normal_text_chars)
    position = pv->normal_text_chars;
  if (position + n_chars > pv->normal_text_chars)
    n_chars = pv->normal_text_chars - position;

  if (n_chars > 0)
    {
      start = g_utf8_offset_to_pointer (pv->normal_text, position) - pv->normal_text;
      end = g_utf8_offset_to_pointer (pv->normal_text, position + n_chars) - pv->normal_text;

      g_memmove (pv->normal_text + start, pv->normal_text + end, pv->normal_text_bytes + 1 - end);
      pv->normal_text_chars -= n_chars;
      pv->normal_text_bytes -= (end - start);

      /*
       * Could be a password, make sure we don't leave anything sensitive after
       * the terminating zero.  Note, that the terminating zero already trashed
       * one byte.
       */
      trash_area (pv->normal_text + pv->normal_text_bytes + 1, end - start - 1);

      btk_entry_buffer_emit_deleted_text (buffer, position, n_chars);
    }

  return n_chars;
}

/* --------------------------------------------------------------------------------
 *
 */

static void
btk_entry_buffer_real_inserted_text (BtkEntryBuffer *buffer,
                                     guint           position,
                                     const gchar    *chars,
                                     guint           n_chars)
{
  g_object_notify (G_OBJECT (buffer), "text");
  g_object_notify (G_OBJECT (buffer), "length");
}

static void
btk_entry_buffer_real_deleted_text (BtkEntryBuffer *buffer,
                                    guint           position,
                                    guint           n_chars)
{
  g_object_notify (G_OBJECT (buffer), "text");
  g_object_notify (G_OBJECT (buffer), "length");
}

/* --------------------------------------------------------------------------------
 *
 */

static void
btk_entry_buffer_init (BtkEntryBuffer *buffer)
{
  BtkEntryBufferPrivate *pv;

  pv = buffer->priv = G_TYPE_INSTANCE_GET_PRIVATE (buffer, BTK_TYPE_ENTRY_BUFFER, BtkEntryBufferPrivate);

  pv->normal_text = NULL;
  pv->normal_text_chars = 0;
  pv->normal_text_bytes = 0;
  pv->normal_text_size = 0;
}

static void
btk_entry_buffer_finalize (GObject *obj)
{
  BtkEntryBuffer *buffer = BTK_ENTRY_BUFFER (obj);
  BtkEntryBufferPrivate *pv = buffer->priv;

  if (pv->normal_text)
    {
      trash_area (pv->normal_text, pv->normal_text_size);
      g_free (pv->normal_text);
      pv->normal_text = NULL;
      pv->normal_text_bytes = pv->normal_text_size = 0;
      pv->normal_text_chars = 0;
    }

  G_OBJECT_CLASS (btk_entry_buffer_parent_class)->finalize (obj);
}

static void
btk_entry_buffer_set_property (GObject      *obj,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  BtkEntryBuffer *buffer = BTK_ENTRY_BUFFER (obj);

  switch (prop_id)
    {
    case PROP_TEXT:
      btk_entry_buffer_set_text (buffer, g_value_get_string (value), -1);
      break;
    case PROP_MAX_LENGTH:
      btk_entry_buffer_set_max_length (buffer, g_value_get_int (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
      break;
    }
}

static void
btk_entry_buffer_get_property (GObject    *obj,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  BtkEntryBuffer *buffer = BTK_ENTRY_BUFFER (obj);

  switch (prop_id)
    {
    case PROP_TEXT:
      g_value_set_string (value, btk_entry_buffer_get_text (buffer));
      break;
    case PROP_LENGTH:
      g_value_set_uint (value, btk_entry_buffer_get_length (buffer));
      break;
    case PROP_MAX_LENGTH:
      g_value_set_int (value, btk_entry_buffer_get_max_length (buffer));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
      break;
    }
}

static void
btk_entry_buffer_class_init (BtkEntryBufferClass *klass)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (klass);

  bobject_class->finalize = btk_entry_buffer_finalize;
  bobject_class->set_property = btk_entry_buffer_set_property;
  bobject_class->get_property = btk_entry_buffer_get_property;

  klass->get_text = btk_entry_buffer_normal_get_text;
  klass->get_length = btk_entry_buffer_normal_get_length;
  klass->insert_text = btk_entry_buffer_normal_insert_text;
  klass->delete_text = btk_entry_buffer_normal_delete_text;

  klass->inserted_text = btk_entry_buffer_real_inserted_text;
  klass->deleted_text = btk_entry_buffer_real_deleted_text;

  g_type_class_add_private (bobject_class, sizeof (BtkEntryBufferPrivate));

  /**
   * BtkEntryBuffer:text:
   *
   * The contents of the buffer.
   *
   * Since: 2.18
   */
  g_object_class_install_property (bobject_class,
                                   PROP_TEXT,
                                   g_param_spec_string ("text",
                                                        P_("Text"),
                                                        P_("The contents of the buffer"),
                                                        "",
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkEntryBuffer:length:
   *
   * The length (in characters) of the text in buffer.
   *
   * Since: 2.18
   */
  g_object_class_install_property (bobject_class,
                                   PROP_LENGTH,
                                   g_param_spec_uint ("length",
                                                      P_("Text length"),
                                                      P_("Length of the text currently in the buffer"),
                                                      0, BTK_ENTRY_BUFFER_MAX_SIZE, 0,
                                                      BTK_PARAM_READABLE));

  /**
   * BtkEntryBuffer:max-length:
   *
   * The maximum length (in characters) of the text in the buffer.
   *
   * Since: 2.18
   */
  g_object_class_install_property (bobject_class,
                                   PROP_MAX_LENGTH,
                                   g_param_spec_int ("max-length",
                                                     P_("Maximum length"),
                                                     P_("Maximum number of characters for this entry. Zero if no maximum"),
                                   0, BTK_ENTRY_BUFFER_MAX_SIZE, 0,
                                   BTK_PARAM_READWRITE));

  /**
   * BtkEntryBuffer::inserted-text:
   * @buffer: a #BtkEntryBuffer
   * @position: the position the text was inserted at.
   * @chars: The text that was inserted.
   * @n_chars: The number of characters that were inserted.
   *
   * This signal is emitted after text is inserted into the buffer.
   *
   * Since: 2.18
   */
  signals[INSERTED_TEXT] = g_signal_new (I_("inserted-text"),
                                         BTK_TYPE_ENTRY_BUFFER,
                                         G_SIGNAL_RUN_FIRST,
                                         G_STRUCT_OFFSET (BtkEntryBufferClass, inserted_text),
                                         NULL, NULL,
                                         _btk_marshal_VOID__UINT_STRING_UINT,
                                         G_TYPE_NONE, 3,
                                         G_TYPE_UINT,
                                         G_TYPE_STRING,
                                         G_TYPE_UINT);

  /**
   * BtkEntryBuffer::deleted-text:
   * @buffer: a #BtkEntryBuffer
   * @position: the position the text was deleted at.
   * @n_chars: The number of characters that were deleted.
   *
   * This signal is emitted after text is deleted from the buffer.
   *
   * Since: 2.18
   */
  signals[DELETED_TEXT] =  g_signal_new (I_("deleted-text"),
                                         BTK_TYPE_ENTRY_BUFFER,
                                         G_SIGNAL_RUN_FIRST,
                                         G_STRUCT_OFFSET (BtkEntryBufferClass, deleted_text),
                                         NULL, NULL,
                                         _btk_marshal_VOID__UINT_UINT,
                                         G_TYPE_NONE, 2,
                                         G_TYPE_UINT,
                                         G_TYPE_UINT);
}

/* --------------------------------------------------------------------------------
 *
 */

/**
 * btk_entry_buffer_new:
 * @initial_chars: (allow-none): initial buffer text, or %NULL
 * @n_initial_chars: number of characters in @initial_chars, or -1
 *
 * Create a new BtkEntryBuffer object.
 *
 * Optionally, specify initial text to set in the buffer.
 *
 * Return value: A new BtkEntryBuffer object.
 *
 * Since: 2.18
 **/
BtkEntryBuffer*
btk_entry_buffer_new (const gchar *initial_chars,
                      gint         n_initial_chars)
{
  BtkEntryBuffer *buffer = g_object_new (BTK_TYPE_ENTRY_BUFFER, NULL);
  if (initial_chars)
    btk_entry_buffer_set_text (buffer, initial_chars, n_initial_chars);
  return buffer;
}

/**
 * btk_entry_buffer_get_length:
 * @buffer: a #BtkEntryBuffer
 *
 * Retrieves the length in characters of the buffer.
 *
 * Return value: The number of characters in the buffer.
 *
 * Since: 2.18
 **/
guint
btk_entry_buffer_get_length (BtkEntryBuffer *buffer)
{
  BtkEntryBufferClass *klass;

  g_return_val_if_fail (BTK_IS_ENTRY_BUFFER (buffer), 0);

  klass = BTK_ENTRY_BUFFER_GET_CLASS (buffer);
  g_return_val_if_fail (klass->get_length != NULL, 0);

  return (*klass->get_length) (buffer);
}

/**
 * btk_entry_buffer_get_bytes:
 * @buffer: a #BtkEntryBuffer
 *
 * Retrieves the length in bytes of the buffer.
 * See btk_entry_buffer_get_length().
 *
 * Return value: The byte length of the buffer.
 *
 * Since: 2.18
 **/
gsize
btk_entry_buffer_get_bytes (BtkEntryBuffer *buffer)
{
  BtkEntryBufferClass *klass;
  gsize bytes = 0;

  g_return_val_if_fail (BTK_IS_ENTRY_BUFFER (buffer), 0);

  klass = BTK_ENTRY_BUFFER_GET_CLASS (buffer);
  g_return_val_if_fail (klass->get_text != NULL, 0);

  (*klass->get_text) (buffer, &bytes);
  return bytes;
}

/**
 * btk_entry_buffer_get_text:
 * @buffer: a #BtkEntryBuffer
 *
 * Retrieves the contents of the buffer.
 *
 * The memory pointer returned by this call will not change
 * unless this object emits a signal, or is finalized.
 *
 * Return value: a pointer to the contents of the widget as a
 *      string. This string points to internally allocated
 *      storage in the buffer and must not be freed, modified or
 *      stored.
 *
 * Since: 2.18
 **/
const gchar*
btk_entry_buffer_get_text (BtkEntryBuffer *buffer)
{
  BtkEntryBufferClass *klass;

  g_return_val_if_fail (BTK_IS_ENTRY_BUFFER (buffer), NULL);

  klass = BTK_ENTRY_BUFFER_GET_CLASS (buffer);
  g_return_val_if_fail (klass->get_text != NULL, NULL);

  return (*klass->get_text) (buffer, NULL);
}

/**
 * btk_entry_buffer_set_text:
 * @buffer: a #BtkEntryBuffer
 * @chars: the new text
 * @n_chars: the number of characters in @text, or -1
 *
 * Sets the text in the buffer.
 *
 * This is roughly equivalent to calling btk_entry_buffer_delete_text()
 * and btk_entry_buffer_insert_text().
 *
 * Note that @n_chars is in characters, not in bytes.
 *
 * Since: 2.18
 **/
void
btk_entry_buffer_set_text (BtkEntryBuffer *buffer,
                           const gchar    *chars,
                           gint            n_chars)
{
  g_return_if_fail (BTK_IS_ENTRY_BUFFER (buffer));
  g_return_if_fail (chars != NULL);

  g_object_freeze_notify (G_OBJECT (buffer));
  btk_entry_buffer_delete_text (buffer, 0, -1);
  btk_entry_buffer_insert_text (buffer, 0, chars, n_chars);
  g_object_thaw_notify (G_OBJECT (buffer));
}

/**
 * btk_entry_buffer_set_max_length:
 * @buffer: a #BtkEntryBuffer
 * @max_length: the maximum length of the entry buffer, or 0 for no maximum.
 *   (other than the maximum length of entries.) The value passed in will
 *   be clamped to the range 0-65536.
 *
 * Sets the maximum allowed length of the contents of the buffer. If
 * the current contents are longer than the given length, then they
 * will be truncated to fit.
 *
 * Since: 2.18
 **/
void
btk_entry_buffer_set_max_length (BtkEntryBuffer *buffer,
                                 gint            max_length)
{
  g_return_if_fail (BTK_IS_ENTRY_BUFFER (buffer));

  max_length = CLAMP (max_length, 0, BTK_ENTRY_BUFFER_MAX_SIZE);

  if (max_length > 0 && btk_entry_buffer_get_length (buffer) > max_length)
    btk_entry_buffer_delete_text (buffer, max_length, -1);

  buffer->priv->max_length = max_length;
  g_object_notify (G_OBJECT (buffer), "max-length");
}

/**
 * btk_entry_buffer_get_max_length:
 * @buffer: a #BtkEntryBuffer
 *
 * Retrieves the maximum allowed length of the text in
 * @buffer. See btk_entry_buffer_set_max_length().
 *
 * Return value: the maximum allowed number of characters
 *               in #BtkEntryBuffer, or 0 if there is no maximum.
 *
 * Since: 2.18
 */
gint
btk_entry_buffer_get_max_length (BtkEntryBuffer *buffer)
{
  g_return_val_if_fail (BTK_IS_ENTRY_BUFFER (buffer), 0);
  return buffer->priv->max_length;
}

/**
 * btk_entry_buffer_insert_text:
 * @buffer: a #BtkEntryBuffer
 * @position: the position at which to insert text.
 * @chars: the text to insert into the buffer.
 * @n_chars: the length of the text in characters, or -1
 *
 * Inserts @n_chars characters of @chars into the contents of the
 * buffer, at position @position.
 *
 * If @n_chars is negative, then characters from chars will be inserted
 * until a null-terminator is found. If @position or @n_chars are out of
 * bounds, or the maximum buffer text length is exceeded, then they are
 * coerced to sane values.
 *
 * Note that the position and length are in characters, not in bytes.
 *
 * Returns: The number of characters actually inserted.
 *
 * Since: 2.18
 */
guint
btk_entry_buffer_insert_text (BtkEntryBuffer *buffer,
                              guint           position,
                              const gchar    *chars,
                              gint            n_chars)
{
  BtkEntryBufferClass *klass;
  BtkEntryBufferPrivate *pv;
  guint length;

  g_return_val_if_fail (BTK_IS_ENTRY_BUFFER (buffer), 0);

  length = btk_entry_buffer_get_length (buffer);
  pv = buffer->priv;

  if (n_chars < 0)
    n_chars = g_utf8_strlen (chars, -1);

  /* Bring position into bounds */
  if (position > length)
    position = length;

  /* Make sure not entering too much data */
  if (pv->max_length > 0)
    {
      if (length >= pv->max_length)
        n_chars = 0;
      else if (length + n_chars > pv->max_length)
        n_chars -= (length + n_chars) - pv->max_length;
    }

  klass = BTK_ENTRY_BUFFER_GET_CLASS (buffer);
  g_return_val_if_fail (klass->insert_text != NULL, 0);

  return (*klass->insert_text) (buffer, position, chars, n_chars);
}

/**
 * btk_entry_buffer_delete_text:
 * @buffer: a #BtkEntryBuffer
 * @position: position at which to delete text
 * @n_chars: number of characters to delete
 *
 * Deletes a sequence of characters from the buffer. @n_chars characters are
 * deleted starting at @position. If @n_chars is negative, then all characters
 * until the end of the text are deleted.
 *
 * If @position or @n_chars are out of bounds, then they are coerced to sane
 * values.
 *
 * Note that the positions are specified in characters, not bytes.
 *
 * Returns: The number of characters deleted.
 *
 * Since: 2.18
 */
guint
btk_entry_buffer_delete_text (BtkEntryBuffer *buffer,
                              guint           position,
                              gint            n_chars)
{
  BtkEntryBufferClass *klass;
  guint length;

  g_return_val_if_fail (BTK_IS_ENTRY_BUFFER (buffer), 0);

  length = btk_entry_buffer_get_length (buffer);
  if (n_chars < 0)
    n_chars = length;
  if (position > length)
    position = length;
  if (position + n_chars > length)
    n_chars = length - position;

  klass = BTK_ENTRY_BUFFER_GET_CLASS (buffer);
  g_return_val_if_fail (klass->delete_text != NULL, 0);

  return (*klass->delete_text) (buffer, position, n_chars);
}

/**
 * btk_entry_buffer_emit_inserted_text:
 * @buffer: a #BtkEntryBuffer
 * @position: position at which text was inserted
 * @chars: text that was inserted
 * @n_chars: number of characters inserted
 *
 * Used when subclassing #BtkEntryBuffer
 *
 * Since: 2.18
 */
void
btk_entry_buffer_emit_inserted_text (BtkEntryBuffer *buffer,
                                     guint           position,
                                     const gchar    *chars,
                                     guint           n_chars)
{
  g_return_if_fail (BTK_IS_ENTRY_BUFFER (buffer));
  g_signal_emit (buffer, signals[INSERTED_TEXT], 0, position, chars, n_chars);
}

/**
 * btk_entry_buffer_emit_deleted_text:
 * @buffer: a #BtkEntryBuffer
 * @position: position at which text was deleted
 * @n_chars: number of characters deleted
 *
 * Used when subclassing #BtkEntryBuffer
 *
 * Since: 2.18
 */
void
btk_entry_buffer_emit_deleted_text (BtkEntryBuffer *buffer,
                                    guint           position,
                                    guint           n_chars)
{
  g_return_if_fail (BTK_IS_ENTRY_BUFFER (buffer));
  g_signal_emit (buffer, signals[DELETED_TEXT], 0, position, n_chars);
}

#define __BTK_ENTRY_BUFFER_C__
#include "btkaliasdef.c"
