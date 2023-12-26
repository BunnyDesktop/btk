/* BTK - The GIMP Toolkit
 * btktextbuffer.c Copyright (C) 2000 Red Hat, Inc.
 *                 Copyright (C) 2004 Nokia Corporation
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
#include <string.h>
#include <stdarg.h>

#define BTK_TEXT_USE_INTERNAL_UNSUPPORTED_API
#include "btkclipboard.h"
#include "btkdnd.h"
#include "btkinvisible.h"
#include "btkmarshalers.h"
#include "btktextbuffer.h"
#include "btktextbufferrichtext.h"
#include "btktextbtree.h"
#include "btktextiterprivate.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"


#define BTK_TEXT_BUFFER_GET_PRIVATE(obj) (B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_TEXT_BUFFER, BtkTextBufferPrivate))

typedef struct _BtkTextBufferPrivate BtkTextBufferPrivate;

struct _BtkTextBufferPrivate
{
  BtkTargetList  *copy_target_list;
  BtkTargetEntry *copy_target_entries;
  bint            n_copy_target_entries;

  BtkTargetList  *paste_target_list;
  BtkTargetEntry *paste_target_entries;
  bint            n_paste_target_entries;
};


typedef struct _ClipboardRequest ClipboardRequest;

struct _ClipboardRequest
{
  BtkTextBuffer *buffer;
  bboolean interactive;
  bboolean default_editable;
  bboolean is_clipboard;
  bboolean replace_selection;
};

enum {
  INSERT_TEXT,
  INSERT_PIXBUF,
  INSERT_CHILD_ANCHOR,
  DELETE_RANGE,
  CHANGED,
  MODIFIED_CHANGED,
  MARK_SET,
  MARK_DELETED,
  APPLY_TAG,
  REMOVE_TAG,
  BEGIN_USER_ACTION,
  END_USER_ACTION,
  PASTE_DONE,
  LAST_SIGNAL
};

enum {
  PROP_0,

  /* Construct */
  PROP_TAG_TABLE,

  /* Normal */
  PROP_TEXT,
  PROP_HAS_SELECTION,
  PROP_CURSOR_POSITION,
  PROP_COPY_TARGET_LIST,
  PROP_PASTE_TARGET_LIST
};

static void btk_text_buffer_finalize   (BObject            *object);

static void btk_text_buffer_real_insert_text           (BtkTextBuffer     *buffer,
                                                        BtkTextIter       *iter,
                                                        const bchar       *text,
                                                        bint               len);
static void btk_text_buffer_real_insert_pixbuf         (BtkTextBuffer     *buffer,
                                                        BtkTextIter       *iter,
                                                        BdkPixbuf         *pixbuf);
static void btk_text_buffer_real_insert_anchor         (BtkTextBuffer     *buffer,
                                                        BtkTextIter       *iter,
                                                        BtkTextChildAnchor *anchor);
static void btk_text_buffer_real_delete_range          (BtkTextBuffer     *buffer,
                                                        BtkTextIter       *start,
                                                        BtkTextIter       *end);
static void btk_text_buffer_real_apply_tag             (BtkTextBuffer     *buffer,
                                                        BtkTextTag        *tag,
                                                        const BtkTextIter *start_char,
                                                        const BtkTextIter *end_char);
static void btk_text_buffer_real_remove_tag            (BtkTextBuffer     *buffer,
                                                        BtkTextTag        *tag,
                                                        const BtkTextIter *start_char,
                                                        const BtkTextIter *end_char);
static void btk_text_buffer_real_changed               (BtkTextBuffer     *buffer);
static void btk_text_buffer_real_mark_set              (BtkTextBuffer     *buffer,
                                                        const BtkTextIter *iter,
                                                        BtkTextMark       *mark);

static BtkTextBTree* get_btree (BtkTextBuffer *buffer);
static void          free_log_attr_cache (BtkTextLogAttrCache *cache);

static void remove_all_selection_clipboards       (BtkTextBuffer *buffer);
static void update_selection_clipboards           (BtkTextBuffer *buffer);

static BtkTextBuffer *create_clipboard_contents_buffer (BtkTextBuffer *buffer);

static void btk_text_buffer_free_target_lists     (BtkTextBuffer *buffer);

static buint signals[LAST_SIGNAL] = { 0 };

static void btk_text_buffer_set_property (BObject         *object,
				          buint            prop_id,
				          const BValue    *value,
				          BParamSpec      *pspec);
static void btk_text_buffer_get_property (BObject         *object,
				          buint            prop_id,
				          BValue          *value,
				          BParamSpec      *pspec);
static void btk_text_buffer_notify       (BObject         *object,
                                          BParamSpec      *pspec);

G_DEFINE_TYPE (BtkTextBuffer, btk_text_buffer, B_TYPE_OBJECT)

static void
btk_text_buffer_class_init (BtkTextBufferClass *klass)
{
  BObjectClass *object_class = B_OBJECT_CLASS (klass);

  object_class->finalize = btk_text_buffer_finalize;
  object_class->set_property = btk_text_buffer_set_property;
  object_class->get_property = btk_text_buffer_get_property;
  object_class->notify       = btk_text_buffer_notify;
 
  klass->insert_text = btk_text_buffer_real_insert_text;
  klass->insert_pixbuf = btk_text_buffer_real_insert_pixbuf;
  klass->insert_child_anchor = btk_text_buffer_real_insert_anchor;
  klass->delete_range = btk_text_buffer_real_delete_range;
  klass->apply_tag = btk_text_buffer_real_apply_tag;
  klass->remove_tag = btk_text_buffer_real_remove_tag;
  klass->changed = btk_text_buffer_real_changed;
  klass->mark_set = btk_text_buffer_real_mark_set;

  /* Construct */
  g_object_class_install_property (object_class,
                                   PROP_TAG_TABLE,
                                   g_param_spec_object ("tag-table",
                                                        P_("Tag Table"),
                                                        P_("Text Tag Table"),
                                                        BTK_TYPE_TEXT_TAG_TABLE,
                                                        BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  /* Normal properties*/
  
  /**
   * BtkTextBuffer:text:
   *
   * The text content of the buffer. Without child widgets and images,
   * see btk_text_buffer_get_text() for more information.
   *
   * Since: 2.8
   */
  g_object_class_install_property (object_class,
                                   PROP_TEXT,
                                   g_param_spec_string ("text",
                                                        P_("Text"),
                                                        P_("Current text of the buffer"),
							"",
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkTextBuffer:has-selection:
   *
   * Whether the buffer has some text currently selected.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class,
                                   PROP_HAS_SELECTION,
                                   g_param_spec_boolean ("has-selection",
                                                         P_("Has selection"),
                                                         P_("Whether the buffer has some text currently selected"),
                                                         FALSE,
                                                         BTK_PARAM_READABLE));

  /**
   * BtkTextBuffer:cursor-position:
   *
   * The position of the insert mark (as offset from the beginning 
   * of the buffer). It is useful for getting notified when the 
   * cursor moves.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class,
                                   PROP_CURSOR_POSITION,
                                   g_param_spec_int ("cursor-position",
                                                     P_("Cursor position"),
                                                     P_("The position of the insert mark (as offset from the beginning of the buffer)"),
						     0, B_MAXINT, 0,
                                                     BTK_PARAM_READABLE));

  /**
   * BtkTextBuffer:copy-target-list:
   *
   * The list of targets this buffer supports for clipboard copying
   * and as DND source.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class,
                                   PROP_COPY_TARGET_LIST,
                                   g_param_spec_boxed ("copy-target-list",
                                                       P_("Copy target list"),
                                                       P_("The list of targets this buffer supports for clipboard copying and DND source"),
                                                       BTK_TYPE_TARGET_LIST,
                                                       BTK_PARAM_READABLE));

  /**
   * BtkTextBuffer:paste-target-list:
   *
   * The list of targets this buffer supports for clipboard pasting
   * and as DND destination.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class,
                                   PROP_PASTE_TARGET_LIST,
                                   g_param_spec_boxed ("paste-target-list",
                                                       P_("Paste target list"),
                                                       P_("The list of targets this buffer supports for clipboard pasting and DND destination"),
                                                       BTK_TYPE_TARGET_LIST,
                                                       BTK_PARAM_READABLE));

  /**
   * BtkTextBuffer::insert-text:
   * @textbuffer: the object which received the signal
   * @location: position to insert @text in @textbuffer
   * @text: the UTF-8 text to be inserted
   * @len: length of the inserted text in bytes
   * 
   * The ::insert-text signal is emitted to insert text in a #BtkTextBuffer.
   * Insertion actually occurs in the default handler.  
   * 
   * Note that if your handler runs before the default handler it must not 
   * invalidate the @location iter (or has to revalidate it). 
   * The default signal handler revalidates it to point to the end of the 
   * inserted text.
   * 
   * See also: 
   * btk_text_buffer_insert(), 
   * btk_text_buffer_insert_range().
   */
  signals[INSERT_TEXT] =
    g_signal_new (I_("insert-text"),
                  B_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkTextBufferClass, insert_text),
                  NULL, NULL,
                  _btk_marshal_VOID__BOXED_STRING_INT,
                  B_TYPE_NONE,
                  3,
                  BTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE,
                  B_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE,
                  B_TYPE_INT);

  /**
   * BtkTextBuffer::insert-pixbuf:
   * @textbuffer: the object which received the signal
   * @location: position to insert @pixbuf in @textbuffer
   * @pixbuf: the #BdkPixbuf to be inserted
   * 
   * The ::insert-pixbuf signal is emitted to insert a #BdkPixbuf 
   * in a #BtkTextBuffer. Insertion actually occurs in the default handler.
   * 
   * Note that if your handler runs before the default handler it must not 
   * invalidate the @location iter (or has to revalidate it). 
   * The default signal handler revalidates it to be placed after the 
   * inserted @pixbuf.
   * 
   * See also: btk_text_buffer_insert_pixbuf().
   */
  signals[INSERT_PIXBUF] =
    g_signal_new (I_("insert-pixbuf"),
                  B_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkTextBufferClass, insert_pixbuf),
                  NULL, NULL,
                  _btk_marshal_VOID__BOXED_OBJECT,
                  B_TYPE_NONE,
                  2,
                  BTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE,
                  BDK_TYPE_PIXBUF);


  /**
   * BtkTextBuffer::insert-child-anchor:
   * @textbuffer: the object which received the signal
   * @location: position to insert @anchor in @textbuffer
   * @anchor: the #BtkTextChildAnchor to be inserted
   * 
   * The ::insert-child-anchor signal is emitted to insert a
   * #BtkTextChildAnchor in a #BtkTextBuffer.
   * Insertion actually occurs in the default handler.
   * 
   * Note that if your handler runs before the default handler it must
   * not invalidate the @location iter (or has to revalidate it). 
   * The default signal handler revalidates it to be placed after the 
   * inserted @anchor.
   * 
   * See also: btk_text_buffer_insert_child_anchor().
   */
  signals[INSERT_CHILD_ANCHOR] =
    g_signal_new (I_("insert-child-anchor"),
                  B_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkTextBufferClass, insert_child_anchor),
                  NULL, NULL,
                  _btk_marshal_VOID__BOXED_OBJECT,
                  B_TYPE_NONE,
                  2,
                  BTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE,
                  BTK_TYPE_TEXT_CHILD_ANCHOR);
  
  /**
   * BtkTextBuffer::delete-range:
   * @textbuffer: the object which received the signal
   * @start: the start of the range to be deleted
   * @end: the end of the range to be deleted
   * 
   * The ::delete-range signal is emitted to delete a range 
   * from a #BtkTextBuffer. 
   * 
   * Note that if your handler runs before the default handler it must not 
   * invalidate the @start and @end iters (or has to revalidate them). 
   * The default signal handler revalidates the @start and @end iters to 
   * both point point to the location where text was deleted. Handlers
   * which run after the default handler (see g_signal_connect_after())
   * do not have access to the deleted text.
   * 
   * See also: btk_text_buffer_delete().
   */
  signals[DELETE_RANGE] =
    g_signal_new (I_("delete-range"),
                  B_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkTextBufferClass, delete_range),
                  NULL, NULL,
                  _btk_marshal_VOID__BOXED_BOXED,
                  B_TYPE_NONE,
                  2,
                  BTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE,
                  BTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkTextBuffer::changed:
   * @textbuffer: the object which received the signal
   * 
   * The ::changed signal is emitted when the content of a #BtkTextBuffer 
   * has changed.
   */
  signals[CHANGED] =
    g_signal_new (I_("changed"),
                  B_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,                   
                  G_STRUCT_OFFSET (BtkTextBufferClass, changed),
                  NULL, NULL,
                  _btk_marshal_VOID__VOID,
                  B_TYPE_NONE,
                  0);

  /**
   * BtkTextBuffer::modified-changed:
   * @textbuffer: the object which received the signal
   * 
   * The ::modified-changed signal is emitted when the modified bit of a 
   * #BtkTextBuffer flips.
   * 
   * See also:
   * btk_text_buffer_set_modified().
   */
  signals[MODIFIED_CHANGED] =
    g_signal_new (I_("modified-changed"),
                  B_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkTextBufferClass, modified_changed),
                  NULL, NULL,
                  _btk_marshal_VOID__VOID,
                  B_TYPE_NONE,
                  0);

  /**
   * BtkTextBuffer::mark-set:
   * @textbuffer: the object which received the signal
   * @location: The location of @mark in @textbuffer
   * @mark: The mark that is set
   * 
   * The ::mark-set signal is emitted as notification
   * after a #BtkTextMark is set.
   * 
   * See also: 
   * btk_text_buffer_create_mark(),
   * btk_text_buffer_move_mark().
   */
  signals[MARK_SET] =
    g_signal_new (I_("mark-set"),
                  B_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,                   
                  G_STRUCT_OFFSET (BtkTextBufferClass, mark_set),
                  NULL, NULL,
                  _btk_marshal_VOID__BOXED_OBJECT,
                  B_TYPE_NONE,
                  2,
                  BTK_TYPE_TEXT_ITER,
                  BTK_TYPE_TEXT_MARK);

  /**
   * BtkTextBuffer::mark-deleted:
   * @textbuffer: the object which received the signal
   * @mark: The mark that was deleted
   * 
   * The ::mark-deleted signal is emitted as notification
   * after a #BtkTextMark is deleted. 
   * 
   * See also:
   * btk_text_buffer_delete_mark().
   */
  signals[MARK_DELETED] =
    g_signal_new (I_("mark-deleted"),
                  B_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,                   
                  G_STRUCT_OFFSET (BtkTextBufferClass, mark_deleted),
                  NULL, NULL,
                  _btk_marshal_VOID__OBJECT,
                  B_TYPE_NONE,
                  1,
                  BTK_TYPE_TEXT_MARK);

   /**
   * BtkTextBuffer::apply-tag:
   * @textbuffer: the object which received the signal
   * @tag: the applied tag
   * @start: the start of the range the tag is applied to
   * @end: the end of the range the tag is applied to
   * 
   * The ::apply-tag signal is emitted to apply a tag to a
   * range of text in a #BtkTextBuffer. 
   * Applying actually occurs in the default handler.
   * 
   * Note that if your handler runs before the default handler it must not 
   * invalidate the @start and @end iters (or has to revalidate them). 
   * 
   * See also: 
   * btk_text_buffer_apply_tag(),
   * btk_text_buffer_insert_with_tags(),
   * btk_text_buffer_insert_range().
   */ 
  signals[APPLY_TAG] =
    g_signal_new (I_("apply-tag"),
                  B_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkTextBufferClass, apply_tag),
                  NULL, NULL,
                  _btk_marshal_VOID__OBJECT_BOXED_BOXED,
                  B_TYPE_NONE,
                  3,
                  BTK_TYPE_TEXT_TAG,
                  BTK_TYPE_TEXT_ITER,
                  BTK_TYPE_TEXT_ITER);


   /**
   * BtkTextBuffer::remove-tag:
   * @textbuffer: the object which received the signal
   * @tag: the tag to be removed
   * @start: the start of the range the tag is removed from
   * @end: the end of the range the tag is removed from
   * 
   * The ::remove-tag signal is emitted to remove all occurrences of @tag from
   * a range of text in a #BtkTextBuffer. 
   * Removal actually occurs in the default handler.
   * 
   * Note that if your handler runs before the default handler it must not 
   * invalidate the @start and @end iters (or has to revalidate them). 
   * 
   * See also: 
   * btk_text_buffer_remove_tag(). 
   */ 
  signals[REMOVE_TAG] =
    g_signal_new (I_("remove-tag"),
                  B_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkTextBufferClass, remove_tag),
                  NULL, NULL,
                  _btk_marshal_VOID__OBJECT_BOXED_BOXED,
                  B_TYPE_NONE,
                  3,
                  BTK_TYPE_TEXT_TAG,
                  BTK_TYPE_TEXT_ITER,
                  BTK_TYPE_TEXT_ITER);

   /**
   * BtkTextBuffer::begin-user-action:
   * @textbuffer: the object which received the signal
   * 
   * The ::begin-user-action signal is emitted at the beginning of a single
   * user-visible operation on a #BtkTextBuffer.
   * 
   * See also: 
   * btk_text_buffer_begin_user_action(),
   * btk_text_buffer_insert_interactive(),
   * btk_text_buffer_insert_range_interactive(),
   * btk_text_buffer_delete_interactive(),
   * btk_text_buffer_backspace(),
   * btk_text_buffer_delete_selection().
   */ 
  signals[BEGIN_USER_ACTION] =
    g_signal_new (I_("begin-user-action"),
                  B_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,                   
                  G_STRUCT_OFFSET (BtkTextBufferClass, begin_user_action),
                  NULL, NULL,
                  _btk_marshal_VOID__VOID,
                  B_TYPE_NONE,
                  0);

   /**
   * BtkTextBuffer::end-user-action:
   * @textbuffer: the object which received the signal
   * 
   * The ::end-user-action signal is emitted at the end of a single
   * user-visible operation on the #BtkTextBuffer.
   * 
   * See also: 
   * btk_text_buffer_end_user_action(),
   * btk_text_buffer_insert_interactive(),
   * btk_text_buffer_insert_range_interactive(),
   * btk_text_buffer_delete_interactive(),
   * btk_text_buffer_backspace(),
   * btk_text_buffer_delete_selection(),
   * btk_text_buffer_backspace().
   */ 
  signals[END_USER_ACTION] =
    g_signal_new (I_("end-user-action"),
                  B_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,                   
                  G_STRUCT_OFFSET (BtkTextBufferClass, end_user_action),
                  NULL, NULL,
                  _btk_marshal_VOID__VOID,
                  B_TYPE_NONE,
                  0);

   /**
   * BtkTextBuffer::paste-done:
   * @textbuffer: the object which received the signal
   * 
   * The paste-done signal is emitted after paste operation has been completed.
   * This is useful to properly scroll the view to the end of the pasted text.
   * See btk_text_buffer_paste_clipboard() for more details.
   * 
   * Since: 2.16
   */ 
  signals[PASTE_DONE] =
    g_signal_new (I_("paste-done"),
                  B_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkTextBufferClass, paste_done),
                  NULL, NULL,
                  _btk_marshal_VOID__OBJECT,
                  B_TYPE_NONE,
                  1,
                  BTK_TYPE_CLIPBOARD);

  g_type_class_add_private (object_class, sizeof (BtkTextBufferPrivate));
}

