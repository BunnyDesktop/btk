/* BTK - The GIMP Toolkit
 * btktextbuffer.h Copyright (C) 2000 Red Hat, Inc.
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

#ifndef __BTK_TEXT_BUFFER_H__
#define __BTK_TEXT_BUFFER_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkwidget.h>
#include <btk/btkclipboard.h>
#include <btk/btktexttagtable.h>
#include <btk/btktextiter.h>
#include <btk/btktextmark.h>
#include <btk/btktextchild.h>

B_BEGIN_DECLS

/*
 * This is the PUBLIC representation of a text buffer.
 * BtkTextBTree is the PRIVATE internal representation of it.
 */

/* these values are used as "info" for the targets contained in the
 * lists returned by btk_text_buffer_get_copy,paste_target_list()
 *
 * the enum counts down from G_MAXUINT to avoid clashes with application
 * added drag destinations which usually start at 0.
 */
typedef enum
{
  BTK_TEXT_BUFFER_TARGET_INFO_BUFFER_CONTENTS = - 1,
  BTK_TEXT_BUFFER_TARGET_INFO_RICH_TEXT       = - 2,
  BTK_TEXT_BUFFER_TARGET_INFO_TEXT            = - 3
} BtkTextBufferTargetInfo;

typedef struct _BtkTextBTree BtkTextBTree;

typedef struct _BtkTextLogAttrCache BtkTextLogAttrCache;

#define BTK_TYPE_TEXT_BUFFER            (btk_text_buffer_get_type ())
#define BTK_TEXT_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TEXT_BUFFER, BtkTextBuffer))
#define BTK_TEXT_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TEXT_BUFFER, BtkTextBufferClass))
#define BTK_IS_TEXT_BUFFER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TEXT_BUFFER))
#define BTK_IS_TEXT_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TEXT_BUFFER))
#define BTK_TEXT_BUFFER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TEXT_BUFFER, BtkTextBufferClass))

typedef struct _BtkTextBufferClass BtkTextBufferClass;

struct _BtkTextBuffer
{
  GObject parent_instance;

  BtkTextTagTable *GSEAL (tag_table);
  BtkTextBTree *GSEAL (btree);

  GSList *GSEAL (clipboard_contents_buffers);
  GSList *GSEAL (selection_clipboards);

  BtkTextLogAttrCache *GSEAL (log_attr_cache);

  guint GSEAL (user_action_count);

  /* Whether the buffer has been modified since last save */
  guint GSEAL (modified) : 1;

  guint GSEAL (has_selection) : 1;
};

struct _BtkTextBufferClass
{
  GObjectClass parent_class;

  void (* insert_text)     (BtkTextBuffer *buffer,
                            BtkTextIter *pos,
                            const gchar *text,
                            gint length);

  void (* insert_pixbuf)   (BtkTextBuffer *buffer,
                            BtkTextIter   *pos,
                            BdkPixbuf     *pixbuf);

  void (* insert_child_anchor)   (BtkTextBuffer      *buffer,
                                  BtkTextIter        *pos,
                                  BtkTextChildAnchor *anchor);

  void (* delete_range)     (BtkTextBuffer *buffer,
                             BtkTextIter   *start,
                             BtkTextIter   *end);

  /* Only for text/widgets/pixbuf changed, marks/tags don't cause this
   * to be emitted
   */
  void (* changed)         (BtkTextBuffer *buffer);


  /* New value for the modified flag */
  void (* modified_changed)   (BtkTextBuffer *buffer);

  /* Mark moved or created */
  void (* mark_set)           (BtkTextBuffer *buffer,
                               const BtkTextIter *location,
                               BtkTextMark *mark);

  void (* mark_deleted)       (BtkTextBuffer *buffer,
                               BtkTextMark *mark);

  void (* apply_tag)          (BtkTextBuffer *buffer,
                               BtkTextTag *tag,
                               const BtkTextIter *start_char,
                               const BtkTextIter *end_char);

