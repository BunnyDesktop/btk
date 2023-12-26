/* BTK - The GIMP Toolkit
 * btktexttagprivate.h Copyright (C) 2000 Red Hat, Inc.
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

#ifndef __BTK_TEXT_TAG_PRIVATE_H__
#define __BTK_TEXT_TAG_PRIVATE_H__

#include <btk/btk.h>

typedef struct _BtkTextBTreeNode BtkTextBTreeNode;

/* values should already have desired defaults; this function will override
 * the defaults with settings in the given tags, which should be sorted in
 * ascending order of priority
*/
void _btk_text_attributes_fill_from_tags   (BtkTextAttributes   *values,
                                            BtkTextTag         **tags,
                                            guint                n_tags);
void _btk_text_tag_array_sort              (BtkTextTag         **tag_array_p,
                                            guint                len);

/* ensure colors are allocated, etc. for drawing */
void                _btk_text_attributes_realize   (BtkTextAttributes *values,
                                                    BdkColormap       *cmap,
                                                    BdkVisual         *visual);

/* free the stuff again */
void                _btk_text_attributes_unrealize (BtkTextAttributes *values,
                                                    BdkColormap       *cmap,
                                                    BdkVisual         *visual);

gboolean _btk_text_tag_affects_size               (BtkTextTag *tag);
gboolean _btk_text_tag_affects_nonsize_appearance (BtkTextTag *tag);

#endif