static void
btk_text_buffer_init (BtkTextBuffer *buffer)
{
  buffer->clipboard_contents_buffers = NULL;
  buffer->tag_table = NULL;

  /* allow copying of arbiatray stuff in the internal rich text format */
  btk_text_buffer_register_serialize_tagset (buffer, NULL);
}

static void
set_table (BtkTextBuffer *buffer, BtkTextTagTable *table)
{
  g_return_if_fail (buffer->tag_table == NULL);

  if (table)
    {
      buffer->tag_table = table;
      g_object_ref (buffer->tag_table);
      _btk_text_tag_table_add_buffer (table, buffer);
    }
}

static BtkTextTagTable*
get_table (BtkTextBuffer *buffer)
{
  if (buffer->tag_table == NULL)
    {
      buffer->tag_table = btk_text_tag_table_new ();
      _btk_text_tag_table_add_buffer (buffer->tag_table, buffer);
    }

  return buffer->tag_table;
}

static void
btk_text_buffer_set_property (BObject         *object,
                              buint            prop_id,
                              const BValue    *value,
                              BParamSpec      *pspec)
{
  BtkTextBuffer *text_buffer;

  text_buffer = BTK_TEXT_BUFFER (object);

  switch (prop_id)
    {
    case PROP_TAG_TABLE:
      set_table (text_buffer, b_value_get_object (value));
      break;

    case PROP_TEXT:
      btk_text_buffer_set_text (text_buffer,
				b_value_get_string (value), -1);
      break;

    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_text_buffer_get_property (BObject         *object,
                              buint            prop_id,
                              BValue          *value,
                              BParamSpec      *pspec)
{
  BtkTextBuffer *text_buffer;
  BtkTextIter iter;

  text_buffer = BTK_TEXT_BUFFER (object);

  switch (prop_id)
    {
    case PROP_TAG_TABLE:
      b_value_set_object (value, get_table (text_buffer));
      break;

    case PROP_TEXT:
      {
        BtkTextIter start, end;

        btk_text_buffer_get_start_iter (text_buffer, &start);
        btk_text_buffer_get_end_iter (text_buffer, &end);

        b_value_take_string (value,
                            btk_text_buffer_get_text (text_buffer,
                                                      &start, &end, FALSE));
        break;
      }

    case PROP_HAS_SELECTION:
      b_value_set_boolean (value, text_buffer->has_selection);
      break;

    case PROP_CURSOR_POSITION:
      btk_text_buffer_get_iter_at_mark (text_buffer, &iter, 
    				        btk_text_buffer_get_insert (text_buffer));
      b_value_set_int (value, btk_text_iter_get_offset (&iter));
      break;

    case PROP_COPY_TARGET_LIST:
      b_value_set_boxed (value, btk_text_buffer_get_copy_target_list (text_buffer));
      break;

    case PROP_PASTE_TARGET_LIST:
      b_value_set_boxed (value, btk_text_buffer_get_paste_target_list (text_buffer));
      break;

    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_text_buffer_notify (BObject    *object,
                        BParamSpec *pspec)
{
  if (!strcmp (pspec->name, "copy-target-list") ||
      !strcmp (pspec->name, "paste-target-list"))
    {
      btk_text_buffer_free_target_lists (BTK_TEXT_BUFFER (object));
    }
}

/**
 * btk_text_buffer_new:
 * @table: (allow-none): a tag table, or %NULL to create a new one
 *
 * Creates a new text buffer.
 *
 * Return value: a new text buffer
 **/
BtkTextBuffer*
btk_text_buffer_new (BtkTextTagTable *table)
{
  BtkTextBuffer *text_buffer;

  text_buffer = g_object_new (BTK_TYPE_TEXT_BUFFER, "tag-table", table, NULL);

  return text_buffer;
}

static void
btk_text_buffer_finalize (BObject *object)
{
  BtkTextBuffer *buffer;

  buffer = BTK_TEXT_BUFFER (object);

  remove_all_selection_clipboards (buffer);

  if (buffer->tag_table)
    {
      _btk_text_tag_table_remove_buffer (buffer->tag_table, buffer);
      g_object_unref (buffer->tag_table);
      buffer->tag_table = NULL;
    }

  if (buffer->btree)
    {
      _btk_text_btree_unref (buffer->btree);
      buffer->btree = NULL;
    }

  if (buffer->log_attr_cache)
    free_log_attr_cache (buffer->log_attr_cache);

  buffer->log_attr_cache = NULL;

  btk_text_buffer_free_target_lists (buffer);

  B_OBJECT_CLASS (btk_text_buffer_parent_class)->finalize (object);
}

static BtkTextBTree*
get_btree (BtkTextBuffer *buffer)
{
  if (buffer->btree == NULL)
    buffer->btree = _btk_text_btree_new (btk_text_buffer_get_tag_table (buffer),
                                         buffer);

  return buffer->btree;
}

BtkTextBTree*
_btk_text_buffer_get_btree (BtkTextBuffer *buffer)
{
  return get_btree (buffer);
}

/**
 * btk_text_buffer_get_tag_table:
 * @buffer: a #BtkTextBuffer
 *
 * Get the #BtkTextTagTable associated with this buffer.
 *
 * Return value: (transfer none): the buffer's tag table
 **/
BtkTextTagTable*
btk_text_buffer_get_tag_table (BtkTextBuffer *buffer)
{
  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), NULL);

  return get_table (buffer);
}

/**
 * btk_text_buffer_set_text:
 * @buffer: a #BtkTextBuffer
 * @text: UTF-8 text to insert
 * @len: length of @text in bytes
 *
 * Deletes current contents of @buffer, and inserts @text instead. If
 * @len is -1, @text must be nul-terminated. @text must be valid UTF-8.
 **/
void
btk_text_buffer_set_text (BtkTextBuffer *buffer,
                          const bchar   *text,
                          bint           len)
{
  BtkTextIter start, end;

  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (text != NULL);

  if (len < 0)
    len = strlen (text);

  btk_text_buffer_get_bounds (buffer, &start, &end);

  btk_text_buffer_delete (buffer, &start, &end);

  if (len > 0)
    {
      btk_text_buffer_get_iter_at_offset (buffer, &start, 0);
      btk_text_buffer_insert (buffer, &start, text, len);
    }
  
  g_object_notify (B_OBJECT (buffer), "text");
}

 

/*
 * Insertion
 */

static void
btk_text_buffer_real_insert_text (BtkTextBuffer *buffer,
                                  BtkTextIter   *iter,
                                  const bchar   *text,
                                  bint           len)
{
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  
  _btk_text_btree_insert (iter, text, len);

  g_signal_emit (buffer, signals[CHANGED], 0);
  g_object_notify (B_OBJECT (buffer), "cursor-position");
}

static void
btk_text_buffer_emit_insert (BtkTextBuffer *buffer,
                             BtkTextIter   *iter,
                             const bchar   *text,
                             bint           len)
{
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (text != NULL);

  if (len < 0)
    len = strlen (text);

  g_return_if_fail (g_utf8_validate (text, len, NULL));
  
  if (len > 0)
    {
      g_signal_emit (buffer, signals[INSERT_TEXT], 0,
                     iter, text, len);
    }
}

/**
 * btk_text_buffer_insert:
 * @buffer: a #BtkTextBuffer
 * @iter: a position in the buffer
 * @text: text in UTF-8 format
 * @len: length of text in bytes, or -1
 *
 * Inserts @len bytes of @text at position @iter.  If @len is -1,
 * @text must be nul-terminated and will be inserted in its
 * entirety. Emits the "insert-text" signal; insertion actually occurs
 * in the default handler for the signal. @iter is invalidated when
 * insertion occurs (because the buffer contents change), but the
 * default signal handler revalidates it to point to the end of the
 * inserted text.
 **/
void
btk_text_buffer_insert (BtkTextBuffer *buffer,
                        BtkTextIter   *iter,
                        const bchar   *text,
                        bint           len)
{
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (btk_text_iter_get_buffer (iter) == buffer);
  
  btk_text_buffer_emit_insert (buffer, iter, text, len);
}

/**
 * btk_text_buffer_insert_at_cursor:
 * @buffer: a #BtkTextBuffer
 * @text: text in UTF-8 format
 * @len: length of text, in bytes
 *
 * Simply calls btk_text_buffer_insert(), using the current
 * cursor position as the insertion point.
 **/
void
btk_text_buffer_insert_at_cursor (BtkTextBuffer *buffer,
                                  const bchar   *text,
                                  bint           len)
{
  BtkTextIter iter;

  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (text != NULL);

  btk_text_buffer_get_iter_at_mark (buffer, &iter,
                                    btk_text_buffer_get_insert (buffer));

  btk_text_buffer_insert (buffer, &iter, text, len);
}

/**
 * btk_text_buffer_insert_interactive:
 * @buffer: a #BtkTextBuffer
 * @iter: a position in @buffer
 * @text: some UTF-8 text
 * @len: length of text in bytes, or -1
 * @default_editable: default editability of buffer
 *
 * Like btk_text_buffer_insert(), but the insertion will not occur if
 * @iter is at a non-editable location in the buffer. Usually you
 * want to prevent insertions at ineditable locations if the insertion
 * results from a user action (is interactive).
 *
 * @default_editable indicates the editability of text that doesn't
 * have a tag affecting editability applied to it. Typically the
 * result of btk_text_view_get_editable() is appropriate here.
 *
 * Return value: whether text was actually inserted
 **/
bboolean
btk_text_buffer_insert_interactive (BtkTextBuffer *buffer,
                                    BtkTextIter   *iter,
                                    const bchar   *text,
                                    bint           len,
                                    bboolean       default_editable)
{
  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), FALSE);
  g_return_val_if_fail (text != NULL, FALSE);
  g_return_val_if_fail (btk_text_iter_get_buffer (iter) == buffer, FALSE);

  if (btk_text_iter_can_insert (iter, default_editable))
    {
      btk_text_buffer_begin_user_action (buffer);
      btk_text_buffer_emit_insert (buffer, iter, text, len);
      btk_text_buffer_end_user_action (buffer);
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * btk_text_buffer_insert_interactive_at_cursor:
 * @buffer: a #BtkTextBuffer
 * @text: text in UTF-8 format
 * @len: length of text in bytes, or -1
 * @default_editable: default editability of buffer
 *
 * Calls btk_text_buffer_insert_interactive() at the cursor
 * position.
 *
 * @default_editable indicates the editability of text that doesn't
 * have a tag affecting editability applied to it. Typically the
 * result of btk_text_view_get_editable() is appropriate here.
 * 
 * Return value: whether text was actually inserted
 **/
bboolean
btk_text_buffer_insert_interactive_at_cursor (BtkTextBuffer *buffer,
                                              const bchar   *text,
                                              bint           len,
                                              bboolean       default_editable)
{
  BtkTextIter iter;

  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), FALSE);
  g_return_val_if_fail (text != NULL, FALSE);

  btk_text_buffer_get_iter_at_mark (buffer, &iter,
                                    btk_text_buffer_get_insert (buffer));

  return btk_text_buffer_insert_interactive (buffer, &iter, text, len,
                                             default_editable);
}

static bboolean
possibly_not_text (gunichar ch,
                   bpointer user_data)
{
  return ch == BTK_TEXT_UNKNOWN_CHAR;
}

static void
insert_text_range (BtkTextBuffer     *buffer,
                   BtkTextIter       *iter,
                   const BtkTextIter *orig_start,
                   const BtkTextIter *orig_end,
                   bboolean           interactive)
{
  bchar *text;

  text = btk_text_iter_get_text (orig_start, orig_end);

  btk_text_buffer_emit_insert (buffer, iter, text, -1);

  g_free (text);
}

typedef struct _Range Range;
struct _Range
{
  BtkTextBuffer *buffer;
  BtkTextMark *start_mark;
  BtkTextMark *end_mark;
  BtkTextMark *whole_end_mark;
  BtkTextIter *range_start;
  BtkTextIter *range_end;
  BtkTextIter *whole_end;
};

static Range*
save_range (BtkTextIter *range_start,
            BtkTextIter *range_end,
            BtkTextIter *whole_end)
{
  Range *r;

  r = g_new (Range, 1);

  r->buffer = btk_text_iter_get_buffer (range_start);
  g_object_ref (r->buffer);
  
  r->start_mark = 
    btk_text_buffer_create_mark (btk_text_iter_get_buffer (range_start),
                                 NULL,
                                 range_start,
                                 FALSE);
  r->end_mark = 
    btk_text_buffer_create_mark (btk_text_iter_get_buffer (range_start),
                                 NULL,
                                 range_end,
                                 TRUE);

  r->whole_end_mark = 
    btk_text_buffer_create_mark (btk_text_iter_get_buffer (range_start),
                                 NULL,
                                 whole_end,
                                 TRUE);

  r->range_start = range_start;
  r->range_end = range_end;
  r->whole_end = whole_end;

  return r;
}

