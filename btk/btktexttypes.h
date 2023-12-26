/* BTK - The GIMP Toolkit
 * btktexttypes.h Copyright (C) 2000 Red Hat, Inc.
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

#ifndef __BTK_TEXT_TYPES_H__
#define __BTK_TEXT_TYPES_H__

#include <btk/btk.h>
#include <btk/btktexttagprivate.h>

B_BEGIN_DECLS

typedef struct _BtkTextCounter BtkTextCounter;
typedef struct _BtkTextLineSegment BtkTextLineSegment;
typedef struct _BtkTextLineSegmentClass BtkTextLineSegmentClass;
typedef struct _BtkTextToggleBody BtkTextToggleBody;
typedef struct _BtkTextMarkBody BtkTextMarkBody;

/*
 * Declarations for variables shared among the text-related files:
 */

#ifdef G_OS_WIN32
#ifdef BTK_COMPILATION
#define VARIABLE extern __declspec(dllexport)
#else
#define VARIABLE extern __declspec(dllimport)
#endif
#else
#define VARIABLE extern
#endif

/* In btktextbtree.c */
extern const BtkTextLineSegmentClass btk_text_char_type;
extern const BtkTextLineSegmentClass btk_text_toggle_on_type;
extern const BtkTextLineSegmentClass btk_text_toggle_off_type;

/* In btktextmark.c */
extern const BtkTextLineSegmentClass btk_text_left_mark_type;
extern const BtkTextLineSegmentClass btk_text_right_mark_type;

/* In btktextchild.c */
extern const BtkTextLineSegmentClass btk_text_pixbuf_type;
extern const BtkTextLineSegmentClass btk_text_child_type;

/*
 * UTF 8 Stubs
 */

#define BTK_TEXT_UNKNOWN_CHAR 0xFFFC
VARIABLE const gchar btk_text_unknown_char_utf8[];

gboolean btk_text_byte_begins_utf8_char (const gchar *byte);

B_END_DECLS

#endif

