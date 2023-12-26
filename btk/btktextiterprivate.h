/* BTK - The GIMP Toolkit
 * btktextiterprivate.h Copyright (C) 2000 Red Hat, Inc.
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

#ifndef __BTK_TEXT_ITER_PRIVATE_H__
#define __BTK_TEXT_ITER_PRIVATE_H__

#include <btk/btktextiter.h>

B_BEGIN_DECLS

#include <btk/btktextiter.h>
#include <btk/btktextbtree.h>

BtkTextLineSegment *_btk_text_iter_get_indexable_segment      (const BtkTextIter *iter);
BtkTextLineSegment *_btk_text_iter_get_any_segment            (const BtkTextIter *iter);
BtkTextLine *       _btk_text_iter_get_text_line              (const BtkTextIter *iter);
BtkTextBTree *      _btk_text_iter_get_btree                  (const BtkTextIter *iter);
bboolean            _btk_text_iter_forward_indexable_segment  (BtkTextIter       *iter);
bboolean            _btk_text_iter_backward_indexable_segment (BtkTextIter       *iter);
bint                _btk_text_iter_get_segment_byte           (const BtkTextIter *iter);
bint                _btk_text_iter_get_segment_char           (const BtkTextIter *iter);


/* debug */
void _btk_text_iter_check (const BtkTextIter *iter);

B_END_DECLS

#endif