static void
restore_range (Range *r)
{
  btk_text_buffer_get_iter_at_mark (r->buffer,
                                    r->range_start,
                                    r->start_mark);
      
  btk_text_buffer_get_iter_at_mark (r->buffer,
                                    r->range_end,
                                    r->end_mark);
      
  btk_text_buffer_get_iter_at_mark (r->buffer,
                                    r->whole_end,
                                    r->whole_end_mark);  
  
  btk_text_buffer_delete_mark (r->buffer, r->start_mark);
  btk_text_buffer_delete_mark (r->buffer, r->end_mark);
  btk_text_buffer_delete_mark (r->buffer, r->whole_end_mark);

  /* Due to the gravities on the marks, the ordering could have
   * gotten mangled; we switch to an empty range in that
   * case
   */
  
  if (btk_text_iter_compare (r->range_start, r->range_end) > 0)
    *r->range_start = *r->range_end;

  if (btk_text_iter_compare (r->range_end, r->whole_end) > 0)
    *r->range_end = *r->whole_end;
  
  g_object_unref (r->buffer);
  g_free (r); 
}

static void
insert_range_untagged (BtkTextBuffer     *buffer,
                       BtkTextIter       *iter,
                       const BtkTextIter *orig_start,
                       const BtkTextIter *orig_end,
                       bboolean           interactive)
{
  BtkTextIter range_start;
  BtkTextIter range_end;
  BtkTextIter start, end;
  Range *r;
  
  if (btk_text_iter_equal (orig_start, orig_end))
    return;

  start = *orig_start;
  end = *orig_end;
  
  range_start = start;
  range_end = start;
  
  while (TRUE)
    {
      if (btk_text_iter_equal (&range_start, &range_end))
        {
          /* Figure out how to move forward */

          g_assert (btk_text_iter_compare (&range_end, &end) <= 0);
          
          if (btk_text_iter_equal (&range_end, &end))
            {
              /* nothing left to do */
              break;
            }
          else if (btk_text_iter_get_char (&range_end) == BTK_TEXT_UNKNOWN_CHAR)
            {
              BdkPixbuf *pixbuf = NULL;
              BtkTextChildAnchor *anchor = NULL;
              pixbuf = btk_text_iter_get_pixbuf (&range_end);
              anchor = btk_text_iter_get_child_anchor (&range_end);

              if (pixbuf)
                {
                  r = save_range (&range_start,
                                  &range_end,
                                  &end);

                  btk_text_buffer_insert_pixbuf (buffer,
                                                 iter,
                                                 pixbuf);

                  restore_range (r);
                  r = NULL;
                  
                  btk_text_iter_forward_char (&range_end);
                  
                  range_start = range_end;
                }
              else if (anchor)
                {
                  /* Just skip anchors */

                  btk_text_iter_forward_char (&range_end);
                  range_start = range_end;
                }
              else
                {
                  /* The BTK_TEXT_UNKNOWN_CHAR was in a text segment, so
                   * keep going. 
                   */
                  btk_text_iter_forward_find_char (&range_end,
                                                   possibly_not_text, NULL,
                                                   &end);
                  
                  g_assert (btk_text_iter_compare (&range_end, &end) <= 0);
                }
            }
          else
            {
              /* Text segment starts here, so forward search to
               * find its possible endpoint
               */
              btk_text_iter_forward_find_char (&range_end,
                                               possibly_not_text, NULL,
                                               &end);
              
              g_assert (btk_text_iter_compare (&range_end, &end) <= 0);
            }
        }
      else
        {
          r = save_range (&range_start,
                          &range_end,
                          &end);
          
          insert_text_range (buffer,
                             iter,
                             &range_start,
                             &range_end,
                             interactive);

          restore_range (r);
          r = NULL;
          
          range_start = range_end;
        }
    }
}

static void
insert_range_not_inside_self (BtkTextBuffer     *buffer,
                              BtkTextIter       *iter,
                              const BtkTextIter *orig_start,
                              const BtkTextIter *orig_end,
                              bboolean           interactive)
{
  /* Find each range of uniformly-tagged text, insert it,
   * then apply the tags.
   */
  BtkTextIter start = *orig_start;
  BtkTextIter end = *orig_end;
  BtkTextIter range_start;
  BtkTextIter range_end;
  
  if (btk_text_iter_equal (orig_start, orig_end))
    return;
  
  btk_text_iter_order (&start, &end);

  range_start = start;
  range_end = start;  
  
  while (TRUE)
    {
      bint start_offset;
      BtkTextIter start_iter;
      GSList *tags;
      GSList *tmp_list;
      Range *r;
      
      if (btk_text_iter_equal (&range_start, &end))
        break; /* All done */

      g_assert (btk_text_iter_compare (&range_start, &end) < 0);
      
      btk_text_iter_forward_to_tag_toggle (&range_end, NULL);

      g_assert (!btk_text_iter_equal (&range_start, &range_end));

      /* Clamp to the end iterator */
      if (btk_text_iter_compare (&range_end, &end) > 0)
        range_end = end;
      
      /* We have a range with unique tags; insert it, and
       * apply all tags.
       */
      start_offset = btk_text_iter_get_offset (iter);

      r = save_range (&range_start, &range_end, &end);
      
      insert_range_untagged (buffer, iter, &range_start, &range_end, interactive);

      restore_range (r);
      r = NULL;
      
      btk_text_buffer_get_iter_at_offset (buffer, &start_iter, start_offset);
      
      tags = btk_text_iter_get_tags (&range_start);
      tmp_list = tags;
      while (tmp_list != NULL)
        {
          btk_text_buffer_apply_tag (buffer,
                                     tmp_list->data,
                                     &start_iter,
                                     iter);
          
          tmp_list = b_slist_next (tmp_list);
        }
      b_slist_free (tags);

      range_start = range_end;
    }
}

static void
btk_text_buffer_real_insert_range (BtkTextBuffer     *buffer,
                                   BtkTextIter       *iter,
                                   const BtkTextIter *orig_start,
                                   const BtkTextIter *orig_end,
                                   bboolean           interactive)
{
  BtkTextBuffer *src_buffer;
  
  /* Find each range of uniformly-tagged text, insert it,
   * then apply the tags.
   */  
  if (btk_text_iter_equal (orig_start, orig_end))
    return;

  if (interactive)
    btk_text_buffer_begin_user_action (buffer);
  
  src_buffer = btk_text_iter_get_buffer (orig_start);
  
  if (btk_text_iter_get_buffer (iter) != src_buffer ||
      !btk_text_iter_in_range (iter, orig_start, orig_end))
    {
      insert_range_not_inside_self (buffer, iter, orig_start, orig_end, interactive);
    }
  else
    {
      /* If you insert a range into itself, it could loop infinitely
       * because the rebunnyion being copied keeps growing as we insert. So
       * we have to separately copy the range before and after
       * the insertion point.
       */
      BtkTextIter start = *orig_start;
      BtkTextIter end = *orig_end;
      BtkTextIter range_start;
      BtkTextIter range_end;
      Range *first_half;
      Range *second_half;

      btk_text_iter_order (&start, &end);
      
      range_start = start;
      range_end = *iter;
      first_half = save_range (&range_start, &range_end, &end);

      range_start = *iter;
      range_end = end;
      second_half = save_range (&range_start, &range_end, &end);

      restore_range (first_half);
      insert_range_not_inside_self (buffer, iter, &range_start, &range_end, interactive);

      restore_range (second_half);
      insert_range_not_inside_self (buffer, iter, &range_start, &range_end, interactive);
    }
  
  if (interactive)
    btk_text_buffer_end_user_action (buffer);
}

/**
 * btk_text_buffer_insert_range:
 * @buffer: a #BtkTextBuffer
 * @iter: a position in @buffer
 * @start: a position in a #BtkTextBuffer
 * @end: another position in the same buffer as @start
 *
 * Copies text, tags, and pixbufs between @start and @end (the order
 * of @start and @end doesn't matter) and inserts the copy at @iter.
 * Used instead of simply getting/inserting text because it preserves
 * images and tags. If @start and @end are in a different buffer from
 * @buffer, the two buffers must share the same tag table.
 *
 * Implemented via emissions of the insert_text and apply_tag signals,
 * so expect those.
 **/
void
btk_text_buffer_insert_range (BtkTextBuffer     *buffer,
                              BtkTextIter       *iter,
                              const BtkTextIter *start,
                              const BtkTextIter *end)
{
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (btk_text_iter_get_buffer (start) ==
                    btk_text_iter_get_buffer (end));
  g_return_if_fail (btk_text_iter_get_buffer (start)->tag_table ==
                    buffer->tag_table);  
  g_return_if_fail (btk_text_iter_get_buffer (iter) == buffer);
  
  btk_text_buffer_real_insert_range (buffer, iter, start, end, FALSE);
}

/**
 * btk_text_buffer_insert_range_interactive:
 * @buffer: a #BtkTextBuffer
 * @iter: a position in @buffer
 * @start: a position in a #BtkTextBuffer
 * @end: another position in the same buffer as @start
 * @default_editable: default editability of the buffer
 *
 * Same as btk_text_buffer_insert_range(), but does nothing if the
 * insertion point isn't editable. The @default_editable parameter
 * indicates whether the text is editable at @iter if no tags
 * enclosing @iter affect editability. Typically the result of
 * btk_text_view_get_editable() is appropriate here.
 *
 * Returns: whether an insertion was possible at @iter
 **/
bboolean
btk_text_buffer_insert_range_interactive (BtkTextBuffer     *buffer,
                                          BtkTextIter       *iter,
                                          const BtkTextIter *start,
                                          const BtkTextIter *end,
                                          bboolean           default_editable)
{
  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (start != NULL, FALSE);
  g_return_val_if_fail (end != NULL, FALSE);
  g_return_val_if_fail (btk_text_iter_get_buffer (start) ==
                        btk_text_iter_get_buffer (end), FALSE);
  g_return_val_if_fail (btk_text_iter_get_buffer (start)->tag_table ==
                        buffer->tag_table, FALSE);

  if (btk_text_iter_can_insert (iter, default_editable))
    {
      btk_text_buffer_real_insert_range (buffer, iter, start, end, TRUE);
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * btk_text_buffer_insert_with_tags:
 * @buffer: a #BtkTextBuffer
 * @iter: an iterator in @buffer
 * @text: UTF-8 text
 * @len: length of @text, or -1
 * @first_tag: first tag to apply to @text
 * @Varargs: NULL-terminated list of tags to apply
 *
 * Inserts @text into @buffer at @iter, applying the list of tags to
 * the newly-inserted text. The last tag specified must be NULL to
 * terminate the list. Equivalent to calling btk_text_buffer_insert(),
 * then btk_text_buffer_apply_tag() on the inserted text;
 * btk_text_buffer_insert_with_tags() is just a convenience function.
 **/
void
btk_text_buffer_insert_with_tags (BtkTextBuffer *buffer,
                                  BtkTextIter   *iter,
                                  const bchar   *text,
                                  bint           len,
                                  BtkTextTag    *first_tag,
                                  ...)
{
  bint start_offset;
  BtkTextIter start;
  va_list args;
  BtkTextTag *tag;

  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (btk_text_iter_get_buffer (iter) == buffer);
  
  start_offset = btk_text_iter_get_offset (iter);

  btk_text_buffer_insert (buffer, iter, text, len);

  if (first_tag == NULL)
    return;

  btk_text_buffer_get_iter_at_offset (buffer, &start, start_offset);

  va_start (args, first_tag);
  tag = first_tag;
  while (tag)
    {
      btk_text_buffer_apply_tag (buffer, tag, &start, iter);

      tag = va_arg (args, BtkTextTag*);
    }

  va_end (args);
}

/**
 * btk_text_buffer_insert_with_tags_by_name:
 * @buffer: a #BtkTextBuffer
 * @iter: position in @buffer
 * @text: UTF-8 text
 * @len: length of @text, or -1
 * @first_tag_name: name of a tag to apply to @text
 * @Varargs: more tag names
 *
 * Same as btk_text_buffer_insert_with_tags(), but allows you
 * to pass in tag names instead of tag objects.
 **/
void
btk_text_buffer_insert_with_tags_by_name  (BtkTextBuffer *buffer,
                                           BtkTextIter   *iter,
                                           const bchar   *text,
                                           bint           len,
                                           const bchar   *first_tag_name,
                                           ...)
{
  bint start_offset;
  BtkTextIter start;
  va_list args;
  const bchar *tag_name;

  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (btk_text_iter_get_buffer (iter) == buffer);
  
  start_offset = btk_text_iter_get_offset (iter);

  btk_text_buffer_insert (buffer, iter, text, len);

  if (first_tag_name == NULL)
    return;

  btk_text_buffer_get_iter_at_offset (buffer, &start, start_offset);

  va_start (args, first_tag_name);
  tag_name = first_tag_name;
  while (tag_name)
    {
      BtkTextTag *tag;

      tag = btk_text_tag_table_lookup (buffer->tag_table,
                                       tag_name);

      if (tag == NULL)
        {
          g_warning ("%s: no tag with name '%s'!", B_STRLOC, tag_name);
          va_end (args);
          return;
        }

      btk_text_buffer_apply_tag (buffer, tag, &start, iter);

      tag_name = va_arg (args, const bchar*);
    }

  va_end (args);
}


/*
 * Deletion
 */

static void
btk_text_buffer_real_delete_range (BtkTextBuffer *buffer,
                                   BtkTextIter   *start,
                                   BtkTextIter   *end)
{
  bboolean has_selection;

  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);

  _btk_text_btree_delete (start, end);

  /* may have deleted the selection... */
  update_selection_clipboards (buffer);

  has_selection = btk_text_buffer_get_selection_bounds (buffer, NULL, NULL);
  if (has_selection != buffer->has_selection)
    {
      buffer->has_selection = has_selection;
      g_object_notify (B_OBJECT (buffer), "has-selection");
    }

  g_signal_emit (buffer, signals[CHANGED], 0);
  g_object_notify (B_OBJECT (buffer), "cursor-position");
}

static void
btk_text_buffer_emit_delete (BtkTextBuffer *buffer,
                             BtkTextIter *start,
                             BtkTextIter *end)
{
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);

  if (btk_text_iter_equal (start, end))
    return;

  btk_text_iter_order (start, end);

  g_signal_emit (buffer,
                 signals[DELETE_RANGE],
                 0,
                 start, end);
}

/**
 * btk_text_buffer_delete:
 * @buffer: a #BtkTextBuffer
 * @start: a position in @buffer
 * @end: another position in @buffer
 *
 * Deletes text between @start and @end. The order of @start and @end
 * is not actually relevant; btk_text_buffer_delete() will reorder
 * them. This function actually emits the "delete-range" signal, and
 * the default handler of that signal deletes the text. Because the
 * buffer is modified, all outstanding iterators become invalid after
 * calling this function; however, the @start and @end will be
 * re-initialized to point to the location where text was deleted.
 **/
void
btk_text_buffer_delete (BtkTextBuffer *buffer,
                        BtkTextIter   *start,
                        BtkTextIter   *end)
{
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (btk_text_iter_get_buffer (start) == buffer);
  g_return_if_fail (btk_text_iter_get_buffer (end) == buffer);
  
  btk_text_buffer_emit_delete (buffer, start, end);
}

/**
 * btk_text_buffer_delete_interactive:
 * @buffer: a #BtkTextBuffer
 * @start_iter: start of range to delete
 * @end_iter: end of range
 * @default_editable: whether the buffer is editable by default
 *
 * Deletes all <emphasis>editable</emphasis> text in the given range.
 * Calls btk_text_buffer_delete() for each editable sub-range of
 * [@start,@end). @start and @end are revalidated to point to
 * the location of the last deleted range, or left untouched if
 * no text was deleted.
 *
 * Return value: whether some text was actually deleted
 **/
