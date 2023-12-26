/* btkentryprivate.h
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

#ifndef __BTK_ENTRY_PRIVATE_H__
#define __BTK_ENTRY_PRIVATE_H__

#include <btk/btktreeviewcolumn.h>
#include <btk/btktreemodelfilter.h>
#include <btk/btkliststore.h>
#include <btk/btkentrycompletion.h>
#include <btk/btkentry.h>

B_BEGIN_DECLS

struct _BtkEntryCompletionPrivate
{
  BtkWidget *entry;

  BtkWidget *tree_view;
  BtkTreeViewColumn *column;
  BtkTreeModelFilter *filter_model;
  BtkListStore *actions;
  bboolean first_sel_changed;

  BtkEntryCompletionMatchFunc match_func;
  bpointer match_data;
  GDestroyNotify match_notify;

  bint minimum_key_length;
  bint text_column;
  bint current_selected;

  bchar *case_normalized_key;

  /* only used by BtkEntry when attached: */
  BtkWidget *popup_window;
  BtkWidget *vbox;
  BtkWidget *scrolled_window;
  BtkWidget *action_view;

  bulong completion_timeout;
  bulong changed_id;
  bulong insert_text_id;

  buint ignore_enter      : 1;
  buint has_completion    : 1;
  buint inline_completion : 1;
  buint popup_completion  : 1;
  buint popup_set_width   : 1;
  buint popup_single_match : 1;
  buint inline_selection   : 1;

  bchar *completion_prefix;

  GSource *check_completion_idle;
};

bboolean _btk_entry_completion_resize_popup (BtkEntryCompletion *completion);
void     _btk_entry_completion_popup        (BtkEntryCompletion *completion);
void     _btk_entry_completion_popdown      (BtkEntryCompletion *completion);
bchar *  _btk_entry_completion_compute_prefix (BtkEntryCompletion *completion,
					       const char         *key);

void      _btk_entry_get_borders            (BtkEntry  *entry,
					     bint      *xborder,
					     bint      *yborder);
void     _btk_entry_effective_inner_border (BtkEntry  *entry,
					    BtkBorder *border);
void     _btk_entry_reset_im_context       (BtkEntry  *entry);
B_END_DECLS

#endif /* __BTK_ENTRY_PRIVATE_H__ */