  void (* remove_tag)         (BtkTextBuffer *buffer,
                               BtkTextTag *tag,
                               const BtkTextIter *start_char,
                               const BtkTextIter *end_char);

  /* Called at the start and end of an atomic user action */
  void (* begin_user_action)  (BtkTextBuffer *buffer);
  void (* end_user_action)    (BtkTextBuffer *buffer);

  void (* paste_done)         (BtkTextBuffer *buffer,
                               BtkClipboard  *clipboard);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
  void (*_btk_reserved5) (void);
};

GType        btk_text_buffer_get_type       (void) B_GNUC_CONST;



/* table is NULL to create a new one */
BtkTextBuffer *btk_text_buffer_new            (BtkTextTagTable *table);
gint           btk_text_buffer_get_line_count (BtkTextBuffer   *buffer);
gint           btk_text_buffer_get_char_count (BtkTextBuffer   *buffer);


BtkTextTagTable* btk_text_buffer_get_tag_table (BtkTextBuffer  *buffer);

/* Delete whole buffer, then insert */
void btk_text_buffer_set_text          (BtkTextBuffer *buffer,
                                        const gchar   *text,
                                        gint           len);

/* Insert into the buffer */
void btk_text_buffer_insert            (BtkTextBuffer *buffer,
                                        BtkTextIter   *iter,
                                        const gchar   *text,
                                        gint           len);
void btk_text_buffer_insert_at_cursor  (BtkTextBuffer *buffer,
                                        const gchar   *text,
                                        gint           len);

gboolean btk_text_buffer_insert_interactive           (BtkTextBuffer *buffer,
                                                       BtkTextIter   *iter,
                                                       const gchar   *text,
                                                       gint           len,
                                                       gboolean       default_editable);
gboolean btk_text_buffer_insert_interactive_at_cursor (BtkTextBuffer *buffer,
                                                       const gchar   *text,
                                                       gint           len,
                                                       gboolean       default_editable);

void     btk_text_buffer_insert_range             (BtkTextBuffer     *buffer,
                                                   BtkTextIter       *iter,
                                                   const BtkTextIter *start,
                                                   const BtkTextIter *end);
gboolean btk_text_buffer_insert_range_interactive (BtkTextBuffer     *buffer,
                                                   BtkTextIter       *iter,
                                                   const BtkTextIter *start,
                                                   const BtkTextIter *end,
                                                   gboolean           default_editable);

void    btk_text_buffer_insert_with_tags          (BtkTextBuffer     *buffer,
                                                   BtkTextIter       *iter,
                                                   const gchar       *text,
                                                   gint               len,
                                                   BtkTextTag        *first_tag,
                                                   ...) B_GNUC_NULL_TERMINATED;

void    btk_text_buffer_insert_with_tags_by_name  (BtkTextBuffer     *buffer,
                                                   BtkTextIter       *iter,
                                                   const gchar       *text,
                                                   gint               len,
                                                   const gchar       *first_tag_name,
                                                   ...) B_GNUC_NULL_TERMINATED;

/* Delete from the buffer */
void     btk_text_buffer_delete             (BtkTextBuffer *buffer,
					     BtkTextIter   *start,
					     BtkTextIter   *end);
gboolean btk_text_buffer_delete_interactive (BtkTextBuffer *buffer,
					     BtkTextIter   *start_iter,
					     BtkTextIter   *end_iter,
					     gboolean       default_editable);
gboolean btk_text_buffer_backspace          (BtkTextBuffer *buffer,
					     BtkTextIter   *iter,
					     gboolean       interactive,
					     gboolean       default_editable);

/* Obtain strings from the buffer */
gchar          *btk_text_buffer_get_text            (BtkTextBuffer     *buffer,
                                                     const BtkTextIter *start,
                                                     const BtkTextIter *end,
                                                     gboolean           include_hidden_chars);

gchar          *btk_text_buffer_get_slice           (BtkTextBuffer     *buffer,
                                                     const BtkTextIter *start,
                                                     const BtkTextIter *end,
                                                     gboolean           include_hidden_chars);