bboolean
btk_text_buffer_delete_interactive (BtkTextBuffer *buffer,
                                    BtkTextIter   *start_iter,
                                    BtkTextIter   *end_iter,
                                    bboolean       default_editable)
{
  BtkTextMark *end_mark;
  BtkTextMark *start_mark;
  BtkTextIter iter;
  bboolean current_state;
  bboolean deleted_stuff = FALSE;

  /* Delete all editable text in the range start_iter, end_iter */

  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), FALSE);
  g_return_val_if_fail (start_iter != NULL, FALSE);
  g_return_val_if_fail (end_iter != NULL, FALSE);
  g_return_val_if_fail (btk_text_iter_get_buffer (start_iter) == buffer, FALSE);
  g_return_val_if_fail (btk_text_iter_get_buffer (end_iter) == buffer, FALSE);

  
  btk_text_buffer_begin_user_action (buffer);
  
  btk_text_iter_order (start_iter, end_iter);

  start_mark = btk_text_buffer_create_mark (buffer, NULL,
                                            start_iter, TRUE);
  end_mark = btk_text_buffer_create_mark (buffer, NULL,
                                          end_iter, FALSE);

  btk_text_buffer_get_iter_at_mark (buffer, &iter, start_mark);

  current_state = btk_text_iter_editable (&iter, default_editable);

  while (TRUE)
    {
      bboolean new_state;
      bboolean done = FALSE;
      BtkTextIter end;

      btk_text_iter_forward_to_tag_toggle (&iter, NULL);

      btk_text_buffer_get_iter_at_mark (buffer, &end, end_mark);

      if (btk_text_iter_compare (&iter, &end) >= 0)
        {
          done = TRUE;
          iter = end; /* clamp to the last boundary */
        }

      new_state = btk_text_iter_editable (&iter, default_editable);

      if (current_state == new_state)
        {
          if (done)
            {
              if (current_state)
                {
                  /* We're ending an editable rebunnyion. Delete said rebunnyion. */
                  BtkTextIter start;

                  btk_text_buffer_get_iter_at_mark (buffer, &start, start_mark);

                  btk_text_buffer_emit_delete (buffer, &start, &iter);

                  deleted_stuff = TRUE;

                  /* revalidate user's iterators. */
                  *start_iter = start;
                  *end_iter = iter;
                }

              break;
            }
          else
            continue;
        }

      if (current_state && !new_state)
        {
          /* End of an editable rebunnyion. Delete it. */
          BtkTextIter start;

          btk_text_buffer_get_iter_at_mark (buffer, &start, start_mark);

          btk_text_buffer_emit_delete (buffer, &start, &iter);

	  /* It's more robust to ask for the state again then to assume that
	   * we're on the next not-editable segment. We don't know what the
	   * ::delete-range handler did.... maybe it deleted the following
           * not-editable segment because it was associated with the editable
           * segment.
	   */
	  current_state = btk_text_iter_editable (&iter, default_editable);
          deleted_stuff = TRUE;

          /* revalidate user's iterators. */
          *start_iter = start;
          *end_iter = iter;
        }
      else
        {
          /* We are at the start of an editable rebunnyion. We won't be deleting
           * the previous rebunnyion. Move start mark to start of this rebunnyion.
           */

          g_assert (!current_state && new_state);

          btk_text_buffer_move_mark (buffer, start_mark, &iter);

          current_state = TRUE;
        }

      if (done)
        break;
    }

  btk_text_buffer_delete_mark (buffer, start_mark);
  btk_text_buffer_delete_mark (buffer, end_mark);

  btk_text_buffer_end_user_action (buffer);
  
  return deleted_stuff;
}

/*
 * Extracting textual buffer contents
 */

/**
 * btk_text_buffer_get_text:
 * @buffer: a #BtkTextBuffer
 * @start: start of a range
 * @end: end of a range
 * @include_hidden_chars: whether to include invisible text
 *
 * Returns the text in the range [@start,@end). Excludes undisplayed
 * text (text marked with tags that set the invisibility attribute) if
 * @include_hidden_chars is %FALSE. Does not include characters
 * representing embedded images, so byte and character indexes into
 * the returned string do <emphasis>not</emphasis> correspond to byte
 * and character indexes into the buffer. Contrast with
 * btk_text_buffer_get_slice().
 *
 * Return value: (transfer full): an allocated UTF-8 string
 **/
bchar*
btk_text_buffer_get_text (BtkTextBuffer     *buffer,
                          const BtkTextIter *start,
                          const BtkTextIter *end,
                          bboolean           include_hidden_chars)
{
  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (start != NULL, NULL);
  g_return_val_if_fail (end != NULL, NULL);
  g_return_val_if_fail (btk_text_iter_get_buffer (start) == buffer, NULL);
  g_return_val_if_fail (btk_text_iter_get_buffer (end) == buffer, NULL);
  
  if (include_hidden_chars)
    return btk_text_iter_get_text (start, end);
  else
    return btk_text_iter_get_visible_text (start, end);
}

/**
 * btk_text_buffer_get_slice:
 * @buffer: a #BtkTextBuffer
 * @start: start of a range
 * @end: end of a range
 * @include_hidden_chars: whether to include invisible text
 *
 * Returns the text in the range [@start,@end). Excludes undisplayed
 * text (text marked with tags that set the invisibility attribute) if
 * @include_hidden_chars is %FALSE. The returned string includes a
 * 0xFFFC character whenever the buffer contains
 * embedded images, so byte and character indexes into
 * the returned string <emphasis>do</emphasis> correspond to byte
 * and character indexes into the buffer. Contrast with
 * btk_text_buffer_get_text(). Note that 0xFFFC can occur in normal
 * text as well, so it is not a reliable indicator that a pixbuf or
 * widget is in the buffer.
 *
 * Return value: (transfer full): an allocated UTF-8 string
 **/
bchar*
btk_text_buffer_get_slice (BtkTextBuffer     *buffer,
                           const BtkTextIter *start,
                           const BtkTextIter *end,
                           bboolean           include_hidden_chars)
{
  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (start != NULL, NULL);
  g_return_val_if_fail (end != NULL, NULL);
  g_return_val_if_fail (btk_text_iter_get_buffer (start) == buffer, NULL);
  g_return_val_if_fail (btk_text_iter_get_buffer (end) == buffer, NULL);
  
  if (include_hidden_chars)
    return btk_text_iter_get_slice (start, end);
  else
    return btk_text_iter_get_visible_slice (start, end);
}

/*
 * Pixbufs
 */

static void
btk_text_buffer_real_insert_pixbuf (BtkTextBuffer *buffer,
                                    BtkTextIter   *iter,
                                    BdkPixbuf     *pixbuf)
{ 
  _btk_text_btree_insert_pixbuf (iter, pixbuf);

  g_signal_emit (buffer, signals[CHANGED], 0);
}

/**
 * btk_text_buffer_insert_pixbuf:
 * @buffer: a #BtkTextBuffer
 * @iter: location to insert the pixbuf
 * @pixbuf: a #BdkPixbuf
 *
 * Inserts an image into the text buffer at @iter. The image will be
 * counted as one character in character counts, and when obtaining
 * the buffer contents as a string, will be represented by the Unicode
 * "object replacement character" 0xFFFC. Note that the "slice"
 * variants for obtaining portions of the buffer as a string include
 * this character for pixbufs, but the "text" variants do
 * not. e.g. see btk_text_buffer_get_slice() and
 * btk_text_buffer_get_text().
 **/
void
btk_text_buffer_insert_pixbuf (BtkTextBuffer *buffer,
                               BtkTextIter   *iter,
                               BdkPixbuf     *pixbuf)
{
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (BDK_IS_PIXBUF (pixbuf));
  g_return_if_fail (btk_text_iter_get_buffer (iter) == buffer);
  
  g_signal_emit (buffer, signals[INSERT_PIXBUF], 0,
                 iter, pixbuf);
}

/*
 * Child anchor
 */


static void
btk_text_buffer_real_insert_anchor (BtkTextBuffer      *buffer,
                                    BtkTextIter        *iter,
                                    BtkTextChildAnchor *anchor)
{
  _btk_text_btree_insert_child_anchor (iter, anchor);

  g_signal_emit (buffer, signals[CHANGED], 0);
}

/**
 * btk_text_buffer_insert_child_anchor:
 * @buffer: a #BtkTextBuffer
 * @iter: location to insert the anchor
 * @anchor: a #BtkTextChildAnchor
 *
 * Inserts a child widget anchor into the text buffer at @iter. The
 * anchor will be counted as one character in character counts, and
 * when obtaining the buffer contents as a string, will be represented
 * by the Unicode "object replacement character" 0xFFFC. Note that the
 * "slice" variants for obtaining portions of the buffer as a string
 * include this character for child anchors, but the "text" variants do
 * not. E.g. see btk_text_buffer_get_slice() and
 * btk_text_buffer_get_text(). Consider
 * btk_text_buffer_create_child_anchor() as a more convenient
 * alternative to this function. The buffer will add a reference to
 * the anchor, so you can unref it after insertion.
 **/
void
btk_text_buffer_insert_child_anchor (BtkTextBuffer      *buffer,
                                     BtkTextIter        *iter,
                                     BtkTextChildAnchor *anchor)
{
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (BTK_IS_TEXT_CHILD_ANCHOR (anchor));
  g_return_if_fail (btk_text_iter_get_buffer (iter) == buffer);
  
  g_signal_emit (buffer, signals[INSERT_CHILD_ANCHOR], 0,
                 iter, anchor);
}

/**
 * btk_text_buffer_create_child_anchor:
 * @buffer: a #BtkTextBuffer
 * @iter: location in the buffer
 * 
 * This is a convenience function which simply creates a child anchor
 * with btk_text_child_anchor_new() and inserts it into the buffer
 * with btk_text_buffer_insert_child_anchor(). The new anchor is
 * owned by the buffer; no reference count is returned to
 * the caller of btk_text_buffer_create_child_anchor().
 * 
 * Return value: (transfer none): the created child anchor
 **/
BtkTextChildAnchor*
btk_text_buffer_create_child_anchor (BtkTextBuffer *buffer,
                                     BtkTextIter   *iter)
{
  BtkTextChildAnchor *anchor;
  
  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (iter != NULL, NULL);
  g_return_val_if_fail (btk_text_iter_get_buffer (iter) == buffer, NULL);
  
  anchor = btk_text_child_anchor_new ();

  btk_text_buffer_insert_child_anchor (buffer, iter, anchor);

  g_object_unref (anchor);

  return anchor;
}

/*
 * Mark manipulation
 */

static void
btk_text_buffer_mark_set (BtkTextBuffer     *buffer,
                          const BtkTextIter *location,
                          BtkTextMark       *mark)
{
  /* IMO this should NOT work like insert_text and delete_range,
   * where the real action happens in the default handler.
   * 
   * The reason is that the default handler would be _required_,
   * i.e. the whole widget would start breaking and segfaulting if the
   * default handler didn't get run. So you can't really override the
   * default handler or stop the emission; that is, this signal is
   * purely for notification, and not to allow users to modify the
   * default behavior.
   */

  g_object_ref (mark);

  g_signal_emit (buffer,
                 signals[MARK_SET],
                 0,
                 location,
                 mark);

  g_object_unref (mark);
}

/**
 * btk_text_buffer_set_mark:
 * @buffer:       a #BtkTextBuffer
 * @mark_name:    name of the mark
 * @iter:         location for the mark
 * @left_gravity: if the mark is created by this function, gravity for 
 *                the new mark
 * @should_exist: if %TRUE, warn if the mark does not exist, and return
 *                immediately
 *
 * Move the mark to the given position, if not @should_exist, 
 * create the mark.
 *
 * Return value: mark
 **/
static BtkTextMark*
btk_text_buffer_set_mark (BtkTextBuffer     *buffer,
                          BtkTextMark       *existing_mark,
                          const bchar       *mark_name,
                          const BtkTextIter *iter,
                          bboolean           left_gravity,
                          bboolean           should_exist)
{
  BtkTextIter location;
  BtkTextMark *mark;

  g_return_val_if_fail (btk_text_iter_get_buffer (iter) == buffer, NULL);
  
  mark = _btk_text_btree_set_mark (get_btree (buffer),
                                   existing_mark,
                                   mark_name,
                                   left_gravity,
                                   iter,
                                   should_exist);
  
  _btk_text_btree_get_iter_at_mark (get_btree (buffer),
                                   &location,
                                   mark);

  btk_text_buffer_mark_set (buffer, &location, mark);

  return mark;
}

/**
 * btk_text_buffer_create_mark:
 * @buffer: a #BtkTextBuffer
 * @mark_name: (allow-none): name for mark, or %NULL
 * @where: location to place mark
 * @left_gravity: whether the mark has left gravity
 *
 * Creates a mark at position @where. If @mark_name is %NULL, the mark
 * is anonymous; otherwise, the mark can be retrieved by name using
 * btk_text_buffer_get_mark(). If a mark has left gravity, and text is
 * inserted at the mark's current location, the mark will be moved to
 * the left of the newly-inserted text. If the mark has right gravity
 * (@left_gravity = %FALSE), the mark will end up on the right of
 * newly-inserted text. The standard left-to-right cursor is a mark
 * with right gravity (when you type, the cursor stays on the right
 * side of the text you're typing).
 *
 * The caller of this function does <emphasis>not</emphasis> own a 
 * reference to the returned #BtkTextMark, so you can ignore the 
 * return value if you like. Marks are owned by the buffer and go 
 * away when the buffer does.
 *
 * Emits the "mark-set" signal as notification of the mark's initial
 * placement.
 *
 * Return value: (transfer none): the new #BtkTextMark object
 **/
BtkTextMark*
btk_text_buffer_create_mark (BtkTextBuffer     *buffer,
                             const bchar       *mark_name,
                             const BtkTextIter *where,
                             bboolean           left_gravity)
{
  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), NULL);

  return btk_text_buffer_set_mark (buffer, NULL, mark_name, where,
                                   left_gravity, FALSE);
}

/**
 * btk_text_buffer_add_mark:
 * @buffer: a #BtkTextBuffer
 * @mark: the mark to add
 * @where: location to place mark
 *
 * Adds the mark at position @where. The mark must not be added to
 * another buffer, and if its name is not %NULL then there must not
 * be another mark in the buffer with the same name.
 *
 * Emits the "mark-set" signal as notification of the mark's initial
 * placement.
 *
 * Since: 2.12
 **/
void
btk_text_buffer_add_mark (BtkTextBuffer     *buffer,
                          BtkTextMark       *mark,
                          const BtkTextIter *where)
{
  const bchar *name;

  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (BTK_IS_TEXT_MARK (mark));
  g_return_if_fail (where != NULL);
  g_return_if_fail (btk_text_mark_get_buffer (mark) == NULL);

  name = btk_text_mark_get_name (mark);

  if (name != NULL && btk_text_buffer_get_mark (buffer, name) != NULL)
    {
      g_critical ("Mark %s already exists in the buffer", name);
      return;
    }

  btk_text_buffer_set_mark (buffer, mark, NULL, where, FALSE, FALSE);
}

/**
 * btk_text_buffer_move_mark:
 * @buffer: a #BtkTextBuffer
 * @mark: a #BtkTextMark
 * @where: new location for @mark in @buffer
 *
 * Moves @mark to the new location @where. Emits the "mark-set" signal
 * as notification of the move.
 **/
void
btk_text_buffer_move_mark (BtkTextBuffer     *buffer,
                           BtkTextMark       *mark,
                           const BtkTextIter *where)
{
  g_return_if_fail (BTK_IS_TEXT_MARK (mark));
  g_return_if_fail (!btk_text_mark_get_deleted (mark));
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));

  btk_text_buffer_set_mark (buffer, mark, NULL, where, FALSE, TRUE);
}

/**
 * btk_text_buffer_get_iter_at_mark:
 * @buffer: a #BtkTextBuffer
 * @iter: (out): iterator to initialize
 * @mark: a #BtkTextMark in @buffer
 *
 * Initializes @iter with the current position of @mark.
 **/
void
btk_text_buffer_get_iter_at_mark (BtkTextBuffer *buffer,
                                  BtkTextIter   *iter,
                                  BtkTextMark   *mark)
{
  g_return_if_fail (BTK_IS_TEXT_MARK (mark));
  g_return_if_fail (!btk_text_mark_get_deleted (mark));
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));

  _btk_text_btree_get_iter_at_mark (get_btree (buffer),
                                    iter,
                                    mark);
}

/**
 * btk_text_buffer_delete_mark:
 * @buffer: a #BtkTextBuffer
 * @mark: a #BtkTextMark in @buffer
 *
 * Deletes @mark, so that it's no longer located anywhere in the
 * buffer. Removes the reference the buffer holds to the mark, so if
 * you haven't called g_object_ref() on the mark, it will be freed. Even
 * if the mark isn't freed, most operations on @mark become
 * invalid, until it gets added to a buffer again with 
 * btk_text_buffer_add_mark(). Use btk_text_mark_get_deleted() to  
 * find out if a mark has been removed from its buffer.
 * The "mark-deleted" signal will be emitted as notification after 
 * the mark is deleted.
 **/
