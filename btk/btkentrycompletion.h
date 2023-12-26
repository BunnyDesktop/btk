/* btkentrycompletion.h
 * Copyright (C) 2003  Kristian Rietveld  <kris@btk.org>
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

#ifndef __BTK_ENTRY_COMPLETION_H__
#define __BTK_ENTRY_COMPLETION_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btktreemodel.h>
#include <btk/btkliststore.h>
#include <btk/btktreeviewcolumn.h>
#include <btk/btktreemodelfilter.h>

B_BEGIN_DECLS

#define BTK_TYPE_ENTRY_COMPLETION            (btk_entry_completion_get_type ())
#define BTK_ENTRY_COMPLETION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_ENTRY_COMPLETION, BtkEntryCompletion))
#define BTK_ENTRY_COMPLETION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_ENTRY_COMPLETION, BtkEntryCompletionClass))
#define BTK_IS_ENTRY_COMPLETION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_ENTRY_COMPLETION))
#define BTK_IS_ENTRY_COMPLETION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_ENTRY_COMPLETION))
#define BTK_ENTRY_COMPLETION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_ENTRY_COMPLETION, BtkEntryCompletionClass))

typedef struct _BtkEntryCompletion            BtkEntryCompletion;
typedef struct _BtkEntryCompletionClass       BtkEntryCompletionClass;
typedef struct _BtkEntryCompletionPrivate     BtkEntryCompletionPrivate;

typedef gboolean (* BtkEntryCompletionMatchFunc) (BtkEntryCompletion *completion,
                                                  const gchar        *key,
                                                  BtkTreeIter        *iter,
                                                  gpointer            user_data);


struct _BtkEntryCompletion
{
  GObject parent_instance;

  /*< private >*/
  BtkEntryCompletionPrivate *GSEAL (priv);
};

struct _BtkEntryCompletionClass
{
  GObjectClass parent_class;

  gboolean (* match_selected)   (BtkEntryCompletion *completion,
                                 BtkTreeModel       *model,
                                 BtkTreeIter        *iter);
  void     (* action_activated) (BtkEntryCompletion *completion,
                                 gint                index_);
  gboolean (* insert_prefix)    (BtkEntryCompletion *completion,
				 const gchar        *prefix);
  gboolean (* cursor_on_match)  (BtkEntryCompletion *completion,
				 BtkTreeModel       *model,
				 BtkTreeIter        *iter);

  /* Padding for future expansion */
  void (*_btk_reserved0) (void);
  void (*_btk_reserved1) (void);
};

/* core */
GType               btk_entry_completion_get_type               (void) B_GNUC_CONST;
BtkEntryCompletion *btk_entry_completion_new                    (void);

BtkWidget          *btk_entry_completion_get_entry              (BtkEntryCompletion          *completion);

void                btk_entry_completion_set_model              (BtkEntryCompletion          *completion,
                                                                 BtkTreeModel                *model);
BtkTreeModel       *btk_entry_completion_get_model              (BtkEntryCompletion          *completion);

void                btk_entry_completion_set_match_func         (BtkEntryCompletion          *completion,
                                                                 BtkEntryCompletionMatchFunc  func,
                                                                 gpointer                     func_data,
                                                                 GDestroyNotify               func_notify);
void                btk_entry_completion_set_minimum_key_length (BtkEntryCompletion          *completion,
                                                                 gint                         length);
gint                btk_entry_completion_get_minimum_key_length (BtkEntryCompletion          *completion);
void                btk_entry_completion_complete               (BtkEntryCompletion          *completion);
void                btk_entry_completion_insert_prefix          (BtkEntryCompletion          *completion);

void                btk_entry_completion_insert_action_text     (BtkEntryCompletion          *completion,
                                                                 gint                         index_,
                                                                 const gchar                 *text);
void                btk_entry_completion_insert_action_markup   (BtkEntryCompletion          *completion,
                                                                 gint                         index_,
                                                                 const gchar                 *markup);
void                btk_entry_completion_delete_action          (BtkEntryCompletion          *completion,
                                                                 gint                         index_);

void                btk_entry_completion_set_inline_completion  (BtkEntryCompletion          *completion,
                                                                 gboolean                     inline_completion);
gboolean            btk_entry_completion_get_inline_completion  (BtkEntryCompletion          *completion);
void                btk_entry_completion_set_inline_selection  (BtkEntryCompletion          *completion,
                                                                 gboolean                     inline_selection);
gboolean            btk_entry_completion_get_inline_selection  (BtkEntryCompletion          *completion);
void                btk_entry_completion_set_popup_completion   (BtkEntryCompletion          *completion,
                                                                 gboolean                     popup_completion);
gboolean            btk_entry_completion_get_popup_completion   (BtkEntryCompletion          *completion);
void                btk_entry_completion_set_popup_set_width    (BtkEntryCompletion          *completion,
                                                                 gboolean                     popup_set_width);
gboolean            btk_entry_completion_get_popup_set_width    (BtkEntryCompletion          *completion);
void                btk_entry_completion_set_popup_single_match (BtkEntryCompletion          *completion,
                                                                 gboolean                     popup_single_match);
gboolean            btk_entry_completion_get_popup_single_match (BtkEntryCompletion          *completion);

const gchar         *btk_entry_completion_get_completion_prefix (BtkEntryCompletion *completion);
/* convenience */
void                btk_entry_completion_set_text_column        (BtkEntryCompletion          *completion,
                                                                 gint                         column);
gint                btk_entry_completion_get_text_column        (BtkEntryCompletion          *completion);

B_END_DECLS

#endif /* __BTK_ENTRY_COMPLETION_H__ */
