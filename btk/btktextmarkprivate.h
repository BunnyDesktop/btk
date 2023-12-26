/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#ifndef __BTK_TEXT_MARK_PRIVATE_H__
#define __BTK_TEXT_MARK_PRIVATE_H__

#include <btk/btktexttypes.h>
#include <btk/btktextlayout.h>

G_BEGIN_DECLS

#define BTK_IS_TEXT_MARK_SEGMENT(mark) (((BtkTextLineSegment*)mark)->type == &btk_text_left_mark_type || \
                                ((BtkTextLineSegment*)mark)->type == &btk_text_right_mark_type)

/*
 * The data structure below defines line segments that represent
 * marks.  There is one of these for each mark in the text.
 */

struct _BtkTextMarkBody {
  BtkTextMark *obj;
  gchar *name;
  BtkTextBTree *tree;
  BtkTextLine *line;
  guint visible : 1;
  guint not_deleteable : 1;
};

void _btk_mark_segment_set_tree (BtkTextLineSegment *mark,
				 BtkTextBTree       *tree);

G_END_DECLS

#endif