void
btk_text_buffer_delete_mark (BtkTextBuffer *buffer,
                             BtkTextMark   *mark)
{
  g_return_if_fail (BTK_IS_TEXT_MARK (mark));
  g_return_if_fail (!btk_text_mark_get_deleted (mark));
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));

  g_object_ref (mark);

  _btk_text_btree_remove_mark (get_btree (buffer), mark);

  /* See rationale above for MARK_SET on why we emit this after
   * removing the mark, rather than removing the mark in a default
   * handler.
   */
  g_signal_emit (buffer, signals[MARK_DELETED],
                 0,
                 mark);

  g_object_unref (mark);
}

/**
 * btk_text_buffer_get_mark:
 * @buffer: a #BtkTextBuffer
 * @name: a mark name
 *
 * Returns the mark named @name in buffer @buffer, or %NULL if no such
 * mark exists in the buffer.
 *
 * Return value: (transfer none): a #BtkTextMark, or %NULL
 **/
BtkTextMark*
btk_text_buffer_get_mark (BtkTextBuffer *buffer,
                          const bchar   *name)
{
  BtkTextMark *mark;

  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  mark = _btk_text_btree_get_mark_by_name (get_btree (buffer), name);

  return mark;
}

/**
 * btk_text_buffer_move_mark_by_name:
 * @buffer: a #BtkTextBuffer
 * @name: name of a mark
 * @where: new location for mark
 *
 * Moves the mark named @name (which must exist) to location @where.
 * See btk_text_buffer_move_mark() for details.
 **/
void
btk_text_buffer_move_mark_by_name (BtkTextBuffer     *buffer,
                                   const bchar       *name,
                                   const BtkTextIter *where)
{
  BtkTextMark *mark;

  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (name != NULL);

  mark = _btk_text_btree_get_mark_by_name (get_btree (buffer), name);

  if (mark == NULL)
    {
      g_warning ("%s: no mark named '%s'", B_STRLOC, name);
      return;
    }

  btk_text_buffer_move_mark (buffer, mark, where);
}

/**
 * btk_text_buffer_delete_mark_by_name:
 * @buffer: a #BtkTextBuffer
 * @name: name of a mark in @buffer
 *
 * Deletes the mark named @name; the mark must exist. See
 * btk_text_buffer_delete_mark() for details.
 **/
void
btk_text_buffer_delete_mark_by_name (BtkTextBuffer *buffer,
                                     const bchar   *name)
{
  BtkTextMark *mark;

  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (name != NULL);

  mark = _btk_text_btree_get_mark_by_name (get_btree (buffer), name);

  if (mark == NULL)
    {
      g_warning ("%s: no mark named '%s'", B_STRLOC, name);
      return;
    }

  btk_text_buffer_delete_mark (buffer, mark);
}

/**
 * btk_text_buffer_get_insert:
 * @buffer: a #BtkTextBuffer
 *
 * Returns the mark that represents the cursor (insertion point).
 * Equivalent to calling btk_text_buffer_get_mark() to get the mark
 * named "insert", but very slightly more efficient, and involves less
 * typing.
 *
 * Return value: (transfer none): insertion point mark
 **/
BtkTextMark*
btk_text_buffer_get_insert (BtkTextBuffer *buffer)
{
  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), NULL);

  return _btk_text_btree_get_insert (get_btree (buffer));
}

/**
 * btk_text_buffer_get_selection_bound:
 * @buffer: a #BtkTextBuffer
 *
 * Returns the mark that represents the selection bound.  Equivalent
 * to calling btk_text_buffer_get_mark() to get the mark named
 * "selection_bound", but very slightly more efficient, and involves
 * less typing.
 *
 * The currently-selected text in @buffer is the rebunnyion between the
 * "selection_bound" and "insert" marks. If "selection_bound" and
 * "insert" are in the same place, then there is no current selection.
 * btk_text_buffer_get_selection_bounds() is another convenient function
 * for handling the selection, if you just want to know whether there's a
 * selection and what its bounds are.
 *
 * Return value: (transfer none): selection bound mark
 **/
BtkTextMark*
btk_text_buffer_get_selection_bound (BtkTextBuffer *buffer)
{
  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), NULL);

  return _btk_text_btree_get_selection_bound (get_btree (buffer));
}

/**
 * btk_text_buffer_get_iter_at_child_anchor:
 * @buffer: a #BtkTextBuffer
 * @iter: (out): an iterator to be initialized
 * @anchor: a child anchor that appears in @buffer
 *
 * Obtains the location of @anchor within @buffer.
 **/
void
btk_text_buffer_get_iter_at_child_anchor (BtkTextBuffer      *buffer,
                                          BtkTextIter        *iter,
                                          BtkTextChildAnchor *anchor)
{
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (BTK_IS_TEXT_CHILD_ANCHOR (anchor));
  g_return_if_fail (!btk_text_child_anchor_get_deleted (anchor));
  
  _btk_text_btree_get_iter_at_child_anchor (get_btree (buffer),
                                           iter,
                                           anchor);
}

/**
 * btk_text_buffer_place_cursor:
 * @buffer: a #BtkTextBuffer
 * @where: where to put the cursor
 *
 * This function moves the "insert" and "selection_bound" marks
 * simultaneously.  If you move them to the same place in two steps
 * with btk_text_buffer_move_mark(), you will temporarily select a
 * rebunnyion in between their old and new locations, which can be pretty
 * inefficient since the temporarily-selected rebunnyion will force stuff
 * to be recalculated. This function moves them as a unit, which can
 * be optimized.
 **/
void
btk_text_buffer_place_cursor (BtkTextBuffer     *buffer,
                              const BtkTextIter *where)
{
  btk_text_buffer_select_range (buffer, where, where);
}

/**
 * btk_text_buffer_select_range:
 * @buffer: a #BtkTextBuffer
 * @ins: where to put the "insert" mark
 * @bound: where to put the "selection_bound" mark
 *
 * This function moves the "insert" and "selection_bound" marks
 * simultaneously.  If you move them in two steps
 * with btk_text_buffer_move_mark(), you will temporarily select a
 * rebunnyion in between their old and new locations, which can be pretty
 * inefficient since the temporarily-selected rebunnyion will force stuff
 * to be recalculated. This function moves them as a unit, which can
 * be optimized.
 *
 * Since: 2.4
 **/
void
btk_text_buffer_select_range (BtkTextBuffer     *buffer,
			      const BtkTextIter *ins,
                              const BtkTextIter *bound)
{
  BtkTextIter real_ins;
  BtkTextIter real_bound;

  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));

  real_ins = *ins;
  real_bound = *bound;

  _btk_text_btree_select_range (get_btree (buffer), &real_ins, &real_bound);
  btk_text_buffer_mark_set (buffer, &real_ins,
                            btk_text_buffer_get_insert (buffer));
  btk_text_buffer_mark_set (buffer, &real_bound,
                            btk_text_buffer_get_selection_bound (buffer));
}

/*
 * Tags
 */

/**
 * btk_text_buffer_create_tag:
 * @buffer: a #BtkTextBuffer
 * @tag_name: (allow-none): name of the new tag, or %NULL
 * @first_property_name: (allow-none): name of first property to set, or %NULL
 * @Varargs: %NULL-terminated list of property names and values
 *
 * Creates a tag and adds it to the tag table for @buffer.
 * Equivalent to calling btk_text_tag_new() and then adding the
 * tag to the buffer's tag table. The returned tag is owned by
 * the buffer's tag table, so the ref count will be equal to one.
 *
 * If @tag_name is %NULL, the tag is anonymous.
 *
 * If @tag_name is non-%NULL, a tag called @tag_name must not already
 * exist in the tag table for this buffer.
 *
 * The @first_property_name argument and subsequent arguments are a list
 * of properties to set on the tag, as with g_object_set().
 *
 * Return value: (transfer none): a new tag
 */
BtkTextTag*
btk_text_buffer_create_tag (BtkTextBuffer *buffer,
                            const bchar   *tag_name,
                            const bchar   *first_property_name,
                            ...)
{
  BtkTextTag *tag;
  va_list list;
  
  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), NULL);

  tag = btk_text_tag_new (tag_name);

  btk_text_tag_table_add (get_table (buffer), tag);

  if (first_property_name)
    {
      va_start (list, first_property_name);
      g_object_set_valist (B_OBJECT (tag), first_property_name, list);
      va_end (list);
    }
  
  g_object_unref (tag);

  return tag;
}

static void
btk_text_buffer_real_apply_tag (BtkTextBuffer     *buffer,
                                BtkTextTag        *tag,
                                const BtkTextIter *start,
                                const BtkTextIter *end)
{
  if (tag->table != buffer->tag_table)
    {
      g_warning ("Can only apply tags that are in the tag table for the buffer");
      return;
    }
  
  _btk_text_btree_tag (start, end, tag, TRUE);
}

static void
btk_text_buffer_real_remove_tag (BtkTextBuffer     *buffer,
                                 BtkTextTag        *tag,
                                 const BtkTextIter *start,
                                 const BtkTextIter *end)
{
  if (tag->table != buffer->tag_table)
    {
      g_warning ("Can only remove tags that are in the tag table for the buffer");
      return;
    }
  
  _btk_text_btree_tag (start, end, tag, FALSE);
}

static void
btk_text_buffer_real_changed (BtkTextBuffer *buffer)
{
  btk_text_buffer_set_modified (buffer, TRUE);
}

static void
btk_text_buffer_real_mark_set (BtkTextBuffer     *buffer,
                               const BtkTextIter *iter,
                               BtkTextMark       *mark)
{
  BtkTextMark *insert;
  
  insert = btk_text_buffer_get_insert (buffer);

  if (mark == insert || mark == btk_text_buffer_get_selection_bound (buffer))
    {
      bboolean has_selection;

      update_selection_clipboards (buffer);
    
      has_selection = btk_text_buffer_get_selection_bounds (buffer,
                                                            NULL,
                                                            NULL);

      if (has_selection != buffer->has_selection)
        {
          buffer->has_selection = has_selection;
          g_object_notify (B_OBJECT (buffer), "has-selection");
        }
    }
    
    if (mark == insert)
      g_object_notify (B_OBJECT (buffer), "cursor-position");
}

static void
btk_text_buffer_emit_tag (BtkTextBuffer     *buffer,
                          BtkTextTag        *tag,
                          bboolean           apply,
                          const BtkTextIter *start,
                          const BtkTextIter *end)
{
  BtkTextIter start_tmp = *start;
  BtkTextIter end_tmp = *end;

  g_return_if_fail (tag != NULL);

  btk_text_iter_order (&start_tmp, &end_tmp);

  if (apply)
    g_signal_emit (buffer, signals[APPLY_TAG],
                   0,
                   tag, &start_tmp, &end_tmp);
  else
    g_signal_emit (buffer, signals[REMOVE_TAG],
                   0,
                   tag, &start_tmp, &end_tmp);
}

/**
 * btk_text_buffer_apply_tag:
 * @buffer: a #BtkTextBuffer
 * @tag: a #BtkTextTag
 * @start: one bound of range to be tagged
 * @end: other bound of range to be tagged
 *
 * Emits the "apply-tag" signal on @buffer. The default
 * handler for the signal applies @tag to the given range.
 * @start and @end do not have to be in order.
 **/
void
btk_text_buffer_apply_tag (BtkTextBuffer     *buffer,
                           BtkTextTag        *tag,
                           const BtkTextIter *start,
                           const BtkTextIter *end)
{
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (BTK_IS_TEXT_TAG (tag));
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (btk_text_iter_get_buffer (start) == buffer);
  g_return_if_fail (btk_text_iter_get_buffer (end) == buffer);
  g_return_if_fail (tag->table == buffer->tag_table);
  
  btk_text_buffer_emit_tag (buffer, tag, TRUE, start, end);
}

/**
 * btk_text_buffer_remove_tag:
 * @buffer: a #BtkTextBuffer
 * @tag: a #BtkTextTag
 * @start: one bound of range to be untagged
 * @end: other bound of range to be untagged
 *
 * Emits the "remove-tag" signal. The default handler for the signal
 * removes all occurrences of @tag from the given range. @start and
 * @end don't have to be in order.
 **/
void
btk_text_buffer_remove_tag (BtkTextBuffer     *buffer,
                            BtkTextTag        *tag,
                            const BtkTextIter *start,
                            const BtkTextIter *end)

{
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (BTK_IS_TEXT_TAG (tag));
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (btk_text_iter_get_buffer (start) == buffer);
  g_return_if_fail (btk_text_iter_get_buffer (end) == buffer);
  g_return_if_fail (tag->table == buffer->tag_table);
  
  btk_text_buffer_emit_tag (buffer, tag, FALSE, start, end);
}

/**
 * btk_text_buffer_apply_tag_by_name:
 * @buffer: a #BtkTextBuffer
 * @name: name of a named #BtkTextTag
 * @start: one bound of range to be tagged
 * @end: other bound of range to be tagged
 *
 * Calls btk_text_tag_table_lookup() on the buffer's tag table to
 * get a #BtkTextTag, then calls btk_text_buffer_apply_tag().
 **/
void
btk_text_buffer_apply_tag_by_name (BtkTextBuffer     *buffer,
                                   const bchar       *name,
                                   const BtkTextIter *start,
                                   const BtkTextIter *end)
{
  BtkTextTag *tag;

  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (name != NULL);
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (btk_text_iter_get_buffer (start) == buffer);
  g_return_if_fail (btk_text_iter_get_buffer (end) == buffer);

  tag = btk_text_tag_table_lookup (get_table (buffer),
                                   name);

  if (tag == NULL)
    {
      g_warning ("Unknown tag `%s'", name);
      return;
    }

  btk_text_buffer_emit_tag (buffer, tag, TRUE, start, end);
}

/**
 * btk_text_buffer_remove_tag_by_name:
 * @buffer: a #BtkTextBuffer
 * @name: name of a #BtkTextTag
 * @start: one bound of range to be untagged
 * @end: other bound of range to be untagged
 *
 * Calls btk_text_tag_table_lookup() on the buffer's tag table to
 * get a #BtkTextTag, then calls btk_text_buffer_remove_tag().
 **/
void
btk_text_buffer_remove_tag_by_name (BtkTextBuffer     *buffer,
                                    const bchar       *name,
                                    const BtkTextIter *start,
                                    const BtkTextIter *end)
{
  BtkTextTag *tag;

  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (name != NULL);
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (btk_text_iter_get_buffer (start) == buffer);
  g_return_if_fail (btk_text_iter_get_buffer (end) == buffer);
  
  tag = btk_text_tag_table_lookup (get_table (buffer),
                                   name);

  if (tag == NULL)
    {
      g_warning ("Unknown tag `%s'", name);
      return;
    }

  btk_text_buffer_emit_tag (buffer, tag, FALSE, start, end);
}

static bint
pointer_cmp (gconstpointer a,
             gconstpointer b)
{
  if (a < b)
    return -1;
  else if (a > b)
    return 1;
  else
    return 0;
}

/**
 * btk_text_buffer_remove_all_tags:
 * @buffer: a #BtkTextBuffer
 * @start: one bound of range to be untagged
 * @end: other bound of range to be untagged
 * 
 * Removes all tags in the range between @start and @end.  Be careful
 * with this function; it could remove tags added in code unrelated to
 * the code you're currently writing. That is, using this function is
 * probably a bad idea if you have two or more unrelated code sections
 * that add tags.
 **/