/* Insert a pixbuf */
void btk_text_buffer_insert_pixbuf         (BtkTextBuffer *buffer,
                                            BtkTextIter   *iter,
                                            BdkPixbuf     *pixbuf);

/* Insert a child anchor */
void               btk_text_buffer_insert_child_anchor (BtkTextBuffer      *buffer,
                                                        BtkTextIter        *iter,
                                                        BtkTextChildAnchor *anchor);

/* Convenience, create and insert a child anchor */
BtkTextChildAnchor *btk_text_buffer_create_child_anchor (BtkTextBuffer *buffer,
                                                         BtkTextIter   *iter);

/* Mark manipulation */
void           btk_text_buffer_add_mark    (BtkTextBuffer     *buffer,
                                            BtkTextMark       *mark,
                                            const BtkTextIter *where);
BtkTextMark   *btk_text_buffer_create_mark (BtkTextBuffer     *buffer,
                                            const gchar       *mark_name,
                                            const BtkTextIter *where,
                                            gboolean           left_gravity);
void           btk_text_buffer_move_mark   (BtkTextBuffer     *buffer,
                                            BtkTextMark       *mark,
                                            const BtkTextIter *where);
void           btk_text_buffer_delete_mark (BtkTextBuffer     *buffer,
                                            BtkTextMark       *mark);
BtkTextMark*   btk_text_buffer_get_mark    (BtkTextBuffer     *buffer,
                                            const gchar       *name);

void btk_text_buffer_move_mark_by_name   (BtkTextBuffer     *buffer,
                                          const gchar       *name,
                                          const BtkTextIter *where);
void btk_text_buffer_delete_mark_by_name (BtkTextBuffer     *buffer,
                                          const gchar       *name);

BtkTextMark* btk_text_buffer_get_insert          (BtkTextBuffer *buffer);
BtkTextMark* btk_text_buffer_get_selection_bound (BtkTextBuffer *buffer);

/* efficiently move insert and selection_bound at the same time */
void btk_text_buffer_place_cursor (BtkTextBuffer     *buffer,
                                   const BtkTextIter *where);
void btk_text_buffer_select_range (BtkTextBuffer     *buffer,
                                   const BtkTextIter *ins,
				   const BtkTextIter *bound);



/* Tag manipulation */
void btk_text_buffer_apply_tag             (BtkTextBuffer     *buffer,
                                            BtkTextTag        *tag,
                                            const BtkTextIter *start,
                                            const BtkTextIter *end);
void btk_text_buffer_remove_tag            (BtkTextBuffer     *buffer,
                                            BtkTextTag        *tag,
                                            const BtkTextIter *start,
                                            const BtkTextIter *end);
void btk_text_buffer_apply_tag_by_name     (BtkTextBuffer     *buffer,
                                            const gchar       *name,
                                            const BtkTextIter *start,
                                            const BtkTextIter *end);
void btk_text_buffer_remove_tag_by_name    (BtkTextBuffer     *buffer,
                                            const gchar       *name,
                                            const BtkTextIter *start,
                                            const BtkTextIter *end);
void btk_text_buffer_remove_all_tags       (BtkTextBuffer     *buffer,
                                            const BtkTextIter *start,
                                            const BtkTextIter *end);


/* You can either ignore the return value, or use it to
 * set the attributes of the tag. tag_name can be NULL
 */
BtkTextTag    *btk_text_buffer_create_tag (BtkTextBuffer *buffer,
                                           const gchar   *tag_name,
                                           const gchar   *first_property_name,
                                           ...);

/* Obtain iterators pointed at various places, then you can move the
 * iterator around using the BtkTextIter operators
 */
void btk_text_buffer_get_iter_at_line_offset (BtkTextBuffer *buffer,
                                              BtkTextIter   *iter,
                                              gint           line_number,
                                              gint           char_offset);
void btk_text_buffer_get_iter_at_line_index  (BtkTextBuffer *buffer,
                                              BtkTextIter   *iter,
                                              gint           line_number,
                                              gint           byte_index);
