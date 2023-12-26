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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the BTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BTK_TEXT_UTIL_H__
#define __BTK_TEXT_UTIL_H__

B_BEGIN_DECLS

/* This is a private uninstalled header shared between
 * BtkTextView and BtkEntry
 */

typedef void (* BtkTextUtilCharChosenFunc) (const char *text,
                                            gpointer    data);

void _btk_text_util_append_special_char_menuitems (BtkMenuShell              *menushell,
                                                   BtkTextUtilCharChosenFunc  func,
                                                   gpointer                   data);

BdkPixmap* _btk_text_util_create_drag_icon      (BtkWidget     *widget,
                                                 gchar         *text,
                                                 gsize          len);
BdkPixmap* _btk_text_util_create_rich_drag_icon (BtkWidget     *widget,
                                                 BtkTextBuffer *buffer,
                                                 BtkTextIter   *start,
                                                 BtkTextIter   *end);

gboolean _btk_text_util_get_block_cursor_location (BangoLayout    *layout,
						   gint            index_,
						   BangoRectangle *rectangle,
						   gboolean       *at_line_end);

B_END_DECLS

#endif /* __BTK_TEXT_UTIL_H__ */