void
btk_text_buffer_remove_all_tags (BtkTextBuffer     *buffer,
                                 const BtkTextIter *start,
                                 const BtkTextIter *end)
{
  BtkTextIter first, second, tmp;
  GSList *tags;
  GSList *tmp_list;
  GSList *prev, *next;
  BtkTextTag *tag;
  
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (btk_text_iter_get_buffer (start) == buffer);
  g_return_if_fail (btk_text_iter_get_buffer (end) == buffer);
  
  first = *start;
  second = *end;

  btk_text_iter_order (&first, &second);

  /* Get all tags turned on at the start */
  tags = btk_text_iter_get_tags (&first);
  
  /* Find any that are toggled on within the range */
  tmp = first;
  while (btk_text_iter_forward_to_tag_toggle (&tmp, NULL))
    {
      GSList *toggled;
      GSList *tmp_list2;

      if (btk_text_iter_compare (&tmp, &second) >= 0)
        break; /* past the end of the range */
      
      toggled = btk_text_iter_get_toggled_tags (&tmp, TRUE);

      /* We could end up with a really big-ass list here.
       * Fix it someday.
       */
      tmp_list2 = toggled;
      while (tmp_list2 != NULL)
        {
          tags = b_slist_prepend (tags, tmp_list2->data);

          tmp_list2 = b_slist_next (tmp_list2);
        }

      b_slist_free (toggled);
    }
  
  /* Sort the list */
  tags = b_slist_sort (tags, pointer_cmp);

  /* Strip duplicates */
  tag = NULL;
  prev = NULL;
  tmp_list = tags;
  while (tmp_list != NULL)
    {
      if (tag == tmp_list->data)
        {
          /* duplicate */
          next = tmp_list->next;
          if (prev)
            prev->next = next;

          tmp_list->next = NULL;

          b_slist_free (tmp_list);

          tmp_list = next;
          /* prev is unchanged */
        }
      else
        {
          /* not a duplicate */
          tag = BTK_TEXT_TAG (tmp_list->data);
          prev = tmp_list;
          tmp_list = tmp_list->next;
        }
    }

  b_slist_foreach (tags, (GFunc) g_object_ref, NULL);
  
  tmp_list = tags;
  while (tmp_list != NULL)
    {
      tag = BTK_TEXT_TAG (tmp_list->data);

      btk_text_buffer_remove_tag (buffer, tag, &first, &second);
      
      tmp_list = tmp_list->next;
    }

  b_slist_foreach (tags, (GFunc) g_object_unref, NULL);
  
  b_slist_free (tags);
}


/*
 * Obtain various iterators
 */

/**
 * btk_text_buffer_get_iter_at_line_offset:
 * @buffer: a #BtkTextBuffer
 * @iter: (out): iterator to initialize
 * @line_number: line number counting from 0
 * @char_offset: char offset from start of line
 *
 * Obtains an iterator pointing to @char_offset within the given
 * line. The @char_offset must exist, offsets off the end of the line
 * are not allowed. Note <emphasis>characters</emphasis>, not bytes;
 * UTF-8 may encode one character as multiple bytes.
 **/
void
btk_text_buffer_get_iter_at_line_offset (BtkTextBuffer *buffer,
                                         BtkTextIter   *iter,
                                         bint           line_number,
                                         bint           char_offset)
{
  g_return_if_fail (iter != NULL);
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));

  _btk_text_btree_get_iter_at_line_char (get_btree (buffer),
                                         iter, line_number, char_offset);
}

/**
 * btk_text_buffer_get_iter_at_line_index:
 * @buffer: a #BtkTextBuffer 
 * @iter: (out): iterator to initialize 
 * @line_number: line number counting from 0
 * @byte_index: byte index from start of line
 *
 * Obtains an iterator pointing to @byte_index within the given line.
 * @byte_index must be the start of a UTF-8 character, and must not be
 * beyond the end of the line.  Note <emphasis>bytes</emphasis>, not
 * characters; UTF-8 may encode one character as multiple bytes.
 **/
void
btk_text_buffer_get_iter_at_line_index  (BtkTextBuffer *buffer,
                                         BtkTextIter   *iter,
                                         bint           line_number,
                                         bint           byte_index)
{
  g_return_if_fail (iter != NULL);
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));

  _btk_text_btree_get_iter_at_line_byte (get_btree (buffer),
                                         iter, line_number, byte_index);
}

/**
 * btk_text_buffer_get_iter_at_line:
 * @buffer: a #BtkTextBuffer 
 * @iter: (out): iterator to initialize
 * @line_number: line number counting from 0
 * 
 * Initializes @iter to the start of the given line.
 **/
void
btk_text_buffer_get_iter_at_line (BtkTextBuffer *buffer,
                                  BtkTextIter   *iter,
                                  bint           line_number)
{
  g_return_if_fail (iter != NULL);
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));

  btk_text_buffer_get_iter_at_line_offset (buffer, iter, line_number, 0);
}

/**
 * btk_text_buffer_get_iter_at_offset:
 * @buffer: a #BtkTextBuffer 
 * @iter: (out): iterator to initialize
 * @char_offset: char offset from start of buffer, counting from 0, or -1
 *
 * Initializes @iter to a position @char_offset chars from the start
 * of the entire buffer. If @char_offset is -1 or greater than the number
 * of characters in the buffer, @iter is initialized to the end iterator,
 * the iterator one past the last valid character in the buffer.
 **/
void
btk_text_buffer_get_iter_at_offset (BtkTextBuffer *buffer,
                                    BtkTextIter   *iter,
                                    bint           char_offset)
{
  g_return_if_fail (iter != NULL);
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));

  _btk_text_btree_get_iter_at_char (get_btree (buffer), iter, char_offset);
}

/**
 * btk_text_buffer_get_start_iter:
 * @buffer: a #BtkTextBuffer
 * @iter: (out): iterator to initialize
 *
 * Initialized @iter with the first position in the text buffer. This
 * is the same as using btk_text_buffer_get_iter_at_offset() to get
 * the iter at character offset 0.
 **/
void
btk_text_buffer_get_start_iter (BtkTextBuffer *buffer,
                                BtkTextIter   *iter)
{
  g_return_if_fail (iter != NULL);
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));

  _btk_text_btree_get_iter_at_char (get_btree (buffer), iter, 0);
}

/**
 * btk_text_buffer_get_end_iter:
 * @buffer: a #BtkTextBuffer 
 * @iter: (out): iterator to initialize
 *
 * Initializes @iter with the "end iterator," one past the last valid
 * character in the text buffer. If dereferenced with
 * btk_text_iter_get_char(), the end iterator has a character value of
 * 0. The entire buffer lies in the range from the first position in
 * the buffer (call btk_text_buffer_get_start_iter() to get
 * character position 0) to the end iterator.
 **/
void
btk_text_buffer_get_end_iter (BtkTextBuffer *buffer,
                              BtkTextIter   *iter)
{
  g_return_if_fail (iter != NULL);
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));

  _btk_text_btree_get_end_iter (get_btree (buffer), iter);
}

/**
 * btk_text_buffer_get_bounds:
 * @buffer: a #BtkTextBuffer 
 * @start: (out): iterator to initialize with first position in the buffer
 * @end: (out): iterator to initialize with the end iterator
 *
 * Retrieves the first and last iterators in the buffer, i.e. the
 * entire buffer lies within the range [@start,@end).
 **/
void
btk_text_buffer_get_bounds (BtkTextBuffer *buffer,
                            BtkTextIter   *start,
                            BtkTextIter   *end)
{
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));

  _btk_text_btree_get_iter_at_char (get_btree (buffer), start, 0);
  _btk_text_btree_get_end_iter (get_btree (buffer), end);
}

/*
 * Modified flag
 */

/**
 * btk_text_buffer_get_modified:
 * @buffer: a #BtkTextBuffer 
 * 
 * Indicates whether the buffer has been modified since the last call
 * to btk_text_buffer_set_modified() set the modification flag to
 * %FALSE. Used for example to enable a "save" function in a text
 * editor.
 * 
 * Return value: %TRUE if the buffer has been modified
 **/
bboolean
btk_text_buffer_get_modified (BtkTextBuffer *buffer)
{
  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), FALSE);

  return buffer->modified;
}

/**
 * btk_text_buffer_set_modified:
 * @buffer: a #BtkTextBuffer 
 * @setting: modification flag setting
 *
 * Used to keep track of whether the buffer has been modified since the
 * last time it was saved. Whenever the buffer is saved to disk, call
 * btk_text_buffer_set_modified (@buffer, FALSE). When the buffer is modified,
 * it will automatically toggled on the modified bit again. When the modified
 * bit flips, the buffer emits a "modified-changed" signal.
 **/
void
btk_text_buffer_set_modified (BtkTextBuffer *buffer,
                              bboolean       setting)
{
  bboolean fixed_setting;

  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));

  fixed_setting = setting != FALSE;

  if (buffer->modified == fixed_setting)
    return;
  else
    {
      buffer->modified = fixed_setting;
      g_signal_emit (buffer, signals[MODIFIED_CHANGED], 0);
    }
}

/**
 * btk_text_buffer_get_has_selection:
 * @buffer: a #BtkTextBuffer 
 * 
 * Indicates whether the buffer has some text currently selected.
 * 
 * Return value: %TRUE if the there is text selected
 *
 * Since: 2.10
 **/
bboolean
btk_text_buffer_get_has_selection (BtkTextBuffer *buffer)
{
  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), FALSE);

  return buffer->has_selection;
}


/*
 * Assorted other stuff
 */

/**
 * btk_text_buffer_get_line_count:
 * @buffer: a #BtkTextBuffer 
 * 
 * Obtains the number of lines in the buffer. This value is cached, so
 * the function is very fast.
 * 
 * Return value: number of lines in the buffer
 **/
bint
btk_text_buffer_get_line_count (BtkTextBuffer *buffer)
{
  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), 0);

  return _btk_text_btree_line_count (get_btree (buffer));
}

/**
 * btk_text_buffer_get_char_count:
 * @buffer: a #BtkTextBuffer 
 * 
 * Gets the number of characters in the buffer; note that characters
 * and bytes are not the same, you can't e.g. expect the contents of
 * the buffer in string form to be this many bytes long. The character
 * count is cached, so this function is very fast.
 * 
 * Return value: number of characters in the buffer
 **/
bint
btk_text_buffer_get_char_count (BtkTextBuffer *buffer)
{
  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), 0);

  return _btk_text_btree_char_count (get_btree (buffer));
}

/* Called when we lose the primary selection.
 */
static void
clipboard_clear_selection_cb (BtkClipboard *clipboard,
                              bpointer      data)
{
  /* Move selection_bound to the insertion point */
  BtkTextIter insert;
  BtkTextIter selection_bound;
  BtkTextBuffer *buffer = BTK_TEXT_BUFFER (data);

  btk_text_buffer_get_iter_at_mark (buffer, &insert,
                                    btk_text_buffer_get_insert (buffer));
  btk_text_buffer_get_iter_at_mark (buffer, &selection_bound,
                                    btk_text_buffer_get_selection_bound (buffer));

  if (!btk_text_iter_equal (&insert, &selection_bound))
    btk_text_buffer_move_mark (buffer,
                               btk_text_buffer_get_selection_bound (buffer),
                               &insert);
}

/* Called when we have the primary selection and someone else wants our
 * data in order to paste it.
 */
static void
clipboard_get_selection_cb (BtkClipboard     *clipboard,
                            BtkSelectionData *selection_data,
                            buint             info,
                            bpointer          data)
{
  BtkTextBuffer *buffer = BTK_TEXT_BUFFER (data);
  BtkTextIter start, end;

  if (btk_text_buffer_get_selection_bounds (buffer, &start, &end))
    {
      if (info == BTK_TEXT_BUFFER_TARGET_INFO_BUFFER_CONTENTS)
        {
          /* Provide the address of the buffer; this will only be
           * used within-process
           */
          btk_selection_data_set (selection_data,
                                  selection_data->target,
                                  8, /* bytes */
                                  (void*)&buffer,
                                  sizeof (buffer));
        }
      else if (info == BTK_TEXT_BUFFER_TARGET_INFO_RICH_TEXT)
        {
          buint8 *str;
          bsize   len;

          str = btk_text_buffer_serialize (buffer, buffer,
                                           selection_data->target,
                                           &start, &end, &len);

          btk_selection_data_set (selection_data,
                                  selection_data->target,
                                  8, /* bytes */
                                  str, len);
          g_free (str);
        }
      else
        {
          bchar *str;

          str = btk_text_iter_get_visible_text (&start, &end);
          btk_selection_data_set_text (selection_data, str, -1);
          g_free (str);
        }
    }
}

static BtkTextBuffer *
create_clipboard_contents_buffer (BtkTextBuffer *buffer)
{
  BtkTextBuffer *contents;

  contents = btk_text_buffer_new (btk_text_buffer_get_tag_table (buffer));

  g_object_set_data (B_OBJECT (contents), I_("btk-text-buffer-clipboard-source"),
                     buffer);
  g_object_set_data (B_OBJECT (contents), I_("btk-text-buffer-clipboard"),
                     BINT_TO_POINTER (1));

  /*  Ref the source buffer as long as the clipboard contents buffer
   *  exists, because it's needed for serializing the contents buffer.
   *  See http://bugzilla.bunny.org/show_bug.cgi?id=339195
   */
  g_object_ref (buffer);
  g_object_weak_ref (B_OBJECT (contents), (GWeakNotify) g_object_unref, buffer);

  return contents;
}

/* Provide cut/copied data */
static void
clipboard_get_contents_cb (BtkClipboard     *clipboard,
                           BtkSelectionData *selection_data,
                           buint             info,
                           bpointer          data)
{
  BtkTextBuffer *contents = BTK_TEXT_BUFFER (data);

  g_assert (contents); /* This should never be called unless we own the clipboard */

  if (info == BTK_TEXT_BUFFER_TARGET_INFO_BUFFER_CONTENTS)
    {
      /* Provide the address of the clipboard buffer; this will only
       * be used within-process. OK to supply a NULL value for contents.
       */
      btk_selection_data_set (selection_data,
                              selection_data->target,
                              8, /* bytes */
                              (void*)&contents,
                              sizeof (contents));
    }
  else if (info == BTK_TEXT_BUFFER_TARGET_INFO_RICH_TEXT)
    {
      BtkTextBuffer *clipboard_source_buffer;
      BtkTextIter start, end;
      buint8 *str;
      bsize   len;

      clipboard_source_buffer = g_object_get_data (B_OBJECT (contents),
                                                   "btk-text-buffer-clipboard-source");

      btk_text_buffer_get_bounds (contents, &start, &end);

      str = btk_text_buffer_serialize (clipboard_source_buffer, contents,
                                       selection_data->target,
                                       &start, &end, &len);

      btk_selection_data_set (selection_data,
			      selection_data->target,
			      8, /* bytes */
			      str, len);
      g_free (str);
    }
  else
    {
      bchar *str;
      BtkTextIter start, end;

      btk_text_buffer_get_bounds (contents, &start, &end);

      str = btk_text_iter_get_visible_text (&start, &end);
      btk_selection_data_set_text (selection_data, str, -1);
      g_free (str);
    }
}

static void
clipboard_clear_contents_cb (BtkClipboard *clipboard,
                             bpointer      data)
{
  BtkTextBuffer *contents = BTK_TEXT_BUFFER (data);

  g_object_unref (contents);
}

static void
get_paste_point (BtkTextBuffer *buffer,
                 BtkTextIter   *iter,
                 bboolean       clear_afterward)
{
  BtkTextIter insert_point;
  BtkTextMark *paste_point_override;

  paste_point_override = btk_text_buffer_get_mark (buffer,
                                                   "btk_paste_point_override");

  if (paste_point_override != NULL)
    {
      btk_text_buffer_get_iter_at_mark (buffer, &insert_point,
                                        paste_point_override);
      if (clear_afterward)
        btk_text_buffer_delete_mark (buffer, paste_point_override);
    }
  else
    {
      btk_text_buffer_get_iter_at_mark (buffer, &insert_point,
                                        btk_text_buffer_get_insert (buffer));
    }

  *iter = insert_point;
}

static void
pre_paste_prep (ClipboardRequest *request_data,
                BtkTextIter      *insert_point)
{
  BtkTextBuffer *buffer = request_data->buffer;
  
  get_paste_point (buffer, insert_point, TRUE);

  /* If we're going to replace the selection, we insert before it to
   * avoid messing it up, then we delete the selection after inserting.
   */
  if (request_data->replace_selection)
    {
      BtkTextIter start, end;
      
      if (btk_text_buffer_get_selection_bounds (buffer, &start, &end))
        *insert_point = start;
    }
}