void btk_text_buffer_get_iter_at_offset      (BtkTextBuffer *buffer,
                                              BtkTextIter   *iter,
                                              gint           char_offset);
void btk_text_buffer_get_iter_at_line        (BtkTextBuffer *buffer,
                                              BtkTextIter   *iter,
                                              gint           line_number);
void btk_text_buffer_get_start_iter          (BtkTextBuffer *buffer,
                                              BtkTextIter   *iter);
void btk_text_buffer_get_end_iter            (BtkTextBuffer *buffer,
                                              BtkTextIter   *iter);
void btk_text_buffer_get_bounds              (BtkTextBuffer *buffer,
                                              BtkTextIter   *start,
                                              BtkTextIter   *end);
void btk_text_buffer_get_iter_at_mark        (BtkTextBuffer *buffer,
                                              BtkTextIter   *iter,
                                              BtkTextMark   *mark);

void btk_text_buffer_get_iter_at_child_anchor (BtkTextBuffer      *buffer,
                                               BtkTextIter        *iter,
                                               BtkTextChildAnchor *anchor);

/* There's no get_first_iter because you just get the iter for
   line or char 0 */

/* Used to keep track of whether the buffer needs saving; anytime the
   buffer contents change, the modified flag is turned on. Whenever
   you save, turn it off. Tags and marks do not affect the modified
   flag, but if you would like them to you can connect a handler to
   the tag/mark signals and call set_modified in your handler */

gboolean        btk_text_buffer_get_modified            (BtkTextBuffer *buffer);
void            btk_text_buffer_set_modified            (BtkTextBuffer *buffer,
                                                         gboolean       setting);

gboolean        btk_text_buffer_get_has_selection       (BtkTextBuffer *buffer);

void btk_text_buffer_add_selection_clipboard    (BtkTextBuffer     *buffer,
						 BtkClipboard      *clipboard);
void btk_text_buffer_remove_selection_clipboard (BtkTextBuffer     *buffer,
						 BtkClipboard      *clipboard);

void            btk_text_buffer_cut_clipboard           (BtkTextBuffer *buffer,
							 BtkClipboard  *clipboard,
                                                         gboolean       default_editable);
void            btk_text_buffer_copy_clipboard          (BtkTextBuffer *buffer,
							 BtkClipboard  *clipboard);
void            btk_text_buffer_paste_clipboard         (BtkTextBuffer *buffer,
							 BtkClipboard  *clipboard,
							 BtkTextIter   *override_location,
                                                         gboolean       default_editable);

gboolean        btk_text_buffer_get_selection_bounds    (BtkTextBuffer *buffer,
                                                         BtkTextIter   *start,
                                                         BtkTextIter   *end);
gboolean        btk_text_buffer_delete_selection        (BtkTextBuffer *buffer,
                                                         gboolean       interactive,
                                                         gboolean       default_editable);

/* Called to specify atomic user actions, used to implement undo */
void            btk_text_buffer_begin_user_action       (BtkTextBuffer *buffer);
void            btk_text_buffer_end_user_action         (BtkTextBuffer *buffer);

BtkTargetList * btk_text_buffer_get_copy_target_list    (BtkTextBuffer *buffer);
BtkTargetList * btk_text_buffer_get_paste_target_list   (BtkTextBuffer *buffer);

/* INTERNAL private stuff */
void            _btk_text_buffer_spew                  (BtkTextBuffer      *buffer);

BtkTextBTree*   _btk_text_buffer_get_btree             (BtkTextBuffer      *buffer);

const BangoLogAttr* _btk_text_buffer_get_line_log_attrs (BtkTextBuffer     *buffer,
                                                         const BtkTextIter *anywhere_in_line,
                                                         gint              *char_len);

void _btk_text_buffer_notify_will_remove_tag (BtkTextBuffer *buffer,
                                              BtkTextTag    *tag);

B_END_DECLS

#endif