static void
post_paste_cleanup (ClipboardRequest *request_data)
{
  if (request_data->replace_selection)
    {
      BtkTextIter start, end;
      
      if (btk_text_buffer_get_selection_bounds (request_data->buffer,
                                                &start, &end))
        {
          if (request_data->interactive)
            btk_text_buffer_delete_interactive (request_data->buffer,
                                                &start,
                                                &end,
                                                request_data->default_editable);
          else
            btk_text_buffer_delete (request_data->buffer, &start, &end);
        }
    }
}

static void
emit_paste_done (BtkTextBuffer *buffer,
                 BtkClipboard  *clipboard)
{
  g_signal_emit (buffer, signals[PASTE_DONE], 0, clipboard);
}

static void
free_clipboard_request (ClipboardRequest *request_data)
{
  g_object_unref (request_data->buffer);
  g_free (request_data);
}

/* Called when we request a paste and receive the text data
 */
static void
clipboard_text_received (BtkClipboard *clipboard,
                         const bchar  *str,
                         bpointer      data)
{
  ClipboardRequest *request_data = data;
  BtkTextBuffer *buffer = request_data->buffer;

  if (str)
    {
      BtkTextIter insert_point;
      
      if (request_data->interactive) 
	btk_text_buffer_begin_user_action (buffer);

      pre_paste_prep (request_data, &insert_point);
      
      if (request_data->interactive) 
	btk_text_buffer_insert_interactive (buffer, &insert_point,
					    str, -1, request_data->default_editable);
      else
        btk_text_buffer_insert (buffer, &insert_point,
                                str, -1);

      post_paste_cleanup (request_data);
      
      if (request_data->interactive) 
	btk_text_buffer_end_user_action (buffer);

      emit_paste_done (buffer, clipboard);
    }
  else
    {
      /* It may happen that we set a point override but we are not inserting
         any text, so we must remove it afterwards */
      BtkTextMark *paste_point_override;

      paste_point_override = btk_text_buffer_get_mark (buffer,
                                                       "btk_paste_point_override");

      if (paste_point_override != NULL)
        btk_text_buffer_delete_mark (buffer, paste_point_override);
    }

  free_clipboard_request (request_data);
}

static BtkTextBuffer*
selection_data_get_buffer (BtkSelectionData *selection_data,
                           ClipboardRequest *request_data)
{
  BdkWindow *owner;
  BtkTextBuffer *src_buffer = NULL;

  /* If we can get the owner, the selection is in-process */
  owner = bdk_selection_owner_get_for_display (selection_data->display,
					       selection_data->selection);

  if (owner == NULL)
    return NULL;
  
  if (bdk_window_get_window_type (owner) == BDK_WINDOW_FOREIGN)
    return NULL;
 
  if (selection_data->type !=
      bdk_atom_intern_static_string ("BTK_TEXT_BUFFER_CONTENTS"))
    return NULL;

  if (selection_data->length != sizeof (src_buffer))
    return NULL;
          
  memcpy (&src_buffer, selection_data->data, sizeof (src_buffer));

  if (src_buffer == NULL)
    return NULL;
  
  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (src_buffer), NULL);

  if (btk_text_buffer_get_tag_table (src_buffer) !=
      btk_text_buffer_get_tag_table (request_data->buffer))
    return NULL;
  
  return src_buffer;
}

#if 0
/* These are pretty handy functions; maybe something like them
 * should be in the public API. Also, there are other places in this
 * file where they could be used.
 */
static bpointer
save_iter (const BtkTextIter *iter,
           bboolean           left_gravity)
{
  return btk_text_buffer_create_mark (btk_text_iter_get_buffer (iter),
                                      NULL,
                                      iter,
                                      TRUE);
}

static void
restore_iter (const BtkTextIter *iter,
              bpointer           save_id)
{
  btk_text_buffer_get_iter_at_mark (btk_text_mark_get_buffer (save_id),
                                    (BtkTextIter*) iter,
                                    save_id);
  btk_text_buffer_delete_mark (btk_text_mark_get_buffer (save_id),
                               save_id);
}
#endif

static void
clipboard_rich_text_received (BtkClipboard *clipboard,
                              BdkAtom       format,
                              const buint8 *text,
                              bsize         length,
                              bpointer      data)
{
  ClipboardRequest *request_data = data;
  BtkTextIter insert_point;
  bboolean retval = TRUE;
  GError *error = NULL;

  if (text != NULL && length > 0)
    {
      pre_paste_prep (request_data, &insert_point);

      if (request_data->interactive)
        btk_text_buffer_begin_user_action (request_data->buffer);

      if (!request_data->interactive ||
          btk_text_iter_can_insert (&insert_point,
                                    request_data->default_editable))
        {
          retval = btk_text_buffer_deserialize (request_data->buffer,
                                                request_data->buffer,
                                                format,
                                                &insert_point,
                                                text, length,
                                                &error);
        }

      if (!retval)
        {
          g_warning ("error pasting: %s\n", error->message);
          g_clear_error (&error);
        }

      if (request_data->interactive)
        btk_text_buffer_end_user_action (request_data->buffer);

      emit_paste_done (request_data->buffer, clipboard);

      if (retval)
        {
          post_paste_cleanup (request_data);
          return;
        }
    }

  /* Request the text selection instead */
  btk_clipboard_request_text (clipboard,
                              clipboard_text_received,
                              data);
}

static void
paste_from_buffer (BtkClipboard      *clipboard,
                   ClipboardRequest  *request_data,
                   BtkTextBuffer     *src_buffer,
                   const BtkTextIter *start,
                   const BtkTextIter *end)
{
  BtkTextIter insert_point;
  BtkTextBuffer *buffer = request_data->buffer;
  
  /* We're about to emit a bunch of signals, so be safe */
  g_object_ref (src_buffer);
  
  pre_paste_prep (request_data, &insert_point);
  
  if (request_data->interactive) 
    btk_text_buffer_begin_user_action (buffer);

  if (!btk_text_iter_equal (start, end))
    {
      if (!request_data->interactive ||
          (btk_text_iter_can_insert (&insert_point,
                                     request_data->default_editable)))
        btk_text_buffer_real_insert_range (buffer,
                                           &insert_point,
                                           start,
                                           end,
                                           request_data->interactive);
    }

  post_paste_cleanup (request_data);
      
  if (request_data->interactive) 
    btk_text_buffer_end_user_action (buffer);

  emit_paste_done (buffer, clipboard);

  g_object_unref (src_buffer);

  free_clipboard_request (request_data);
}

static void
clipboard_clipboard_buffer_received (BtkClipboard     *clipboard,
                                     BtkSelectionData *selection_data,
                                     bpointer          data)
{
  ClipboardRequest *request_data = data;
  BtkTextBuffer *src_buffer;

  src_buffer = selection_data_get_buffer (selection_data, request_data); 

  if (src_buffer)
    {
      BtkTextIter start, end;

      if (g_object_get_data (B_OBJECT (src_buffer), "btk-text-buffer-clipboard"))
	{
	  btk_text_buffer_get_bounds (src_buffer, &start, &end);

	  paste_from_buffer (clipboard, request_data, src_buffer,
			     &start, &end);
	}
      else
	{
	  if (btk_text_buffer_get_selection_bounds (src_buffer, &start, &end))
	    paste_from_buffer (clipboard, request_data, src_buffer,
			       &start, &end);
	}
    }
  else
    {
      if (btk_clipboard_wait_is_rich_text_available (clipboard,
                                                     request_data->buffer))
        {
          /* Request rich text */
          btk_clipboard_request_rich_text (clipboard,
                                           request_data->buffer,
                                           clipboard_rich_text_received,
                                           data);
        }
      else
        {
          /* Request the text selection instead */
          btk_clipboard_request_text (clipboard,
                                      clipboard_text_received,
                                      data);
        }
    }
}

typedef struct
{
  BtkClipboard *clipboard;
  buint ref_count;
} SelectionClipboard;

static void
update_selection_clipboards (BtkTextBuffer *buffer)
{
  BtkTextBufferPrivate *priv;
  GSList               *tmp_list = buffer->selection_clipboards;

  priv = BTK_TEXT_BUFFER_GET_PRIVATE (buffer);

  btk_text_buffer_get_copy_target_list (buffer);

  while (tmp_list)
    {
      BtkTextIter start;
      BtkTextIter end;
      
      SelectionClipboard *selection_clipboard = tmp_list->data;
      BtkClipboard *clipboard = selection_clipboard->clipboard;

      /* Determine whether we have a selection and adjust X selection
       * accordingly.
       */
      if (!btk_text_buffer_get_selection_bounds (buffer, &start, &end))
	{
	  if (btk_clipboard_get_owner (clipboard) == B_OBJECT (buffer))
	    btk_clipboard_clear (clipboard);
	}
      else
	{
	  /* Even if we already have the selection, we need to update our
	   * timestamp.
	   */
	  if (!btk_clipboard_set_with_owner (clipboard,
                                             priv->copy_target_entries,
                                             priv->n_copy_target_entries,
					     clipboard_get_selection_cb,
					     clipboard_clear_selection_cb,
					     B_OBJECT (buffer)))
	    clipboard_clear_selection_cb (clipboard, buffer);
	}

      tmp_list = tmp_list->next;
    }
}

static SelectionClipboard *
find_selection_clipboard (BtkTextBuffer *buffer,
			  BtkClipboard  *clipboard)
{
  GSList *tmp_list = buffer->selection_clipboards;
  while (tmp_list)
    {
      SelectionClipboard *selection_clipboard = tmp_list->data;
      if (selection_clipboard->clipboard == clipboard)
	return selection_clipboard;
      
      tmp_list = tmp_list->next;
    }

  return NULL;
}

/**
 * btk_text_buffer_add_selection_clipboard:
 * @buffer: a #BtkTextBuffer
 * @clipboard: a #BtkClipboard
 * 
 * Adds @clipboard to the list of clipboards in which the selection 
 * contents of @buffer are available. In most cases, @clipboard will be 
 * the #BtkClipboard of type %BDK_SELECTION_PRIMARY for a view of @buffer.
 **/
void
btk_text_buffer_add_selection_clipboard (BtkTextBuffer *buffer,
					 BtkClipboard  *clipboard)
{
  SelectionClipboard *selection_clipboard;

  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (clipboard != NULL);

  selection_clipboard = find_selection_clipboard (buffer, clipboard);
  if (selection_clipboard)
    {
      selection_clipboard->ref_count++;
    }
  else
    {
      selection_clipboard = g_new (SelectionClipboard, 1);

      selection_clipboard->clipboard = clipboard;
      selection_clipboard->ref_count = 1;

      buffer->selection_clipboards = b_slist_prepend (buffer->selection_clipboards, selection_clipboard);
    }
}

/**
 * btk_text_buffer_remove_selection_clipboard:
 * @buffer: a #BtkTextBuffer
 * @clipboard: a #BtkClipboard added to @buffer by 
 *             btk_text_buffer_add_selection_clipboard()
 * 
 * Removes a #BtkClipboard added with 
 * btk_text_buffer_add_selection_clipboard().
 **/
void 
btk_text_buffer_remove_selection_clipboard (BtkTextBuffer *buffer,
					    BtkClipboard  *clipboard)
{
  SelectionClipboard *selection_clipboard;

  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (clipboard != NULL);

  selection_clipboard = find_selection_clipboard (buffer, clipboard);
  g_return_if_fail (selection_clipboard != NULL);

  selection_clipboard->ref_count--;
  if (selection_clipboard->ref_count == 0)
    {
      if (btk_clipboard_get_owner (selection_clipboard->clipboard) == B_OBJECT (buffer))
	btk_clipboard_clear (selection_clipboard->clipboard);

      buffer->selection_clipboards = b_slist_remove (buffer->selection_clipboards,
                                                     selection_clipboard);
      
      g_free (selection_clipboard);
    }
}

static void
remove_all_selection_clipboards (BtkTextBuffer *buffer)
{
  b_slist_foreach (buffer->selection_clipboards, (GFunc)g_free, NULL);
  b_slist_free (buffer->selection_clipboards);
  buffer->selection_clipboards = NULL;
}

/**
 * btk_text_buffer_paste_clipboard:
 * @buffer: a #BtkTextBuffer
 * @clipboard: the #BtkClipboard to paste from
 * @override_location: (allow-none): location to insert pasted text, or %NULL for
 *                     at the cursor
 * @default_editable: whether the buffer is editable by default
 *
 * Pastes the contents of a clipboard at the insertion point, or
 * at @override_location. (Note: pasting is asynchronous, that is,
 * we'll ask for the paste data and return, and at some point later
 * after the main loop runs, the paste data will be inserted.)
 **/
void
btk_text_buffer_paste_clipboard (BtkTextBuffer *buffer,
				 BtkClipboard  *clipboard,
				 BtkTextIter   *override_location,
                                 bboolean       default_editable)
{
  ClipboardRequest *data = g_new (ClipboardRequest, 1);
  BtkTextIter paste_point;
  BtkTextIter start, end;

  if (override_location != NULL)
    btk_text_buffer_create_mark (buffer,
                                 "btk_paste_point_override",
                                 override_location, FALSE);

  data->buffer = g_object_ref (buffer);
  data->interactive = TRUE;
  data->default_editable = default_editable;

  /* When pasting with the cursor inside the selection area, you
   * replace the selection with the new text, otherwise, you
   * simply insert the new text at the point where the click
   * occured, unselecting any selected text. The replace_selection
   * flag toggles this behavior.
   */
  data->replace_selection = FALSE;
  
  get_paste_point (buffer, &paste_point, FALSE);
  if (btk_text_buffer_get_selection_bounds (buffer, &start, &end) &&
      (btk_text_iter_in_range (&paste_point, &start, &end) ||
       btk_text_iter_equal (&paste_point, &end)))
    data->replace_selection = TRUE;

  btk_clipboard_request_contents (clipboard,
				  bdk_atom_intern_static_string ("BTK_TEXT_BUFFER_CONTENTS"),
				  clipboard_clipboard_buffer_received, data);
}

/**
 * btk_text_buffer_delete_selection:
 * @buffer: a #BtkTextBuffer 
 * @interactive: whether the deletion is caused by user interaction
 * @default_editable: whether the buffer is editable by default
 *
 * Deletes the range between the "insert" and "selection_bound" marks,
 * that is, the currently-selected text. If @interactive is %TRUE,
 * the editability of the selection will be considered (users can't delete
 * uneditable text).
 * 
 * Return value: whether there was a non-empty selection to delete
 **/
bboolean
btk_text_buffer_delete_selection (BtkTextBuffer *buffer,
                                  bboolean interactive,
                                  bboolean default_editable)
{
  BtkTextIter start;
  BtkTextIter end;

  if (!btk_text_buffer_get_selection_bounds (buffer, &start, &end))
    {
      return FALSE; /* No selection */
    }
  else
    {
      if (interactive)
        btk_text_buffer_delete_interactive (buffer, &start, &end, default_editable);
      else
        btk_text_buffer_delete (buffer, &start, &end);

      return TRUE; /* We deleted stuff */
    }
}

/**
 * btk_text_buffer_backspace:
 * @buffer: a #BtkTextBuffer
 * @iter: a position in @buffer
 * @interactive: whether the deletion is caused by user interaction
 * @default_editable: whether the buffer is editable by default
 * 
 * Performs the appropriate action as if the user hit the delete
 * key with the cursor at the position specified by @iter. In the
 * normal case a single character will be deleted, but when
 * combining accents are involved, more than one character can
 * be deleted, and when precomposed character and accent combinations
 * are involved, less than one character will be deleted.
 * 
 * Because the buffer is modified, all outstanding iterators become 
 * invalid after calling this function; however, the @iter will be
 * re-initialized to point to the location where text was deleted. 
 *
 * Return value: %TRUE if the buffer was modified
 *
 * Since: 2.6
 **/
bboolean
btk_text_buffer_backspace (BtkTextBuffer *buffer,
			   BtkTextIter   *iter,
			   bboolean       interactive,
			   bboolean       default_editable)
{
  bchar *cluster_text;
  BtkTextIter start;
  BtkTextIter end;
  bboolean retval = FALSE;
  const BangoLogAttr *attrs;
  int offset;
  bboolean backspace_deletes_character;

  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  start = *iter;
  end = *iter;

  attrs = _btk_text_buffer_get_line_log_attrs (buffer, &start, NULL);

  /* For no good reason, attrs is NULL for the empty last line in
   * a buffer. Special case that here. (#156164)
   */
  if (attrs)
    {
      offset = btk_text_iter_get_line_offset (&start);
      backspace_deletes_character = attrs[offset].backspace_deletes_character;
    }
  else
    backspace_deletes_character = FALSE;

  btk_text_iter_backward_cursor_position (&start);

  if (btk_text_iter_equal (&start, &end))
    return FALSE;
    
  cluster_text = btk_text_iter_get_text (&start, &end);

  if (interactive)
    btk_text_buffer_begin_user_action (buffer);
  
  if (btk_text_buffer_delete_interactive (buffer, &start, &end,
					  default_editable))
    {
      /* special case \r\n, since we never want to reinsert \r */
      if (backspace_deletes_character && strcmp ("\r\n", cluster_text))
	{
	  bchar *normalized_text = g_utf8_normalize (cluster_text,
						     strlen (cluster_text),
						     G_NORMALIZE_NFD);
	  blong len = g_utf8_strlen (normalized_text, -1);

	  if (len > 1)
	    btk_text_buffer_insert_interactive (buffer,
						&start,
						normalized_text,
						g_utf8_offset_to_pointer (normalized_text, len - 1) - normalized_text,
						default_editable);
	  
	  g_free (normalized_text);
	}

      retval = TRUE;
    }
  
  if (interactive)
    btk_text_buffer_end_user_action (buffer);
  
  g_free (cluster_text);

  /* Revalidate the users iter */
  *iter = start;

  return retval;
}

static void
cut_or_copy (BtkTextBuffer *buffer,
	     BtkClipboard  *clipboard,
             bboolean       delete_rebunnyion_after,
             bboolean       interactive,
             bboolean       default_editable)
{
  BtkTextBufferPrivate *priv;

  /* We prefer to cut the selected rebunnyion between selection_bound and
   * insertion point. If that rebunnyion is empty, then we cut the rebunnyion
   * between the "anchor" and the insertion point (this is for
   * C-space and M-w and other Emacs-style copy/yank keys). Note that
   * insert and selection_bound are guaranteed to exist, but the
   * anchor only exists sometimes.
   */
  BtkTextIter start;
  BtkTextIter end;

  priv = BTK_TEXT_BUFFER_GET_PRIVATE (buffer);

  btk_text_buffer_get_copy_target_list (buffer);

  if (!btk_text_buffer_get_selection_bounds (buffer, &start, &end))
    {
      /* Let's try the anchor thing */
      BtkTextMark * anchor = btk_text_buffer_get_mark (buffer, "anchor");

      if (anchor == NULL)
        return;
      else
        {
          btk_text_buffer_get_iter_at_mark (buffer, &end, anchor);
          btk_text_iter_order (&start, &end);
        }
    }

  if (!btk_text_iter_equal (&start, &end))
    {
      BtkTextIter ins;
      BtkTextBuffer *contents;

      contents = create_clipboard_contents_buffer (buffer);

      btk_text_buffer_get_iter_at_offset (contents, &ins, 0);
      
      btk_text_buffer_insert_range (contents, &ins, &start, &end);
                                    
      if (!btk_clipboard_set_with_data (clipboard,
                                        priv->copy_target_entries,
                                        priv->n_copy_target_entries,
					clipboard_get_contents_cb,
					clipboard_clear_contents_cb,
					contents))
	g_object_unref (contents);
      else
	btk_clipboard_set_can_store (clipboard,
                                     priv->copy_target_entries + 1,
                                     priv->n_copy_target_entries - 1);

      if (delete_rebunnyion_after)
        {
          if (interactive)
            btk_text_buffer_delete_interactive (buffer, &start, &end,
                                                default_editable);
          else
            btk_text_buffer_delete (buffer, &start, &end);
        }
    }
}

/**
 * btk_text_buffer_cut_clipboard:
 * @buffer: a #BtkTextBuffer
 * @clipboard: the #BtkClipboard object to cut to
 * @default_editable: default editability of the buffer
 *
 * Copies the currently-selected text to a clipboard, then deletes
 * said text if it's editable.
 **/
void
btk_text_buffer_cut_clipboard (BtkTextBuffer *buffer,
			       BtkClipboard  *clipboard,
                               bboolean       default_editable)
{
  btk_text_buffer_begin_user_action (buffer);
  cut_or_copy (buffer, clipboard, TRUE, TRUE, default_editable);
  btk_text_buffer_end_user_action (buffer);
}

/**
 * btk_text_buffer_copy_clipboard:
 * @buffer: a #BtkTextBuffer 
 * @clipboard: the #BtkClipboard object to copy to
 *
 * Copies the currently-selected text to a clipboard.
 **/
void
btk_text_buffer_copy_clipboard (BtkTextBuffer *buffer,
				BtkClipboard  *clipboard)
{
  cut_or_copy (buffer, clipboard, FALSE, TRUE, TRUE);
}

/**
 * btk_text_buffer_get_selection_bounds:
 * @buffer: a #BtkTextBuffer a #BtkTextBuffer
 * @start: (out): iterator to initialize with selection start
 * @end: (out): iterator to initialize with selection end
 *
 * Returns %TRUE if some text is selected; places the bounds
 * of the selection in @start and @end (if the selection has length 0,
 * then @start and @end are filled in with the same value).
 * @start and @end will be in ascending order. If @start and @end are
 * NULL, then they are not filled in, but the return value still indicates
 * whether text is selected.
 *
 * Return value: whether the selection has nonzero length
 **/
bboolean
btk_text_buffer_get_selection_bounds (BtkTextBuffer *buffer,
                                      BtkTextIter   *start,
                                      BtkTextIter   *end)
{
  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), FALSE);

  return _btk_text_btree_get_selection_bounds (get_btree (buffer), start, end);
}

/**
 * btk_text_buffer_begin_user_action:
 * @buffer: a #BtkTextBuffer
 * 
 * Called to indicate that the buffer operations between here and a
 * call to btk_text_buffer_end_user_action() are part of a single
 * user-visible operation. The operations between
 * btk_text_buffer_begin_user_action() and
 * btk_text_buffer_end_user_action() can then be grouped when creating
 * an undo stack. #BtkTextBuffer maintains a count of calls to
 * btk_text_buffer_begin_user_action() that have not been closed with
 * a call to btk_text_buffer_end_user_action(), and emits the 
 * "begin-user-action" and "end-user-action" signals only for the 
 * outermost pair of calls. This allows you to build user actions 
 * from other user actions.
 *
 * The "interactive" buffer mutation functions, such as
 * btk_text_buffer_insert_interactive(), automatically call begin/end
 * user action around the buffer operations they perform, so there's
 * no need to add extra calls if you user action consists solely of a
 * single call to one of those functions.
 **/
void
btk_text_buffer_begin_user_action (BtkTextBuffer *buffer)
{
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));

  buffer->user_action_count += 1;
  
  if (buffer->user_action_count == 1)
    {
      /* Outermost nested user action begin emits the signal */
      g_signal_emit (buffer, signals[BEGIN_USER_ACTION], 0);
    }
}

/**
 * btk_text_buffer_end_user_action:
 * @buffer: a #BtkTextBuffer
 * 
 * Should be paired with a call to btk_text_buffer_begin_user_action().
 * See that function for a full explanation.
 **/
void
btk_text_buffer_end_user_action (BtkTextBuffer *buffer)
{
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (buffer->user_action_count > 0);
  
  buffer->user_action_count -= 1;
  
  if (buffer->user_action_count == 0)
    {
      /* Ended the outermost-nested user action end, so emit the signal */
      g_signal_emit (buffer, signals[END_USER_ACTION], 0);
    }
}

static void
btk_text_buffer_free_target_lists (BtkTextBuffer *buffer)
{
  BtkTextBufferPrivate *priv = BTK_TEXT_BUFFER_GET_PRIVATE (buffer);

  if (priv->copy_target_list)
    {
      btk_target_list_unref (priv->copy_target_list);
      priv->copy_target_list = NULL;

      btk_target_table_free (priv->copy_target_entries,
                             priv->n_copy_target_entries);
      priv->copy_target_entries = NULL;
      priv->n_copy_target_entries = 0;
    }

  if (priv->paste_target_list)
    {
      btk_target_list_unref (priv->paste_target_list);
      priv->paste_target_list = NULL;

      btk_target_table_free (priv->paste_target_entries,
                             priv->n_paste_target_entries);
      priv->paste_target_entries = NULL;
      priv->n_paste_target_entries = 0;
    }
}

static BtkTargetList *
btk_text_buffer_get_target_list (BtkTextBuffer   *buffer,
                                 bboolean         deserializable,
                                 BtkTargetEntry **entries,
                                 bint            *n_entries)
{
  BtkTargetList *target_list;

  target_list = btk_target_list_new (NULL, 0);

  btk_target_list_add (target_list,
                       bdk_atom_intern_static_string ("BTK_TEXT_BUFFER_CONTENTS"),
                       BTK_TARGET_SAME_APP,
                       BTK_TEXT_BUFFER_TARGET_INFO_BUFFER_CONTENTS);

  btk_target_list_add_rich_text_targets (target_list,
                                         BTK_TEXT_BUFFER_TARGET_INFO_RICH_TEXT,
                                         deserializable,
                                         buffer);

  btk_target_list_add_text_targets (target_list,
                                    BTK_TEXT_BUFFER_TARGET_INFO_TEXT);

  *entries = btk_target_table_new_from_list (target_list, n_entries);

  return target_list;
}

/**
 * btk_text_buffer_get_copy_target_list:
 * @buffer: a #BtkTextBuffer
 *
 * This function returns the list of targets this text buffer can
 * provide for copying and as DND source. The targets in the list are
 * added with %info values from the #BtkTextBufferTargetInfo enum,
 * using btk_target_list_add_rich_text_targets() and
 * btk_target_list_add_text_targets().
 *
 * Return value: (transfer none): the #BtkTargetList
 *
 * Since: 2.10
 **/
BtkTargetList *
btk_text_buffer_get_copy_target_list (BtkTextBuffer *buffer)
{
  BtkTextBufferPrivate *priv;

  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), NULL);

  priv = BTK_TEXT_BUFFER_GET_PRIVATE (buffer);

  if (! priv->copy_target_list)
    priv->copy_target_list =
      btk_text_buffer_get_target_list (buffer, FALSE,
                                       &priv->copy_target_entries,
                                       &priv->n_copy_target_entries);

  return priv->copy_target_list;
}

/**
 * btk_text_buffer_get_paste_target_list:
 * @buffer: a #BtkTextBuffer
 *
 * This function returns the list of targets this text buffer supports
 * for pasting and as DND destination. The targets in the list are
 * added with %info values from the #BtkTextBufferTargetInfo enum,
 * using btk_target_list_add_rich_text_targets() and
 * btk_target_list_add_text_targets().
 *
 * Return value: (transfer none): the #BtkTargetList
 *
 * Since: 2.10
 **/
BtkTargetList *
btk_text_buffer_get_paste_target_list (BtkTextBuffer *buffer)
{
  BtkTextBufferPrivate *priv;

  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), NULL);

  priv = BTK_TEXT_BUFFER_GET_PRIVATE (buffer);

  if (! priv->paste_target_list)
    priv->paste_target_list =
      btk_text_buffer_get_target_list (buffer, TRUE,
                                       &priv->paste_target_entries,
                                       &priv->n_paste_target_entries);

  return priv->paste_target_list;
}

/*
 * Logical attribute cache
 */

#define ATTR_CACHE_SIZE 2

typedef struct _CacheEntry CacheEntry;
struct _CacheEntry
{
  bint line;
  bint char_len;
  BangoLogAttr *attrs;
};

struct _BtkTextLogAttrCache
{
  bint chars_changed_stamp;
  CacheEntry entries[ATTR_CACHE_SIZE];
};

static void
free_log_attr_cache (BtkTextLogAttrCache *cache)
{
  bint i = 0;
  while (i < ATTR_CACHE_SIZE)
    {
      g_free (cache->entries[i].attrs);
      ++i;
    }
  g_free (cache);
}

static void
clear_log_attr_cache (BtkTextLogAttrCache *cache)
{
  bint i = 0;
  while (i < ATTR_CACHE_SIZE)
    {
      g_free (cache->entries[i].attrs);
      cache->entries[i].attrs = NULL;
      ++i;
    }
}

static BangoLogAttr*
compute_log_attrs (const BtkTextIter *iter,
                   bint              *char_lenp)
{
  BtkTextIter start;
  BtkTextIter end;
  bchar *paragraph;
  bint char_len, byte_len;
  BangoLogAttr *attrs = NULL;
  
  start = *iter;
  end = *iter;

  btk_text_iter_set_line_offset (&start, 0);
  btk_text_iter_forward_line (&end);

  paragraph = btk_text_iter_get_slice (&start, &end);
  char_len = g_utf8_strlen (paragraph, -1);
  byte_len = strlen (paragraph);

  g_assert (char_len > 0);

  if (char_lenp)
    *char_lenp = char_len;
  
  attrs = g_new (BangoLogAttr, char_len + 1);

  /* FIXME we need to follow BangoLayout and allow different language
   * tags within the paragraph
   */
  bango_get_log_attrs (paragraph, byte_len, -1,
		       btk_text_iter_get_language (&start),
                       attrs,
                       char_len + 1);
  
  g_free (paragraph);

  return attrs;
}

/* The return value from this is valid until you call this a second time.
 */
const BangoLogAttr*
_btk_text_buffer_get_line_log_attrs (BtkTextBuffer     *buffer,
                                     const BtkTextIter *anywhere_in_line,
                                     bint              *char_len)
{
  bint line;
  BtkTextLogAttrCache *cache;
  bint i;
  
  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (anywhere_in_line != NULL, NULL);

  /* special-case for empty last line in buffer */
  if (btk_text_iter_is_end (anywhere_in_line) &&
      btk_text_iter_get_line_offset (anywhere_in_line) == 0)
    {
      if (char_len)
        *char_len = 0;
      return NULL;
    }
  
  /* FIXME we also need to recompute log attrs if the language tag at
   * the start of a paragraph changes
   */
  
  if (buffer->log_attr_cache == NULL)
    {
      buffer->log_attr_cache = g_new0 (BtkTextLogAttrCache, 1);
      buffer->log_attr_cache->chars_changed_stamp =
        _btk_text_btree_get_chars_changed_stamp (get_btree (buffer));
    }
  else if (buffer->log_attr_cache->chars_changed_stamp !=
           _btk_text_btree_get_chars_changed_stamp (get_btree (buffer)))
    {
      clear_log_attr_cache (buffer->log_attr_cache);
    }
  
  cache = buffer->log_attr_cache;
  line = btk_text_iter_get_line (anywhere_in_line);

  i = 0;
  while (i < ATTR_CACHE_SIZE)
    {
      if (cache->entries[i].attrs &&
          cache->entries[i].line == line)
        {
          if (char_len)
            *char_len = cache->entries[i].char_len;
          return cache->entries[i].attrs;
        }
      ++i;
    }
  
  /* Not in cache; open up the first cache entry */
  g_free (cache->entries[ATTR_CACHE_SIZE-1].attrs);
  
  g_memmove (cache->entries + 1, cache->entries,
             sizeof (CacheEntry) * (ATTR_CACHE_SIZE - 1));

  cache->entries[0].line = line;
  cache->entries[0].attrs = compute_log_attrs (anywhere_in_line,
                                               &cache->entries[0].char_len);

  if (char_len)
    *char_len = cache->entries[0].char_len;
  
  return cache->entries[0].attrs;
}

void
_btk_text_buffer_notify_will_remove_tag (BtkTextBuffer *buffer,
                                         BtkTextTag    *tag)
{
  /* This removes tag from the buffer, but DOESN'T emit the
   * remove-tag signal, because we can't afford to have user
   * code messing things up at this point; the tag MUST be removed
   * entirely.
   */
  if (buffer->btree)
    _btk_text_btree_notify_will_remove_tag (buffer->btree, tag);
}

/*
 * Debug spew
 */

void
_btk_text_buffer_spew (BtkTextBuffer *buffer)
{
  _btk_text_btree_spew (get_btree (buffer));
}

#define __BTK_TEXT_BUFFER_C__
#include "btkaliasdef.c"
